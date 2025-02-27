description("basic tests for destructuring assignment");

function testDestructuring(pattern, expression, result, expr) {
    if (!expr) expr = "a+b"
    shouldBe("var " + pattern + "=" + expression + "; var r="+expr+"; r", result);
    var functionString = "(function(" + pattern + ") { return "+expr+";})";
    debug("Function as String: " + functionString);
    shouldBe(functionString + "(" + expression + ")", result);
    shouldBe("(" + eval(functionString) + ")(" + expression + ")", result);
}

testDestructuring("[a,b]", "['1','2']", "'12'");
testDestructuring("{a,b}", "{a:'1',b:'2'}", "'12'");
testDestructuring("{c:a,d:b}", "{c:'1',d:'2'}", "'12'");
testDestructuring("{c:b,d:a}", "{c:'1',d:'2'}", "'21'");
testDestructuring("{true:a,false:b,undefined:c,null:d,in:e,for:f,1.5:g,'foo bar':h}", "{true:'a',false:'b',undefined:'c',null:'d',in:'e',for:'f',1.5:'g','foo bar':'h'}", "'abcdefgh'", "a+b+c+d+e+f+g+h");
testDestructuring("[{c:a,d:b}]", "[{c:'1',d:'2'}]", "'12'");
testDestructuring("{x:[{c:a,d:b}]}", "{x:[{c:'1',d:'2'}]}", "'12'");

var anArray = ['1', '2'];
anArray.a = '3'
anArray.b = '4'
var anObject = {a:'1', b:'2', 0:'3',1:'4'}
testDestructuring("[a,b]", "anArray", "'12'");
testDestructuring("{a,b}", "anArray", "'34'");
testDestructuring("{a:a,b:b}", "anArray", "'34'");
testDestructuring("{a,b}", "anObject", "'12'");
testDestructuring("{a:a,b:b}", "anObject", "'12'");
testDestructuring("{0:a,1:b}", "anObject", "'34'");
testDestructuring("{'a':a,'b':b}", "anObject", "'12'");
var [a,b] = ['11','22']
shouldBe("a+b", "'1122'")
var [b,a] = [a,b];
shouldBe("a+b", "'2211'")
function testDeconstructArgs() {
    var [a,b] = arguments;
    return a+b;
}

function testDeconstructArgLength() {
    var {length} = arguments;
    return length;
}
shouldBe("testDeconstructArgs('1', '2')", "'12'");
shouldBe("testDeconstructArgLength('1', '2')", "2");



