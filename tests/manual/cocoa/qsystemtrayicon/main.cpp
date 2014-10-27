/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
