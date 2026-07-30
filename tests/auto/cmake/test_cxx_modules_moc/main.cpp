// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>

#include "helper.h"

#include <cstdio>

import Mod;
import HelperMod;

class Receiver : public QObject
{
    Q_OBJECT

public slots:
    void onSignal() { std::puts("received signal"); }
};

int main()
{
    PrimaryObject primary;
    PartObject part;
    Receiver receiver;

    QObject::connect(&primary, &PrimaryObject::primarySignal, &receiver, &Receiver::onSignal);
    QObject::connect(&part, &PartObject::partSignal, &receiver, &Receiver::onSignal);

    // HeaderHelper and ModuleHelper aren't re-exported by Mod, so even though PrimaryObject
    // and PartObject are visible here via "import Mod;", we still need to make these
    // parameter types visible ourselves (via #include / import) in order to name them.
    emit primary.primarySignal(HeaderHelper{42});
    emit part.partSignal(ModuleHelper{43});
}

#include "main.moc"
