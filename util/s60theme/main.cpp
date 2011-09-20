/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include <QtGui>
#include "s60themeconvert.h"

<<<<<<< HEAD
int help()
=======
#ifndef QT_NO_ACCESSIBILITY

#include "qaccessible.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAccessiblePlugin
    \brief The QAccessiblePlugin class provides an abstract base for
    accessibility plugins.

    \ingroup plugins
    \ingroup accessibility
    \inmodule QtWidgets

    Writing an accessibility plugin is achieved by subclassing this
    base class, reimplementing the pure virtual functions keys() and
    create(), and exporting the class with the Q_EXPORT_PLUGIN2()
    macro.

    \sa QAccessibleBridgePlugin, {How to Create Qt Plugins}
*/

/*!
    Constructs an accessibility plugin with the given \a parent. This
    is invoked automatically by the Q_EXPORT_PLUGIN2() macro.
*/
QAccessiblePlugin::QAccessiblePlugin(QObject *parent)
    : QObject(parent)
>>>>>>> Move the documentation for the classes to their modules.
{
    qDebug() << "Usage: s60theme [modeldir|theme.tdf] output.blob";
    qDebug() << "";
    qDebug() << "Options:";
    qDebug() << "   modeldir:    Theme 'model' directory in Carbide.ui tree";
    qDebug() << "   theme.tdf:   Theme .tdf file";
    qDebug() << "   output.blob: Theme blob output filename";
    qDebug() << "";
    qDebug() << "s60theme takes an S60 theme from Carbide.ui and converts";
    qDebug() << "it into a compact, binary format, that can be directly loaded by";
    qDebug() << "the QtS60Style.";
    qDebug() << "";
    qDebug() << "Visit http://www.forum.nokia.com for details about Carbide.ui";
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
        return help();

    QApplication app(argc, argv);

    const QString input = QString::fromLatin1(argv[1]);
    const QFileInfo inputInfo(input);
    const QString output = QString::fromLatin1(argv[2]);
    if (inputInfo.isDir())
        return S60ThemeConvert::convertDefaultThemeToBlob(input, output) ? 0 : 1;
    else if (inputInfo.suffix().compare(QString::fromLatin1("tdf"), Qt::CaseInsensitive) == 0)
        return S60ThemeConvert::convertTdfToBlob(input, output) ? 0 : 1;

    return help();
}
