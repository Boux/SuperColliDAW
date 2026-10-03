// Instrument: a synth per note from the track
//
// SuperColliDAW.instrument { |freq, amp, gate = 1| ... }
// Take any of these arguments, in any order:
//   freq: the note's pitch in Hz, including bend
//   noteFreq: the key's pitch in Hz, without bend
//   midinote: the key, 0 to 127 (60 is middle C)
//   amp: how hard the key was hit, 0 to 1
//   velocity: the same, 1 to 127
//   gate: 1 while the key is held, 0 when released
//   bend: the pitch wheel, in semitones
//   timbre (MPE): -1 to 1, center at 0
//   pressure (MPE): 0 to 1
//   chan: the MIDI channel, 0 to 15
// freq, bend, timbre and pressure keep changing while a note
// plays.

(
~inst = SuperColliDAW.instrument { |freq, noteFreq, midinote, amp, velocity, gate = 1, bend, timbre, pressure, chan|
    // see the "Parameters" example for an explanation of these
    var cutoff = SuperColliDAW.kr(0, \cutoff, 100, 8000, \exp, start: 1200, unit: "Hz");
    var rq = SuperColliDAW.kr(1, \rq, 0.05, 1, \exp, start: 0.5);
    var drive = In.kr(2);

    // env uses gate, highly recommended: doneAction: 2 ends the synth after the release
    var env = EnvGen.kr(Env.adsr(0.005, 1, 0.6, 0.2), gate, doneAction: 2);

    // (MPE pressure) pressing harder adds vibrato
    var vibrato = 1 + (SinOsc.kr(5.5) * pressure.lag(0.05) * 0.015);

    var sig = Saw.ar(freq * vibrato * [1, 1.005]) * (1 + (drive * 8));

    // (MPE timbre) moves the cutoff up to an octave down or up
    sig = RLPF.ar(sig, (cutoff * (2 ** timbre.lag(0.1))).clip(20, 18000), rq).tanh * 0.15;

    sig * env * amp
};
);

// \legato: one note at a time, new keys change its pitch without restarting it
~inst.mode = \legato;
// \mono: one note at a time, restarting on each new key
~inst.mode = \mono;
// \poly: every key plays (default)
~inst.mode = \poly;

// pitch wheel range, semitones (default 2)
~inst.bendRange = 12;

// MPE per-note bend range, semitones (default 48)
~inst.mpeBendRange = 48;

// or play a SynthDef by name: it needs gate and out arguments, and must free itself
(
SynthDef(\pad, { |out = 0, freq = 440, amp = 0.5, gate = 1|
    var env = EnvGen.kr(Env.adsr(0.3, 0.2, 0.8, 1), gate, doneAction: 2);
    Out.ar(out, LPF.ar(Saw.ar(freq * [1, 1.01]), 2000) * env * amp * 0.1);
}).add;
);
~inst = SuperColliDAW.instrument(\pad);
