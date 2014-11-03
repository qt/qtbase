import QtQuick 2.1
import QtQuick.Window 2.0
import QtQuick.Controls 1.2

ApplicationWindow {
    visible: true

    Button {
        x: 20
        y: 20
        text: "Button"
    }
    Calendar {
        x: 20
        y: 60
    }
    TextArea {
        x: 300
        y: 20
        width: 300
        height: 300
    }
    Slider {
        x: 300
        y: 320
        width: 300
    }
    Switch {
        x: 300
        y: 350
    }
    SpinBox {
        x: 400
        y: 350
    }
}
