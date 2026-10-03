// Tempo sync: the DAW's tempo and position inside a synth

(
~sync = {
    // tempo in bpm
    var bpm = SuperColliDAW.bpm;
    // position in beats
    var beats = SuperColliDAW.beats;
    // one sweep per bar
    var cutoff = SinOsc.kr(0, (beats / 4).frac * 2pi).range(300, 3000);
    // eighth-note tremolo
    var tremolo = 1 - ((beats * 2).frac * 0.8);
    // one vibrato cycle per beat
    var vibrato = SinOsc.kr(bpm / 60).range(0.995, 1.005);
    RLPF.ar(Saw.ar([55, 55.2] * vibrato), cutoff, 0.3) * tremolo * 0.1
}.play;
)
