// MIDI out
// MIDIOut(0) sends to the track, so the device after
// SuperColliDAW receives the notes. To record them, add another
// track and set its input to this SuperColliDAW device.

(
~out = MIDIOut(0);
~melody = Pbind(
    \type, \midi,
    \midiout, ~out,
    \chan, 0,
    \degree, Pseq([0, 2, 4, 7, 4, 2], inf),
    \dur, 0.25,
    \legato, 0.8,
    \amp, 0.7
).play(quant: -1);
);

// A MIDI effect: each note from the track comes out with a
// fifth above it. latency 0 sends right away, instead of 0.2 s
// ahead to stay in time with patterns.
(
~thru = MIDIOut(0).latency_(0);
MIDIdef.noteOn(\fifthOn, { |vel, note, chan| [0, 7].do { |interval| ~thru.noteOn(chan, note + interval, vel) } });
MIDIdef.noteOff(\fifthOff, { |vel, note, chan| [0, 7].do { |interval| ~thru.noteOff(chan, note + interval, vel) } });
)
