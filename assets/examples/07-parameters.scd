// Parameters: up to 32 knobs in the DAW, numbered 0 to 31
//
// SuperColliDAW.kr(number, name, low, high, curve, step, start, unit)
//   number: which DAW parameter, 0 to 31
//   name: the name the DAW shows
//   low, high: the knob's range
//   curve: how the knob moves through the range:
//     \lin evenly, \exp evenly across octaves (good for Hz),
//     \db like a volume fader, or a number to bend it
//   step: rounds the value to multiples of it, 0 for smooth
//   start: the value before you touch the knob
//   unit: shown after the value in the DAW, like "Hz"
// Leave out what you don't need: by default a knob goes from 0
// to 1, evenly, starting at low. Names SuperCollider knows, like
// \freq or \pan, bring their own range.

(
{
    // parameter 0, "cutoff": 100 to 8000 Hz, starts at 1200
    // \exp spreads it evenly across octaves
    var cutoff = SuperColliDAW.kr(0, \cutoff, 100, 8000, \exp, start: 1200, unit: "Hz");

    // parameter 1, "rq": 0.05 to 1, starts at 0.5
    var rq = SuperColliDAW.kr(1, \rq, 0.05, 1, \exp, start: 0.5);

    // parameter 2, "pan": -1 to 1, SuperCollider's built-in pan range
    var pan = SuperColliDAW.kr(2, \pan);

    // parameter 3, "octave": snaps to 0, 1, 2 or 3, starts at 1
    var octave = SuperColliDAW.kr(3, \octave, 0, 3, step: 1, start: 1);

    // parameter 4, "drive": 1 to 20, moves slowly at first, faster near 20
    var drive = SuperColliDAW.kr(4, \drive, 1, 20, 4);

    // parameter 5, "gain": -30 to 6 dB, starts at 0 dB
    var gain = SuperColliDAW.kr(5, \gain, -30, 6, \db, start: 0, unit: "dB").dbamp;

    // parameter 6, no name: plain 0 to 1
    var wobble = In.kr(6);

    var sig = Saw.ar(110 * (2 ** octave)) * drive;
    sig = RLPF.ar(sig, cutoff * SinOsc.kr(4).range(1 - (wobble * 0.5), 1), rq).tanh;

    Pan2.ar(sig * gain * 0.15, pan);
}.play;
);

// show parameter 7 in the DAW without using it in a synth
SuperColliDAW.declare(7, \spare, 0, 100, step: 1, start: 50, unit: "%");
