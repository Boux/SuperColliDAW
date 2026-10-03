SuperColliDAW {
	classvar <server, <clock, <pluginAddr, numParameters;

	*initClass {
		var port = "SUPERCOLLIDAW_SERVER_PORT".getenv;
		if(port.isNil) { ^this };
		numParameters = this.prEnvInteger("SUPERCOLLIDAW_NUM_PARAMETERS", 0);
		Class.initClassTree(Server);
		server = SuperColliDAWServer(\supercollidaw, NetAddr("127.0.0.1", port.asInteger), this.prServerOptions);
		// Its own NetAddr, because inside Server:makeBundle (s.bind) server.addr collects messages into the bundle.
		pluginAddr = NetAddr("127.0.0.1", port.asInteger);
		Server.default = server;
		Class.initClassTree(TempoClock);
		clock = SuperColliDAWClock.new;
		TempoClock.default = clock;
		StartUp.add { SuperColliDAWMIDI.receive; CmdPeriod.add(SuperColliDAWMIDI); clock.prListen };
	}

	*serverStarted {
		server.boot;
	}

	*serverStopped {
		server.prStopped;
	}

	*run { |code|
		CmdPeriod.run;
		server.doWhenBooted { code.interpret };
	}

	*evaluate { |code|
		var function = code.compile;
		if(function.isNil) { ^this };
		this.prWhenRunning { ("-> " ++ function.value).postln };
	}

	*stop {
		CmdPeriod.run;
	}

	*playing {
		^clock.playing
	}

	*onPlay { |function|
		clock.onPlay(function)
	}

	*onStop { |function|
		clock.onStop(function)
	}

	*instrument { |synthDefOrFunction|
		^SuperColliDAWInstrument(synthDefOrFunction)
	}

	*prWhenRunning { |function|
		if(server.serverRunning) { function.value } { server.doWhenBooted(function) };
	}

	// The plugin writes the DAW's tempo and beat position to the two control buses after the parameters.
	*bpm {
		^In.kr(numParameters)
	}

	*beats {
		^In.kr(numParameters + 1)
	}

	*kr { |index, name, spec|
		spec = this.prSpec(spec, name);
		this.declare(index, name, spec);
		^spec.map(In.kr(index))
	}

	*declare { |index, name, spec|
		var warp;
		spec = this.prSpec(spec, name);
		warp = spec.warp.asSpecifier;
		pluginAddr.sendMsg('/supercollidaw/param', index, name.asString, spec.minval, spec.maxval,
			if(warp.isNumber) { "curve" } { warp.asString }, if(warp.isNumber) { warp } { 0 },
			spec.step, spec.default, spec.units.asString);
	}

	*prSpec { |spec, name|
		^(spec ? name.asSymbol).asSpec ?? { ControlSpec() }
	}

	*prServerOptions {
		^ServerOptions.new
			.numInputBusChannels_(this.prEnvInteger("SUPERCOLLIDAW_NUM_INPUTS", 2))
			.numOutputBusChannels_(this.prEnvInteger("SUPERCOLLIDAW_NUM_OUTPUTS", 2))
			.reservedNumControlBusChannels_(numParameters + 2)
	}

	*prEnvInteger { |name, default|
		^(name.getenv ? default).asInteger
	}
}

SuperColliDAWServer : Server {
	boot { |startAliveThread = true, recover = false, onFailure|
		this.startAliveThread;
	}

	reboot { |func, onFailure|
		"SuperColliDAW: the server lives inside the plugin and is always running.".postln;
	}

	quit { |onComplete, onFailure, watchShutDown = true|
		"SuperColliDAW: the server lives inside the plugin and cannot be quit.".postln;
	}

	// Server:quit without sending /quit: the plugin has already destroyed the server.
	prStopped {
		statusWatcher.quit(watchShutDown: false);
		maxNumClients = nil;
		volume.freeSynth;
		RootNode(this).freeAll;
		this.newAllocators;
	}
}
