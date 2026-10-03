// Effect on the track's audio
// SoundIn.ar([0, 1]) is the audio coming into SuperColliDAW
// from the track. Whatever a synth writes to Out.ar(0, ...),
// or returns with .play, goes back out to the track.

(
{
    var in = SoundIn.ar([0, 1]);
    var echoes = CombC.ar(in, 0.5, [0.25, 0.375], 3);
    in + (echoes * 0.4)
}.play;
)
