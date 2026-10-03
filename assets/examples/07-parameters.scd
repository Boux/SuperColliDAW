// Parameters
// SuperColliDAW.kr(index, name, spec) is plugin parameter
// number index, shown with that name in the DAW, so you can
// automate it or map it to a controller. The spec turns the
// DAW's 0 to 1 range into useful values, like any ControlSpec:
//   [min, max, warp, step, default, units]
// In.kr(index) reads the same parameter raw, from 0 to 1.

(
{
    var cutoff = SuperColliDAW.kr(0, \cutoff, [100, 8000, \exp, 0, 1200, "Hz"]);
    var rq = SuperColliDAW.kr(1, \rq, [0.05, 1, \exp, 0, 0.5]);
    var drive = In.kr(2);
    var sig = Saw.ar([110, 110.5]) * (1 + (drive * 8));
    RLPF.ar(sig, cutoff, rq).tanh * 0.15
}.play;
)

// Exponential ranges cannot start at 0:
//   In.kr(0).linexp(0, 1, 0, 4000)
// gives NaN as soon as the parameter moves. Start above 0:
//   In.kr(0).linexp(0, 1, 20, 4000)
