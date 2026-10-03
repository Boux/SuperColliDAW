// Instrument
// SuperColliDAW.instrument plays a synth for every note the
// track sends in. Declare only the arguments you need. Each
// note fills in:
//   freq: the pitch, following the pitch wheel and MPE slides
//   noteFreq: the pitch of the key, without bend
//   midinote, chan, gate
//   amp: the velocity from 0 to 1
//   velocity: from 0 to 127
//   bend: the pitch wheel in semitones
//   timbre: CC 74 from -1 to 1, 0 until it moves
//   pressure: channel aftertouch from 0 to 1
// With MPE, bend, timbre and pressure are per note, and per-note
// bend spans 48 semitones (set it with mpeBendRange_).
// The sustain pedal works without any code.

(
SuperColliDAW.instrument { |freq, amp, gate = 1, timbre, pressure|
    var env = EnvGen.kr(Env.adsr(0.005, 0.2, 0.6, 0.4), gate, doneAction: 2);
    var sig = Pulse.ar(freq * [1, 1.005], 0.4);
    sig = RLPF.ar(sig, freq * timbre.lag(0.02).linexp(-1, 1, 1, 8), 0.4);
    sig * env * (amp + pressure.lag(0.02)).min(1) * 0.3
};
);

// One voice per MIDI channel that glides between held keys,
// with the wheel bending an octave. \mono also plays one voice
// but restarts it for each key. \poly is the default.
(
SuperColliDAW.instrument { |freq, amp, gate = 1|
    var env = EnvGen.kr(Env.adsr(0.01, 0.3, 0.7, 0.3), gate, doneAction: 2);
    var sig = Saw.ar(freq.lag(0.08));
    RLPF.ar(sig, freq.lag(0.08) * 4, 0.3) * env * amp * 0.3 ! 2
}.mode_(\legato).bendRange_(12);
)
