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
