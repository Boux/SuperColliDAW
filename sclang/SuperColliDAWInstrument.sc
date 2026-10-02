SuperColliDAWInstrument {
	classvar current;
	var defName, voices;

	*new { |synthDefOrFunction|
		current !? { current.releaseAll };
		current = super.newCopyArgs(this.prDefName(synthDefOrFunction), Array.newClear(16 * 128));
		^current.prListen
	}

	*prDefName { |synthDefOrFunction|
		if(synthDefOrFunction.isFunction.not) { ^synthDefOrFunction.asDefName };
		^synthDefOrFunction.asSynthDef(fadeTime: 0.02, name: \supercollidawInstrument).add.name
	}

	releaseAll {
		voices.do { |voice| this.prRelease(voice) };
		voices.fill(nil);
	}

	prListen {
		MIDIdef.noteOn(\supercollidawInstrumentOn, { |vel, note, chan| this.prNoteOn(note, vel, chan) });
		MIDIdef.noteOff(\supercollidawInstrumentOff, { |vel, note, chan| this.prNoteOff(note, chan) });
	}

	prNoteOn { |note, vel, chan|
		this.prNoteOff(note, chan);
		voices[this.prVoiceIndex(note, chan)] = Synth(defName, [freq: note.midicps, midinote: note, amp: vel / 127, velocity: vel, chan: chan, gate: 1]);
	}

	prNoteOff { |note, chan|
		var index = this.prVoiceIndex(note, chan);
		this.prRelease(voices[index]);
		voices[index] = nil;
	}

	prVoiceIndex { |note, chan|
		^(chan * 128) + note
	}

	// The synth may already have freed itself, e.g. with Env.perc, so the release must not report a missing node.
	prRelease { |voice|
		voice !? { voice.server.sendBundle(nil, [\error, -1], voice.releaseMsg) }
	}
}
