#/bin/qscript
function QVERIFY(x, server) {
    if (!(x)) {
        throw(server.errorString());
    }
}
var server = new QScriptLocalServer;
QVERIFY(server.listen("qlocalsocket_autotest"), server);
var done = args[1];
var testLine = "test";
while (done > 0) {
    QVERIFY(server.waitForNewConnection(), server);
    var serverSocket = server.nextConnection();
    serverSocket.write(testLine);
    QVERIFY(serverSocket.waitForBytesWritten(), serverSocket);
    QVERIFY(serverSocket.errorString() == ""
            ||serverSocket.errorString() == "Unknown error", serverSocket);
    --done;
}
