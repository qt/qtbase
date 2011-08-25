function QVERIFY(x, debugInfo) {
    if (!(x)) {
        print(debugInfo);
        throw(debugInfo);
    }
}

var lock = new ScriptSystemSemaphore;
lock.setKey("store");
QVERIFY(lock.acquire());
QVERIFY(lock.release());
