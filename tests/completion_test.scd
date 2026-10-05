(
var failures = List.new;
var check = { |line, test, description|
	var names = SuperColliDAWCompletion.complete(line);
	if(test.value(names).not) { failures.add("% %: got %".format(line.cs, description, names.keep(10))) };
};

~foo = 1;
~fizz = 2;
Pdef(\bass, Pbind());
Pdef(\beat, Pbind());
Pdef('with space', Pbind());

check.("SinOsc.a", { |names| names.first == "ar" }, "lists the class's own methods first");
check.("SinOsc.", { |names| names.keep(2) == ["ar", "kr"] }, "lists class methods before typing a letter");
check.("Sin", { |names| names.includesEqual("SinOsc") and: { names.every(_.beginsWith("Sin")) } }, "completes class names");
check.("x = Pbi", { |names| names.includesEqual("Pbind") }, "completes class names after code");
check.("SinOsc.ar(Sin", { |names| names.includesEqual("SinOsc") }, "completes class names inside arguments");
check.("{ SinOsc.ar(440).linex", { |names| names.includesEqual("linexp") }, "completes methods after a call");
check.("s.bo", { |names| names.includesEqual("boot") }, "completes methods on a variable");
check.(".", { |names| names.notEmpty and: { names.every { |name| "^[a-z]\\w*$".matchRegexp(name) and: { "^pr[A-Z]".matchRegexp(name).not } } } }, "hides operators and private methods");
check.("~f", { |names| names == ["fizz", "foo"] }, "completes environment variables");
check.("a~f", { |names| names.isEmpty }, "ignores ~ inside a word");
check.("Pdef(\\b", { |names| names == ["bass", "beat"] }, "completes Pdef names");
check.("Pdef(b", { |names| names.isEmpty }, "needs a backslash for Pdef names");
check.("var freq", { |names| names.isEmpty }, "stays quiet on a plain word");
check.("x.foo bar", { |names| names.isEmpty }, "stays quiet after a space");
check.("", { |names| names.isEmpty }, "stays quiet on an empty line");

failures.do(_.postln);
if(failures.isEmpty) { "completion: all passed".postln };
0.exit;
)
