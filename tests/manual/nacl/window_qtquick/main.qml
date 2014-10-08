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
}
