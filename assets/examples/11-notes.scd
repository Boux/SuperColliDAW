// Notes: things to watch out for

// exponential ranges can't start at 0: In.kr(0).linexp(0, 1, 0, 4000)
// makes the plugin go silent until you stop that synth, so start
// above 0, like 20

// the plugin's output is cut off above about +2 dB

// { SinOsc.kr(1) }.play writes over parameter 0: send control
// signals to your own bus instead (Bus.control)

// Run all runs the whole file as one piece of code: end a block
// with ); when more code follows it
