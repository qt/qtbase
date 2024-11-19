// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_XML_BUILD_REMOVED_API

#include "qtxmlglobal.h"

QT_USE_NAMESPACE

#if QT_XML_REMOVED_SINCE(6, 9)

#if QT_CONFIG(dom)

#include "qdom.h"

bool QDomNodeList::operator==(const QDomNodeList &other) const
{
    return comparesEqual(*this, other);
}

bool QDomNodeList::operator!=(const QDomNodeList &other) const
{
    return !comparesEqual(*this, other);
}

#endif // QT_CONFIG(dom)

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h
// order sections alphabetically to reduce chances of merge conflicts

#endif // QT_XML_REMOVED_SINCE(6, 9)
