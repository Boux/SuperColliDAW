// Global LFO: one LFO on a bus, shared by every voice

(
~lfoBus = ~lfoBus ?? { Bus.control(s, 1) };
~lfo = { Out.kr(~lfoBus, SinOsc.kr(0.1).range(0.95, 1.05)) }.play;

SuperColliDAW.instrument { |freq, amp, gate = 1|
    var env = EnvGen.kr(Env.adsr(0.01, 0.2, 0.7, 0.5), gate, doneAction: 2);
    var sig = Saw.ar(freq * ~lfoBus.kr * [1, 1.003]);
    LPF.ar(sig, freq * 6) * env * amp * 0.2
};
)
