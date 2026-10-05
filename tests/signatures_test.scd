(
var failures = List.new;
var describe = { |signature| signature[\label] ++ "(" ++ signature[\parameters].collect { |parameter| parameter[0] ++ if(parameter[1].isEmpty) { "" } { " = " ++ parameter[1] } }.join(", ") ++ ")" };
var check = { |callee, test, description|
	var signatures = SuperColliDAWSignatures.signatures(callee).collect(describe);
	if(test.value(signatures).not) { failures.add("% %: got %".format(callee.cs, description, signatures.keep(4))) };
};

~list = List[1, 2];

check.("SuperColliDAW.kr", { |s| s == ["SuperColliDAW.kr(number, name, low, high, curve, step, start, unit)"] }, "describes our own class method");
check.("x = SinOsc.ar", { |s| s == ["SinOsc.ar(freq = 440.0, phase = 0.0, mul = 1.0, add = 0.0)"] }, "shows defaults");
check.("SinOsc.ar(LFNoise1.kr", { |s| s == ["LFNoise1.kr(freq = 500.0, mul = 1.0, add = 0.0)"] }, "names an inherited class method after the class written");
check.("Pdef", { |s| s == ["Pdef.new(key, item)"] }, "a class called directly is new");
check.("Pbind", { |s| s == ["Pbind.new(...pairs, kwargs)"] }, "marks variable and keyword arguments");
check.("{ }.play", { |s| s.first.beginsWith("Function:play(target, outbus = 0") }, "knows a function literal");
check.("~list.add", { |s| s == ["List:add(item)"] }, "knows an environment variable's class");
check.("x.play", { |s| s.size > 1 and: { s.every(_.contains(":play(")) } }, "lists every play for an unknown receiver");
check.("if", { |s| s.includesEqual("True:if(this, trueFunc, falseFunc)") }, "a bare call takes the receiver first");
check.("Nonexistent.foo", { |s| s.isEmpty }, "an unknown method has no signature");

failures.do(_.postln);
if(failures.isEmpty) { "signatures: all passed".postln };
0.exit;
)
