function QVERIFY(x, debugInfo) {
    if (!(x)) {
        print(debugInfo);
        throw(debugInfo);
    }
}

var consumer = new ScriptSharedMemory;
consumer.setKey("market");

//print("consumer starting");
var tries = 0;;
while(!consumer.attach()) {
    if (tries == 5000) {
        var message = "consumer exiting, waiting too long";
        print(message);
        throw(message);
    }
    ++tries;
    consumer.sleep(1);
}
//print("consumer attached");


var i = 0;
while(true) {
    QVERIFY(consumer.lock(), "lock");
    if (consumer.get(0) == 'Q') {
        consumer.set(0, ++i);
        //print ("consumer sets" + i);
    }
    if (consumer.get(0) == 'E') {
        QVERIFY(consumer.unlock(), "unlock");
        break;
    }
    QVERIFY(consumer.unlock(), "unlock");
    consumer.sleep(10);
}

//print("consumer detaching");
QVERIFY(consumer.detach());
