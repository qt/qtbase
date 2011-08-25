function QVERIFY(x, debugInfo) {
    if (!(x)) {
        print(debugInfo);
        throw(debugInfo);
    }
}

var sem = new ScriptSystemSemaphore;
sem.setKey("store");
QVERIFY(sem.release());
print ("done releasing");
