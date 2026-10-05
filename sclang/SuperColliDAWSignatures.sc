// Signature rules ported from LanguageServer.quark's SignatureHelpProvider (MIT, 2022 Scott Carver): https://github.com/scztt/LanguageServer.quark
SuperColliDAWSignatures {
	classvar limit = 8, receivers;

	*initClass {
		receivers = [
			"(?:^|[^\\w])([A-Z]\\w*)$" -> { |name| name.asSymbol.asClass !? _.class },
			"~(\\w+)$" -> { |name| currentEnvironment[name.asSymbol] !? _.class },
			"\\}$" -> { Function },
			"\\]$" -> { Array },
			"\"$" -> { String }
		];
	}

	*send { |callee|
		var signatures = this.signatures(callee);
		var fields = signatures.keep(limit).collect { |signature| [signature[\label], signature[\parameters].size] ++ signature[\parameters].flatten };
		SuperColliDAW.pluginAddr.sendMsg('/supercollidaw/signatures', callee, signatures.size, *fields.flatten);
	}

	*signatures { |callee|
		var name = callee.findRegexp("[A-Za-z_]\\w*$");
		var before;
		if(name.isEmpty) { ^[] };
		before = callee[..name[0][0] - 1];
		name = name[0][1];
		if(name[0].isUpper) { ^this.prSignaturesOf(name, \new) };
		if(before.endsWith(".").not) { ^this.prSignaturesNamed(name.asSymbol, true) };
		^this.prSignaturesOf(before.drop(-1), name.asSymbol)
	}

	*prSignaturesOf { |receiver, selector|
		var class = this.prReceiverClass(receiver);
		var method = class !? { class.findRespondingMethodFor(selector) };
		if(class.isNil) { ^this.prSignaturesNamed(selector, false) };
		if(method.isNil) { ^[] };
		^[this.prDescribe(method, this.prLabel(class, selector), false)]
	}

	*prSignaturesNamed { |selector, receiverFirst|
		var methods = Class.allClasses.reject(_.isMetaClass).collect { |class| class.findMethod(selector) }.reject(_.isNil);
		^methods.collect { |method| this.prDescribe(method, this.prLabel(method.ownerClass, selector), receiverFirst) }
	}

	*prReceiverClass { |receiver|
		var rule = receivers.detect { |rule| receiver.findRegexp(rule.key).notEmpty };
		^rule !? { rule.value.value(receiver.findRegexp(rule.key).last[1]) }
	}

	// Calling name(receiver, ...) passes the receiver as the first argument.
	*prDescribe { |method, label, receiverFirst|
		var names = method.argNames.asArray.collect(_.asString);
		var defaults = method.prototypeFrame;
		var parameters = names.collect { |name, i| [this.prParameterName(method, name, i), this.prDefault(method, defaults[i], i)] };
		^(label: label, parameters: if(receiverFirst) { parameters } { parameters.drop(1) })
	}

	*prLabel { |class, selector|
		if(class.isMetaClass) { ^class.name.asString.drop("Meta_".size) ++ "." ++ selector };
		^class.name.asString ++ ":" ++ selector
	}

	*prParameterName { |method, name, index|
		^if(this.prIsVarArgs(method, index)) { "..." ++ name } { name }
	}

	*prDefault { |method, value, index|
		if(value.isNil or: { index >= this.prFirstCollected(method) }) { ^"" };
		if(value.isKindOf(Symbol) and: { value.asString.findRegexp("^\\w+$").notEmpty }) { ^"\\" ++ value };
		^value.asCompileString
	}

	*prIsVarArgs { |method, index|
		^method.hasVarArgs and: { index == this.prFirstCollected(method) }
	}

	// The ...varargs argument comes last, followed by the kwargs one when the method takes keyword arguments.
	*prFirstCollected { |method|
		^method.argNames.size - method.varArgsValue
	}
}
