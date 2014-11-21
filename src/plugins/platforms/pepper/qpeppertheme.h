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

#ifndef QPEPPERTHEME_H
#define QPEPPERTHEME_H

#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QPepperTheme : public QPlatformTheme
{
public:
    QPepperTheme();
    ~QPepperTheme();

    QVariant themeHint(ThemeHint hint) const;
};

QT_END_NAMESPACE

#endif
