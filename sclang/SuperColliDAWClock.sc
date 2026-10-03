SuperColliDAWClock : TempoClock {
	const regridBeats = 0.05;
	var <playing = false, songOffset = 0, playActions, stopActions;

	*new {
		^super.new(1, queueSize: 2048).permanent_(true)
	}

	prListen {
		OSCFunc({ |msg, time| this.prFollow(time, *msg[1..]) }, '/supercollidaw/transport', NetAddr("127.0.0.1", nil)).fix;
		CmdPeriod.add(this);
	}

	songBeats {
		^this.beats + songOffset
	}

	onPlay { |function|
		playActions = playActions.addFunc(function);
		if(playing) { function.value(this) };
	}

	onStop { |function|
		stopActions = stopActions.addFunc(function);
	}

	cmdPeriod {
		playActions = nil;
		stopActions = nil;
	}

	// Events sent at a logical time sound server.latency later, so the playhead at `time` pins the clock at time - latency.
	prFollow { |time, playingFlag, bpm, newSongBeats, barStartBeats, barNumber, newBeatsPerBar|
		var isPlaying = playingFlag == 1;
		var bps = bpm / 60;
		var reference = time - (SuperColliDAW.server.latency ? 0);
		var beatsAtReference = newSongBeats - songOffset;
		var drift = (beatsAtReference - this.secs2beats(reference)).abs;
		var regrid = isPlaying and: { playing.not or: { drift > regridBeats } or: { newBeatsPerBar != beatsPerBar } };
		case
			{ regrid } { this.prRegrid(reference, bps, newSongBeats, barStartBeats, barNumber, newBeatsPerBar) }
			{ isPlaying } { this.prSetAll(bps, beatsAtReference, reference) }
			{ this.setTempoAtSec(bps, Main.elapsedTime) };
		this.prSetPlaying(isPlaying);
	}

	// Moves the bar grid to the DAW's instead of moving the beats, so running patterns never jump back in time.
	prRegrid { |reference, bps, newSongBeats, barStartBeats, barNumber, newBeatsPerBar|
		this.setTempoAtSec(bps, reference);
		songOffset = newSongBeats - this.secs2beats(reference);
		baseBarBeat = barStartBeats - songOffset;
		baseBar = barNumber;
		beatsPerBar = newBeatsPerBar;
		barsPerBeat = newBeatsPerBar.reciprocal;
		this.changed(\meter);
	}

	prSetAll { |newTempo, newBeats, newSeconds|
		_TempoClock_SetAll
		^this.primitiveFailed
	}

	prSetPlaying { |isPlaying|
		if(isPlaying == playing) { ^this };
		playing = isPlaying;
		if(playing) { playActions.value(this) } { stopActions.value(this) };
		this.changed(\playing);
	}
}
