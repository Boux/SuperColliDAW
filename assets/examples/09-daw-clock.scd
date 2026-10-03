// DAW clock
// Patterns play on SuperColliDAW.clock, which follows the DAW's
// tempo, bars and position. onPlay and onStop run when the
// DAW's transport starts and stops.

(
SynthDef(\kick, { |out = 0, amp = 0.5|
    var env = EnvGen.kr(Env.perc(0.001, 0.3), doneAction: 2);
    var sig = SinOsc.ar(EnvGen.kr(Env([200, 50], [0.08], \exp)), 0.5pi) * env * amp;
    OffsetOut.ar(out, sig ! 2);
}).add;

SynthDef(\hat, { |out = 0, amp = 0.1|
    var env = EnvGen.kr(Env.perc(0.001, 0.05), doneAction: 2);
    OffsetOut.ar(out, HPF.ar(WhiteNoise.ar, 8000) * env * amp ! 2);
}).add;

SuperColliDAW.onPlay {
    // quant: -1 starts on the next bar line, in any meter.
    ~kick = Pbind(\instrument, \kick, \dur, 1).play(quant: -1);
    ~hats = Pbind(\instrument, \hat, \dur, 0.25, \amp, Pseq([0.12, 0.04, 0.08, 0.04], inf)).play(quant: -1);
};

SuperColliDAW.onStop { ~kick.stop; ~hats.stop };
);

// Synths can lock to the DAW too. SuperColliDAW.beats is the
// DAW's position in beats and SuperColliDAW.bpm its tempo.
// While the DAW is stopped, beats keeps counting at its tempo.
// This filter sweeps once per bar in 4/4, through tempo changes.
(
~sweep = {
    var phase = (SuperColliDAW.beats / 4).frac * 2pi;
    var cutoff = SinOsc.kr(0, phase).range(300, 3000);
    RLPF.ar(Saw.ar([55, 55.2]), cutoff, 0.3) * 0.1
}.play;
);

// Where the DAW is: SuperColliDAW.playing, and on
// SuperColliDAW.clock: bar, beatInBar, songBeats and tempo.
SuperColliDAW.clock.bar.postln;
