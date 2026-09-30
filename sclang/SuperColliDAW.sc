SuperColliDAW {
	classvar <server;

	*initClass {
		var port = "SUPERCOLLIDAW_SERVER_PORT".getenv;
		if(port.isNil) { ^this };
		Class.initClassTree(Server);
		server = SuperColliDAWServer(\supercollidaw, NetAddr("127.0.0.1", port.asInteger), this.prServerOptions);
		Server.default = server;
		StartUp.add { server.startAliveThread };
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

	*prWhenRunning { |function|
		if(server.serverRunning) { function.value } { server.doWhenBooted(function) };
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
		server.addr.sendMsg('/supercollidaw/param', index, name.asString, spec.minval, spec.maxval,
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
			.reservedNumControlBusChannels_(this.prEnvInteger("SUPERCOLLIDAW_NUM_PARAMETERS", 0))
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
}
