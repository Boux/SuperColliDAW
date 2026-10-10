// Examples: F1, or the book button at the top right

// Instrument: plays a synth with the track's midi notes
/*
(
SuperColliDAW.instrument { |freq, noteFreq, midinote, amp, velocity, gate = 1, bend, timbre, pressure, chan|
    var env = EnvGen.kr(Env.adsr(0.01, 0.3, 0.5, 1), gate, doneAction: 2);

    var sig = SinOsc.ar(freq) ! 2;

    sig * env * amp * 0.5;
};
)
*/

// Track effect: changes the track's audio
/*
(
Ndef(\fx, {
    var sig = SoundIn.ar([0, 1]);

    sig;
}).play;
)
*/
