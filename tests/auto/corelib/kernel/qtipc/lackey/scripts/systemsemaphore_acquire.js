#/bin/qscript
function QVERIFY(x, debugInfo) {
    if (!(x)) {
        print(debugInfo);
        throw(debugInfo);
    }
}


var sem = new ScriptSystemSemaphore;
sem.setKey("store");

var count = Number(args[1]);
if (isNaN(count))
    count = 1;
for (var i = 0; i < count; ++i)
    QVERIFY(sem.acquire());
print("done aquiring");
