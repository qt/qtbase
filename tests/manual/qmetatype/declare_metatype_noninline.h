// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DECLARE_METATYPE_NONINLINE_H
#define DECLARE_METATYPE_NONINLINE_H

#include <QtCore/qmetatype.h>

struct ToBeDeclaredMetaTypeNonInline {
    static int triggerRegistration();
};

#endif // DECLARE_METATYPE_NONINLINE_H

