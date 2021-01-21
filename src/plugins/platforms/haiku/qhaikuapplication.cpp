/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qhaikuapplication.h"

#include <QCoreApplication>
#include <QFileOpenEvent>

#include <qpa/qwindowsysteminterface.h>

#include <Entry.h>
#include <Path.h>

QHaikuApplication::QHaikuApplication(const char *signature)
    : BApplication(signature)
{
}

bool QHaikuApplication::QuitRequested()
{
    QWindowSystemInterface::handleApplicationTermination<QWindowSystemInterface::SynchronousDelivery>();
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
