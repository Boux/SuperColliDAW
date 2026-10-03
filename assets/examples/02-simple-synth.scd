// Simple synth
// A SynthDef describes a sound, and a Pbind plays it as a
// sequence of notes. Put the cursor inside the parentheses and
// press Ctrl+Enter. Ctrl+. stops all sound.

(
SynthDef(\pluck, { |out = 0, freq = 220, amp = 0.2, pan = 0|
    var env = EnvGen.kr(Env.perc(0.005, 0.8), doneAction: 2);
    var sig = LPF.ar(Saw.ar(freq), freq * 4) * env * amp;
    Out.ar(out, Pan2.ar(sig, pan));
}).add;

~pluck = Pbind(\instrument, \pluck, \degree, Pseq([0, 2, 4, 7], inf), \dur, 0.25).play;
);

// The quickest way to hear something: a function played once,
// without a SynthDef.
{ SinOsc.ar([220, 221], 0, 0.1) }.play;
