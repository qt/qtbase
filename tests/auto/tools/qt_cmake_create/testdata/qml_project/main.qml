import QtQuick
import QtQuick.Window

Window {
    width: 640
    height: 480
    visible: true
    title: "Hello World"
    Text {
        anchors.centerIn: parent
        font.pointSize: 16
        text: "Now I have CMakeLists.txt. Thanks!"
    }
    TestComponent {
    }
}
