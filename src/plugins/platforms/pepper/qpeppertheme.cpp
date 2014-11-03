/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpeppertheme.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE


QPepperTheme::QPepperTheme()
{

}

QPepperTheme::~QPepperTheme()
{

}
QVariant QPepperTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("cleanlooks"));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}


QT_END_NAMESPACE
