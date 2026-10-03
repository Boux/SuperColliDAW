SuperColliDAWInstrument {
	const nullRpn = 16383, pitchBendSensitivity = 0;
	classvar current;
	var defName, voices, held, pedals, wheels, timbres, pressures, bendRanges, rpns, sustained, <mode = \poly;

	*new { |synthDefOrFunction|
		current !? { current.releaseAll };
		current = super.new.prInit(this.prDefName(synthDefOrFunction));
		^current.prListen
	}

	*prDefName { |synthDefOrFunction|
		if(synthDefOrFunction.isFunction.not) { ^synthDefOrFunction.asDefName };
		^synthDefOrFunction.asSynthDef(fadeTime: 0.02, name: \supercollidawInstrument).add.name
	}

	bendRange {
		^bendRanges[0]
	}

	bendRange_ { |semitones|
		bendRanges[0] = semitones;
		this.prUpdateBend(0);
	}

	mpeBendRange {
		^bendRanges[1]
	}

	mpeBendRange_ { |semitones|
		(1..15).do { |chan| bendRanges[chan] = semitones };
		this.prUpdateBend(0);
	}

	mode_ { |newMode|
		if([\poly, \mono, \legato].includes(newMode).not) { Error("SuperColliDAW.instrument: mode must be \\poly, \\mono or \\legato, not %".format(newMode.cs)).throw };
		mode = newMode;
	}

	releaseAll {
		voices.do { |voice| this.prRelease(voice) };
		voices.fill(nil);
		sustained.clear;
	}

	prInit { |name|
		defName = name;
		voices = Array.newClear(16 * 128);
		held = Array.fill(16, { List.new });
		pedals = false ! 16;
		wheels = 0 ! 16;
		timbres = 0 ! 16;
		pressures = 0 ! 16;
		// Channel 1 is the MPE master channel; per-note bend on channels 2 to 16 defaults to MPE's 48 semitones.
		bendRanges = [2] ++ (48 ! 15);
		rpns = nullRpn ! 16;
		sustained = Set.new;
	}

	prListen {
		MIDIdef.noteOn(\supercollidawInstrumentOn, { |vel, note, chan| this.prNoteOn(note, vel, chan) });
		MIDIdef.noteOff(\supercollidawInstrumentOff, { |vel, note, chan| this.prNoteOff(note, chan) });
		MIDIdef.cc(\supercollidawInstrumentControl, { |val, num, chan| this.prControl(num, val, chan) }, [6, 64, 74, 100, 101]);
		MIDIdef.bend(\supercollidawInstrumentBend, { |val, chan| this.prBend(val, chan) });
		MIDIdef.touch(\supercollidawInstrumentPressure, { |val, chan| this.prPressure(val, chan) });
	}

	prNoteOn { |note, vel, chan|
		var sounding = this.prSoundingNote(chan);
		held[chan] = held[chan].reject { |key| key.key == note }.add(note -> vel);
		if(mode == \legato and: { sounding.notNil } and: { held[chan].size > 1 }) { ^this.prGlide(sounding, note, chan) };
		if(mode == \poly) { this.prReleaseVoice(note, chan) } { this.prReleaseChannel(chan) };
		this.prStartVoice(note, vel, chan);
	}

	prNoteOff { |note, chan|
		held[chan] = held[chan].reject { |key| key.key == note };
		if(mode != \poly and: { this.prSoundingNote(chan) == note } and: { held[chan].notEmpty }) { ^this.prReturnTo(held[chan].last, note, chan) };
		this.prReleaseOrSustain(note, chan);
	}

	prSustain { |down, chan|
		pedals[chan] = down;
		if(down) { ^this };
		sustained.select { |index| index div: 128 == chan }.do { |index| this.prReleaseIndex(index) };
	}

	prControl { |num, val, chan|
		switch(num,
			6, { this.prDataEntry(val, chan) },
			64, { this.prSustain(val >= 64, chan) },
			74, { this.prTimbre(val, chan) },
			100, { rpns[chan] = (rpns[chan] & 16r3F80) | val },
			101, { rpns[chan] = (val << 7) | (rpns[chan] & 16r7F) }
		);
	}

	prDataEntry { |val, chan|
		if(rpns[chan] != pitchBendSensitivity) { ^this };
		bendRanges[chan] = val;
		this.prUpdateBend(chan);
	}

	prBend { |val, chan|
		wheels[chan] = (val - 8192) / 8192;
		this.prUpdateBend(chan);
	}

	prTimbre { |val, chan|
		timbres[chan] = (val - 64) / if(val >= 64) { 63 } { 64 };
		this.prSetVoices(chan, \timbre, timbres[chan]);
	}

	prPressure { |val, chan|
		pressures[chan] = val / 127;
		this.prUpdatePressure(chan);
	}

	// Bend and pressure on channel 1, the MPE master channel, reach every voice.
	prUpdateBend { |chan|
		this.prAffectedChannels(chan).do { |each| this.prBendVoices(each) };
	}

	prBendVoices { |chan|
		var bend = this.prVoiceBend(chan);
		128.do { |note| voices[this.prVoiceIndex(note, chan)] !? { |voice| this.prSend(voice, voice.setMsg(\bend, bend, \freq, (note + bend).midicps)) } };
	}

	prUpdatePressure { |chan|
		this.prAffectedChannels(chan).do { |each| this.prSetVoices(each, \pressure, this.prVoicePressure(each)) };
	}

	prAffectedChannels { |chan|
		^if(chan == 0) { (0..15) } { [chan] }
	}

	prVoiceBend { |chan|
		^(wheels[chan] * bendRanges[chan]) + if(chan == 0) { 0 } { wheels[0] * bendRanges[0] }
	}

	prVoicePressure { |chan|
		^max(pressures[chan], pressures[0])
	}

	prSetVoices { |chan, name, value|
		this.prChannelVoices(chan).do { |voice| this.prSend(voice, voice.setMsg(name, value)) };
	}

	prStartVoice { |note, vel, chan|
		var bend = this.prVoiceBend(chan);
		var args = [freq: (note + bend).midicps, noteFreq: note.midicps, midinote: note, amp: vel / 127, velocity: vel, chan: chan, gate: 1];
		voices[this.prVoiceIndex(note, chan)] = Synth(defName, args ++ [bend: bend, timbre: timbres[chan], pressure: this.prVoicePressure(chan)]);
	}

	prGlide { |fromNote, toNote, chan|
		var voice = voices[this.prVoiceIndex(fromNote, chan)];
		voices[this.prVoiceIndex(fromNote, chan)] = nil;
		sustained.remove(this.prVoiceIndex(fromNote, chan));
		voices[this.prVoiceIndex(toNote, chan)] = voice;
		this.prSend(voice, voice.setMsg(\freq, (toNote + this.prVoiceBend(chan)).midicps, \noteFreq, toNote.midicps, \midinote, toNote));
	}

	prReturnTo { |key, fromNote, chan|
		if(mode == \legato) { ^this.prGlide(fromNote, key.key, chan) };
		this.prReleaseVoice(fromNote, chan);
		this.prStartVoice(key.key, key.value, chan);
	}

	prReleaseOrSustain { |note, chan|
		var index = this.prVoiceIndex(note, chan);
		if(voices[index].isNil) { ^this };
		if(pedals[chan]) { ^sustained.add(index) };
		this.prReleaseIndex(index);
	}

	prReleaseChannel { |chan|
		128.do { |note| this.prReleaseVoice(note, chan) };
	}

	prReleaseVoice { |note, chan|
		this.prReleaseIndex(this.prVoiceIndex(note, chan));
	}

	prReleaseIndex { |index|
		this.prRelease(voices[index]);
		voices[index] = nil;
		sustained.remove(index);
	}

	prSoundingNote { |chan|
		^(0..127).detect { |note| voices[this.prVoiceIndex(note, chan)].notNil }
	}

	prChannelVoices { |chan|
		^voices.copyRange(chan * 128, chan * 128 + 127).select(_.notNil)
	}

	prVoiceIndex { |note, chan|
		^(chan * 128) + note
	}

	prRelease { |voice|
		voice !? { this.prSend(voice, voice.releaseMsg) }
	}

	// The synth may already have freed itself, e.g. with Env.perc, so messages to it must not report a missing node.
	prSend { |voice, message|
		voice.server.sendBundle(nil, [\error, -1], message);
	}
}
