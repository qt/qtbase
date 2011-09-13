function QVERIFY(x, debugInfo) {
    if (!(x)) {
        print(debugInfo);
        throw(debugInfo);
    }
}

var producer = new ScriptSharedMemory;
producer.setKey("market");

var size = 1024;
if (!producer.create(size)) {
    QVERIFY(producer.error() == 4, "create");
    QVERIFY(producer.attach());
}
//print ("producer created and attached");

QVERIFY(producer.lock());
producer.set(0, 'Q');
QVERIFY(producer.unlock());

var i = 0;
while(i < 5) {
    QVERIFY(producer.lock(), "lock");
    if (producer.get(0) == 'Q') {
        QVERIFY(producer.unlock(), "unlock");
        producer.sleep(1);
        continue;
    }
    //print("producer: " + i);
    ++i;
    producer.set(0, 'Q');
    QVERIFY(producer.unlock(), "unlock");
    producer.sleep(1);
}
QVERIFY(producer.lock());
producer.set(0, 'E');
QVERIFY(producer.unlock());

//print ("producer done");

// Sleep for a bit to let all consumers start, otherwise they will get stuck in the attach loop,
// because at least in Symbian the shared memory will be destroyed if there are no active handles to it.
producer.sleep(3000);