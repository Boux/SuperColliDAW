// Global LFO: one LFO on a bus that every voice reads

(
// buses from Bus.control never clash with the plugin's parameters
~lfoBus = ~lfoBus ?? { Bus.control(s, 1) };
~lfo = { Out.kr(~lfoBus, SinOsc.kr(0.1).range(0.95, 1.05)) }.play;

// see the "Instrument" example for these arguments
~inst = SuperColliDAW.instrument { |freq, noteFreq, midinote, amp, velocity, gate = 1, bend, timbre, pressure, chan|
    var env = EnvGen.kr(Env.adsr(0.01, 0.2, 0.7, 0.5), gate, doneAction: 2);

    // every voice reads the same LFO
    var sig = Saw.ar(freq * ~lfoBus.kr * [1, 1.003]);

    LPF.ar(sig, freq * 6) * env * amp * 0.2;
};
)
