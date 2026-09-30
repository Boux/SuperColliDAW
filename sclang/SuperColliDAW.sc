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

	*prServerOptions {
		^ServerOptions.new
			.numInputBusChannels_("SUPERCOLLIDAW_NUM_INPUTS".getenv.asInteger)
			.numOutputBusChannels_("SUPERCOLLIDAW_NUM_OUTPUTS".getenv.asInteger)
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
