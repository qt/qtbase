// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
