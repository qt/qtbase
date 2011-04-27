#/bin/qscript
function QVERIFY(x, socket) {
    if (!(x)) {
        throw(socket.errorString());
    }
}

var socket = new QScriptLocalSocket;
var tries = 0;
do {
    socket.serverName = "qlocalsocket_autotest";
    if ((socket.errorString() != "QLocalSocket::connectToServer: Invalid name")
        && (socket.errorString() != "QLocalSocket::connectToServer: Connection refused"))
        break;
    socket.sleep(1);
    ++tries;
    print("isConnected:", socket.isConnected());
} while ((socket.errorString() == "QLocalSocket::connectToServer: Invalid name"
        || (socket.errorString() == "QlocalSocket::connectToServer: Connection refused"))
        && tries < 5000);
if (tries == 5000) {
    print("too many tries, exiting");
} else {
socket.waitForConnected();
//print("isConnected:", socket.isConnected());
if (!socket.isConnected())
    print("Not Connected:", socket.errorString());
socket.waitForReadyRead();
var text = socket.readLine();
var testLine = "test";
QVERIFY((text == testLine), socket);
QVERIFY((socket.errorString() == "Unknown error"), socket);
socket.close();
//print("client: exiting", text);
}
