/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#include "qhaikuapplication.h"

#include <QCoreApplication>
#include <QFileOpenEvent>

#include <Entry.h>
#include <Path.h>

QHaikuApplication::QHaikuApplication(const char *signature)
    : BApplication(signature)
{
}

bool QHaikuApplication::QuitRequested()
{
    QEvent quitEvent(QEvent::Quit);
    QCoreApplication::sendEvent(QCoreApplication::instance(), &quitEvent);
    return true;
}

void QHaikuApplication::RefsReceived(BMessage* message)
{
    uint32 type;
    int32 count;

    const status_t status = message->GetInfo("refs", &type, &count);
    if (status == B_OK && type == B_REF_TYPE) {
        entry_ref ref;
        for (int32 i = 0; i < count; ++i) {
            if (message->FindRef("refs", i, &ref) == B_OK) {
                const BPath path(&ref);
                QCoreApplication::postEvent(QCoreApplication::instance(), new QFileOpenEvent(QFile::decodeName(path.Path())));
            }
        }
    }

    BApplication::RefsReceived(message);
}
