/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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
