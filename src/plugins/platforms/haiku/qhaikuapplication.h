// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHAIKUAPPLICATION_H
#define QHAIKUAPPLICATION_H

#include <qglobal.h>

#include <Application.h>

class QHaikuApplication : public BApplication
{
public:
    explicit QHaikuApplication(const char *signature);

    bool QuitRequested() override;
    void RefsReceived(BMessage* message) override;
};

#endif
