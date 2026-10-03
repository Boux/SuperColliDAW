// DAW clock: patterns follow the DAW's tempo and bars

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

// runs when the DAW starts playing
SuperColliDAW.onPlay {
    // quant -1: start on the next bar
    ~kick = Pbind(\instrument, \kick, \dur, 1).play(quant: -1);
    ~hats = Pbind(\instrument, \hat, \dur, 0.25, \amp, Pseq([0.12, 0.04, 0.08, 0.04], inf)).play(quant: -1);
};

// runs when the DAW stops
SuperColliDAW.onStop { ~kick.stop; ~hats.stop };
);

SuperColliDAW.playing.postln;
SuperColliDAW.clock.bar.postln;
SuperColliDAW.clock.beatInBar.postln;
SuperColliDAW.clock.beatsPerBar.postln;
// beats per second
SuperColliDAW.clock.tempo.postln;
SuperColliDAW.clock.songBeats.postln;
