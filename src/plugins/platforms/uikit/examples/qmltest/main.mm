/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <UIKit/UIKit.h>

#include "qmlapplicationviewer/qmlapplicationviewer.h"

#include <QtGui/QApplication>
#include <QtCore/QtPlugin>
#include <QtDeclarative/QDeclarativeEngine>

Q_IMPORT_PLUGIN(UIKit)

static QString qStringFromNSString(NSString *nsstring)
{
	return QString::fromUtf8([nsstring UTF8String]);
}

static QString documentsDirectory()
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	return qStringFromNSString(documentsDirectory);
}

int main(int argc, char *argv[]) {

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    QApplication app(argc, argv);
    QmlApplicationViewer viewer;
    viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto);
	viewer.engine()->setOfflineStoragePath(documentsDirectory());
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    viewer.setMainQmlFile(qStringFromNSString([resourcePath stringByAppendingPathComponent:@"qml/main.qml"]));
    viewer.showMaximized();
    int retVal = app.exec();
    [pool release];
    return retVal;
}
