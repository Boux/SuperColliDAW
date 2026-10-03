// MIDI from the track
//
// MIDIdef works as usual, with no MIDIClient setup: the track is
// the only MIDI source

// post everything the track sends, then stop posting
MIDIFunc.trace(true);
MIDIFunc.trace(false);

(
SynthDef(\ping, { |out = 0, freq = 440, amp = 0.2|
    var env = EnvGen.kr(Env.perc(0.001, 0.6), doneAction: 2);
    Out.ar(out, SinOsc.ar(freq) * env * amp ! 2);
}).add;

~drone = { |freq = 55, cutoff = 400, amp = 0.08| RLPF.ar(Saw.ar(freq.lag(0.05) * [1, 1.007]), cutoff.lag(0.1), 0.3) * amp.lag(0.1) }.play;

// keys play a ping
MIDIdef.noteOn(\ping, { |vel, note, chan| Synth(\ping, [freq: note.midicps, amp: vel / 127 * 0.2]) });
MIDIdef.noteOff(\released, { |vel, note, chan| ("released " ++ note).postln });

// the mod wheel opens the drone's filter
MIDIdef.cc(\modWheel, { |val| ~drone.set(\cutoff, val.linexp(0, 127, 200, 6000)) }, 1);

// the pitch wheel bends the drone
MIDIdef.bend(\wheel, { |val| ~drone.set(\freq, 55 * ((val - 8192) / 8192 * 2).midiratio) });

// aftertouch makes the drone louder
MIDIdef.touch(\pressure, { |val| ~drone.set(\amp, val.linlin(0, 127, 0.08, 0.25)) });

MIDIdef.polytouch(\keyPressure, { |val, note| [\polytouch, note, val].postln });
MIDIdef.program(\program, { |num| ("program " ++ num).postln });
)
