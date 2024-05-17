// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore>

#include "declare_metatype_noninline.h"

int main(int argc, char **argv)
{
    qDebug("expect no warning here");
    const int id = qRegisterMetaType<ToBeDeclaredMetaTypeNonInline>("ToBeDeclaredMetaTypeNonInline");
    qDebug("expect a qfatal now");
    const int result = ToBeDeclaredMetaTypeNonInline::triggerRegistration();
    Q_ASSERT(id == result);
    qDebug("if you read this, check that a qFatal was seen, "
           "otherwise the test failed.");
}
