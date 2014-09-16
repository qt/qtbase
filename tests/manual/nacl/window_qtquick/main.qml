import QtQuick 1.0

Rectangle {
     y: 0; width: 80; height: 80
     color: "lightsteelblue"
 }

 Rectangle {
     y: 100; width: 80; height: 80
     gradient: Gradient {
         GradientStop { position: 0.0; color: "lightsteelblue" }
         GradientStop { position: 1.0; color: "blue" }
     }
 }

 Rectangle {
     y: 200; width: 80; height: 80
     rotation: 90
     gradient: Gradient {
         GradientStop { position: 0.0; color: "lightsteelblue" }
         GradientStop { position: 1.0; color: "blue" }
     }
 }