// Complex synth: supersaw chords into a shared reverb

(
~reverbBus = ~reverbBus ?? { Bus.audio(s, 2) };

SynthDef(\supersaw, { |out = 0, fx = 0, freq = 110, amp = 0.1, gate = 1, cutoff = 2500, send = 0.3|
    var env = EnvGen.kr(Env.adsr(0.02, 0.3, 0.6, 0.8), gate, doneAction: 2);
    var filterEnv = EnvGen.kr(Env.perc(0.005, 0.6), levelScale: 3, levelBias: 1);
    var lfo = SinOsc.kr(0.2).range(0.7, 1.3);

    var sig = Splay.ar(Saw.ar(freq * [-0.12, -0.04, 0.04, 0.12].midiratio));
    sig = RLPF.ar(sig, (cutoff * filterEnv * lfo).clip(20, 18000), 0.3) * env * amp;

    Out.ar(out, sig);
    Out.ar(fx, sig * send);
}).add;

SynthDef(\reverb, { |out = 0, in = 0|
    var sig = In.ar(in, 2);
    Out.ar(out, FreeVerb2.ar(sig[0], sig[1], 1, 0.85, 0.4));
}).add;

fork {
    s.sync;
    ~reverb = Synth.tail(s, \reverb, [in: ~reverbBus]);

    // chords start on the DAW's next bar (quant: -1)
    ~chords = Pbind(
        \instrument, \supersaw,
        \fx, ~reverbBus,
        \degree, Pseq([[0, 2, 4], [-2, 0, 3], [-3, 0, 2], [-1, 1, 4]], inf),
        \octave, 4,
        \dur, 2,
        \legato, 0.9
    ).play(quant: -1);
};
)
