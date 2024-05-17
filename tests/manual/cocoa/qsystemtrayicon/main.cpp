// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtWidgets>

int main(int argc, char**argv)
{
    QApplication app(argc, argv);

    QWidget window;
    window.show();

    QSystemTrayIcon systrayIcon(&window);


    enum Iconset { Square,          // square icons, reccomended size (18 device-independent pixels or less)
                   Rectangular,     // rectangular icons, good size
                   PowerOfTwo,      // standard pow-2 icons, not optimized for the OS X menu bar
                   Small,           // Not enough pixels
                   UnreasonablyLarge  // please do something reasonable with my unreasonably large pixmap
                   };

    // Select icon set and load images
    Iconset iconset = Square;
    QIcon icon;
    switch (iconset) {
        case Square:
            icon.addFile(":/macsystray36x36.png");
            icon.addFile(":/macsystray18x18.png");
        break;
        case Rectangular:
            icon.addFile(":/macsystray50x30.png");
            icon.addFile(":/macsystray25x15.png");
        break;
        case PowerOfTwo:
            icon.addFile(":/macsystray16x16.png");
            icon.addFile(":/macsystray32x32.png");
            icon.addFile(":/macsystray64x64.png");
        break;
        case Small:
            icon.addFile(":/macsystray16x16.png");
        case UnreasonablyLarge:
            icon.addFile(":/macsystray64x64.png");
        break;
    }

    systrayIcon.setIcon(icon);
    systrayIcon.show();

    return app.exec();
}
