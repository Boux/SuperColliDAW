// DAW clock
//
// SuperColliDAW.clock follows the DAW's tempo, bars and position,
// and patterns play on it
// SuperColliDAW.onPlay { }: runs when the DAW starts playing
// SuperColliDAW.onStop { }: runs when the DAW stops
// quant: -1 starts a pattern on the next bar, 1 on the next beat
// a pattern started on play misses its first 0.2 s (s.latency)

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

// kick and hats start on the next bar when the DAW plays
SuperColliDAW.onPlay {
    ~kick = Pbind(\instrument, \kick, \dur, 1).play(quant: -1);
    ~hats = Pbind(\instrument, \hat, \dur, 0.25, \amp, Pseq([0.12, 0.04, 0.08, 0.04], inf)).play(quant: -1);
};

// and stop with it
SuperColliDAW.onStop { ~kick.stop; ~hats.stop };
);

// where the DAW is
SuperColliDAW.playing.postln;
SuperColliDAW.clock.bar.postln;
SuperColliDAW.clock.beatInBar.postln;
SuperColliDAW.clock.beatsPerBar.postln;
SuperColliDAW.clock.songBeats.postln;

// tempo in beats per second
SuperColliDAW.clock.tempo.postln;
