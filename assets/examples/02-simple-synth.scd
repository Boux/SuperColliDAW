// Simple synth
// Ctrl+Enter: run block, Shift+Enter: run line, Ctrl+.: stop

{ SinOsc.ar([220, 221], 0, 0.1) }.play;

(
SynthDef(\pluck, { |out = 0, freq = 220, amp = 0.2, pan = 0|
    var env = EnvGen.kr(Env.perc(0.005, 0.8), doneAction: 2);
    var sig = LPF.ar(Saw.ar(freq), freq * 4) * env * amp;
    Out.ar(out, Pan2.ar(sig, pan));
}).add;

// plays at the DAW's tempo
~pluck = Pbind(\instrument, \pluck, \degree, Pseq([0, 2, 4, 7], inf), \dur, 0.25, \pan, Pwhite(-0.5, 0.5)).play;
);

~pluck.stop;
