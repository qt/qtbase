function QVERIFY(x, debugInfo) {
    if (!(x)) {
        print(debugInfo);
        throw(debugInfo);
    }
}

var lock = new ScriptSystemLock;
lock.setKey("market");
QVERIFY(lock.lockReadOnly());
QVERIFY(lock.unlock());