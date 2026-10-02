SuperColliDAWMIDI {
	classvar <uid = 0;

	*receive {
		OSCFunc({ |msg| this.dispatch(msg[1], msg[2], msg[3]) }, '/supercollidaw/midi', NetAddr("127.0.0.1", nil)).fix;
	}

	*dispatch { |status, data1, data2|
		var channel = status & 16r0F;
		switch(status & 16rF0,
			16r80, { MIDIIn.doNoteOffAction(uid, channel, data1, data2) },
			16r90, { MIDIIn.doNoteOnAction(uid, channel, data1, data2) },
			16rA0, { MIDIIn.doPolyTouchAction(uid, channel, data1, data2) },
			16rB0, { MIDIIn.doControlAction(uid, channel, data1, data2) },
			16rC0, { MIDIIn.doProgramAction(uid, channel, data1) },
			16rD0, { MIDIIn.doTouchAction(uid, channel, data1) },
			16rE0, { MIDIIn.doBendAction(uid, channel, (data2 << 7) | data1) }
		);
	}

	// In the order MIDIClient:list reads them: source uids, devices, names, then destination uids, names, devices.
	*endpoints {
		^[[uid], ["SuperColliDAW"], ["Track"], [], [], []]
	}
}
