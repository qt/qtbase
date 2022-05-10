// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "declare_metatype_noninline.h"

Q_DECLARE_METATYPE(ToBeDeclaredMetaTypeNonInline)

int ToBeDeclaredMetaTypeNonInline::triggerRegistration()
{
    return qMetaTypeId<ToBeDeclaredMetaTypeNonInline>();
}
