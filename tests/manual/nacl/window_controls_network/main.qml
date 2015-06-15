import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import QtQuick.Controls 1.2

ApplicationWindow {
    visible: true

    Image {
        id : img1
    }

    ColumnLayout {
        spacing: 2
        
        // Absolute url, loads from network
        Button {
            text: "Load http://localhost:8000/foo.png"
            onClicked: {
                img1.source = "http://localhost:8000/foo.png"
            }
        }

        // Relative Url, loads from qrc (where this file is placed)
        Button {
            text: "Load foo.png"
            onClicked: {
                img1.source = "foo.png"
            }
        }
    }
}
