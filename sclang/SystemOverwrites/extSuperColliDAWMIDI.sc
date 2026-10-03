// The track is the only MIDI device, so these replace every method that reaches the OS MIDI primitives.
+ MIDIClient {
	*prInitClient { }
	*prInit { |inports, outports| }
	*prList { ^SuperColliDAWMIDI.endpoints }
	*prDisposeClient { }
	*restart { }
	*externalSources { ^sources }
	*externalDestinations { ^destinations }
}

+ MIDIIn {
	*connectByUID { |inport, uid| }
	*disconnectByUID { |inport, uid| }
}

+ MIDIOut {
	*connectByUID { |outport, uid| }
	*disconnectByUID { |outport, uid| }

	send { |outport, uid, len, hiStatus, loStatus, a = 0, b = 0, late|
		SuperColliDAWMIDI.send(late, hiStatus | (loStatus & 16r0F), a, b);
	}

	// TODO: send sysex to the track as CLAP_EVENT_MIDI_SYSEX.
	prSysex { |uid, packet|
		"SuperColliDAW: MIDIOut:sysex is not supported yet.".warn;
	}
}
