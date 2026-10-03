// Effect on the track's audio

(
{
    // track input
    var in = SoundIn.ar([0, 1]);
    // parameter 0, "mix" in the DAW
    var mix = SuperColliDAW.kr(0, \mix, 0, 1, start: 0.3);
    // dotted eighth at the DAW's tempo
    var time = 60 / SuperColliDAW.bpm * 0.75;
    var echoes = CombC.ar(in, 2, time, 3);
    // returned audio goes to the track output
    in + (echoes * mix)
}.play;
)
