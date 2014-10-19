import QtQuick 2.1

Rectangle {
     width: 250
     height: 250
     color: "lightsteelblue"

     Rectangle {
         y: 100;
         x: 10
         width: 80; height: 80
         gradient: Gradient {
             GradientStop { position: 0.0; color: "lightsteelblue" }
             GradientStop { position: 1.0; color: "blue" }
         }
    }

    Rectangle {
        y: 100
        x: 150
        width: 80
        height: 80
        rotation: 90
        gradient: Gradient {
            GradientStop { position: 0.0; color: "lightsteelblue" }
            GradientStop { position: 1.0; color: "blue" }
        }
    }
   MouseArea {
         anchors.fill: parent
         property bool state : false
         onClicked: {
             console.log("click " + mouseX + " " + mouseY);
             parent.color = state ? "lightsteelblue" : "steelblue";
             state = !state;
        }
    }

    Text {
        y : 20
        x : 20
        text : "Text Label"
    }

    TextInput {
        y : 50
        x : 20
        text : "Text Input"
    }
}
