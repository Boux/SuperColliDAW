// MIDI out
//
// MIDIOut(0) sends MIDI to the next device on the track
// to record it, set another track's input to this SuperColliDAW
// Stop and Run all release any notes left on
// sysex is not supported yet

(
~out = MIDIOut(0);

// a melody, starting on the DAW's next bar
~melody = Pbind(\type, \midi, \midiout, ~out, \chan, 0, \degree, Pseq([0, 2, 4, 7, 4, 2], inf), \dur, 0.25, \legato, 0.8, \amp, 0.7).play(quant: -1);
);

(
// CC 74 sweeping up and down every 8 beats
~sweep = Pbind(\type, \midi, \midiout, ~out, \midicmd, \control, \chan, 0, \ctlNum, 74, \control, Pseg([0, 127, 0], [4, 4], \lin, inf), \dur, 0.125).play;
);

// a single message: CC 1 set to 64 on the first channel
~out.control(0, 1, 64);

// 0.2 s ahead (default) lines up with synths, 0 sends right away
~out.latency = 0.2;

(
// a live MIDI effect: every note from the track gets a fifth above
~thru = MIDIOut(0).latency_(0);
MIDIdef.noteOn(\fifthOn, { |vel, note, chan| [0, 7].do { |interval| ~thru.noteOn(chan, note + interval, vel) } });
MIDIdef.noteOff(\fifthOff, { |vel, note, chan| [0, 7].do { |interval| ~thru.noteOff(chan, note + interval, vel) } });
);
