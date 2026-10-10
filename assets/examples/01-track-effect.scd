// Effect on the track's audio
//
// SoundIn.ar([0, 1]): the track's audio coming in, left and right
// whatever the synth returns goes back out to the track

(
Ndef(\fx, {
    var in = SoundIn.ar([0, 1]);

    // see the "Parameters" example for an explanation of this
    var mix = SuperColliDAW.kr(0, \mix, 0, 1, start: 0.3);

    // a dotted eighth note at the DAW's tempo
    var time = 60 / SuperColliDAW.bpm * 0.75;

    var echoes = CombC.ar(in, 2, time, 3);

    in + (echoes * mix);
}).play;
)
