SuperColliDAWInstrument {
	classvar current;
	var defName, voices, held, pedals, bends, sustained, <mode = \poly, <>bendRange = 2;

	*new { |synthDefOrFunction|
		current !? { current.releaseAll };
		current = super.newCopyArgs(this.prDefName(synthDefOrFunction), Array.newClear(16 * 128), Array.fill(16, { List.new }), false ! 16, 0 ! 16, Set.new);
		^current.prListen
	}

	*prDefName { |synthDefOrFunction|
		if(synthDefOrFunction.isFunction.not) { ^synthDefOrFunction.asDefName };
		^synthDefOrFunction.asSynthDef(fadeTime: 0.02, name: \supercollidawInstrument).add.name
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

	prListen {
		MIDIdef.noteOn(\supercollidawInstrumentOn, { |vel, note, chan| this.prNoteOn(note, vel, chan) });
		MIDIdef.noteOff(\supercollidawInstrumentOff, { |vel, note, chan| this.prNoteOff(note, chan) });
		MIDIdef.cc(\supercollidawInstrumentSustain, { |val, num, chan| this.prSustain(val >= 64, chan) }, 64);
		MIDIdef.bend(\supercollidawInstrumentBend, { |val, chan| this.prBend(val, chan) });
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

	prBend { |val, chan|
		bends[chan] = (val - 8192) / 8192 * bendRange;
		this.prChannelVoices(chan).do { |voice| this.prSend(voice, voice.setMsg(\bend, bends[chan])) };
	}

	prStartVoice { |note, vel, chan|
		voices[this.prVoiceIndex(note, chan)] = Synth(defName, [freq: note.midicps, midinote: note, amp: vel / 127, velocity: vel, chan: chan, bend: bends[chan], gate: 1]);
	}

	prGlide { |fromNote, toNote, chan|
		var voice = voices[this.prVoiceIndex(fromNote, chan)];
		voices[this.prVoiceIndex(fromNote, chan)] = nil;
		sustained.remove(this.prVoiceIndex(fromNote, chan));
		voices[this.prVoiceIndex(toNote, chan)] = voice;
		this.prSend(voice, voice.setMsg(\freq, toNote.midicps, \midinote, toNote));
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
