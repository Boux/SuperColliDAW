// MIDI from the track
// MIDIdef works as in plain SuperCollider, with no MIDIClient
// setup: the track is the only MIDI source.

(
SynthDef(\ping, { |out = 0, freq = 440, amp = 0.2|
    var env = EnvGen.kr(Env.perc(0.001, 0.6), doneAction: 2);
    Out.ar(out, SinOsc.ar(freq) * env * amp ! 2);
}).add;

MIDIdef.noteOn(\ping, { |vel, note| Synth(\ping, [freq: note.midicps, amp: vel / 127 * 0.2]) });

// The mod wheel (CC 1) opens the filter of a drone.
~drone = { |cutoff = 400| RLPF.ar(Saw.ar([55, 55.4]), cutoff.lag(0.1), 0.3) * 0.08 }.play;
MIDIdef.cc(\modWheel, { |val| ~drone.set(\cutoff, val.linexp(0, 127, 200, 6000)) }, 1);
)
