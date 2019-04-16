/****************************************************************************
**
** Copyright (C) 2019 Volker Krause <vkrause@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "androidcontentfileengine.h"

#include <private/qjni_p.h>
#include <private/qjnihelpers_p.h>

#include <QDebug>

AndroidContentFileEngine::AndroidContentFileEngine(const QString &fileName)
    : QFSFileEngine(fileName)
{
}

bool AndroidContentFileEngine::open(QIODevice::OpenMode openMode)
{
    QString openModeStr;
    if (openMode & QFileDevice::ReadOnly) {
        openModeStr += QLatin1Char('r');
    }
    if (openMode & QFileDevice::WriteOnly) {
        openModeStr += QLatin1Char('w');
    }
    if (openMode & QFileDevice::Truncate) {
        openModeStr += QLatin1Char('t');
    } else if (openMode & QFileDevice::Append) {
        openModeStr += QLatin1Char('a');
    }

    const auto fd = QJNIObjectPrivate::callStaticMethod<jint>("org/qtproject/qt5/android/QtNative",
        "openFdForContentUrl",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",
        QtAndroidPrivate::context(),
        QJNIObjectPrivate::fromString(fileName(DefaultName)).object(),
        QJNIObjectPrivate::fromString(openModeStr).object());

    if (fd < 0) {
        return false;
    }

    return QFSFileEngine::open(openMode, fd, QFile::AutoCloseHandle);
}


AndroidContentFileEngineHandler::AndroidContentFileEngineHandler() = default;
AndroidContentFileEngineHandler::~AndroidContentFileEngineHandler() = default;

QAbstractFileEngine* AndroidContentFileEngineHandler::create(const QString &fileName) const
{
    if (!fileName.startsWith(QLatin1String("content"))) {
        return nullptr;
    }

    return new AndroidContentFileEngine(fileName);
}
