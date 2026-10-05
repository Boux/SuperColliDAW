// Completion rules ported from LanguageServer.quark's LSPCompletionHandler (MIT, 2022 Scott Carver): https://github.com/scztt/LanguageServer.quark
SuperColliDAWCompletion {
	classvar limit = 100, classNames, methodNames, afterTrigger;

	*initClass {
		afterTrigger = (
			$.: { |prefix, typed| this.prMethodNames(prefix, typed) },
			$~: { |prefix, typed| this.prEnvironmentNames(prefix, typed) },
			$(: { |prefix, typed| this.prDefNames(prefix, typed) }
		);
	}

	*send { |line|
		SuperColliDAW.pluginAddr.sendMsg('/supercollidaw/completions', line, *this.complete(line));
	}

	*complete { |line|
		var word = line.findRegexp("\\w*$")[0][1];
		^this.prCandidates(line).select { |name| word.isEmpty or: { name.beginsWith(word) } }.keep(limit)
	}

	*prCandidates { |line|
		var trigger = line.findRegexp("[.~(][^.~(]*$");
		if(line.findRegexp("\\b[A-Z]\\w*$").notEmpty) { ^this.prClassNames };
		if(trigger.isEmpty) { ^[] };
		^afterTrigger[trigger[0][1][0]].value(line[..trigger[0][0] - 1], trigger[0][1][1..])
	}

	*prMethodNames { |prefix, typed|
		var class = this.prClassAtEnd(prefix);
		if(this.prMatches(typed, "^\\w*$").not) { ^[] };
		if(class.isNil) { ^this.prAllMethodNames };
		^this.prUnique(class.class.superclasses.addFirst(class.class).collect { |each| this.prMethodNamesOf(each).sort }.flatten)
	}

	*prEnvironmentNames { |prefix, typed|
		if(this.prMatches(typed, "^\\w*$").not or: { this.prMatches(prefix, "\\w$") }) { ^[] };
		^currentEnvironment.keys.asArray.collect(_.asString).sort
	}

	*prDefNames { |prefix, typed|
		var class = this.prClassAtEnd(prefix);
		if(this.prMatches(typed, "^\\\\\\w*$").not or: { this.prIsDefClass(class).not }) { ^[] };
		^this.prDefsOf(class).keys.asArray.collect(_.asString).select { |name| this.prMatches(name, "^\\w+$") }.sort
	}

	*prClassAtEnd { |text|
		var name = text.findRegexp("(?:^|[^\\w])([A-Z]\\w*)$");
		^if(name.isEmpty) { nil } { name[1][1].asSymbol.asClass }
	}

	*prIsDefClass { |class|
		^[Ndef, Pdef, Tdef, Fdef, HIDdef, MIDIdef, OSCdef].includes(class)
	}

	*prDefsOf { |class|
		if(class == Ndef) { ^Ndef.all[Server.default.name] ? () };
		^class.all
	}

	*prClassNames {
		^classNames ?? { classNames = Class.allClasses.reject(_.isMetaClass).collect { |class| class.name.asString }.sort }
	}

	*prAllMethodNames {
		^methodNames ?? { methodNames = this.prUnique(Class.allClasses.collect { |class| this.prMethodNamesOf(class) }.flatten.sort) }
	}

	*prMethodNamesOf { |class|
		^(class.methods ? []).collect { |method| method.name.asString }.select { |name| this.prIsPublicMethodName(name) }
	}

	*prIsPublicMethodName { |name|
		^this.prMatches(name, "^[a-z]\\w*$") and: { this.prMatches(name, "^pr[A-Z]").not }
	}

	// matchRegexp never matches an empty string, even with a pattern that allows one.
	*prMatches { |text, pattern|
		^text.findRegexp(pattern).notEmpty
	}

	*prUnique { |names|
		var seen = Set.new;
		^names.reject { |name| seen.includes(name) or: { seen.add(name); false } }
	}
}
