/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformtheme_qpa.h"

#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformTheme
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformTheme class allows customizing the UI based on themes.

*/

/*!
    \enum QPlatformTheme::ThemeHint

    This enum describes the available theme hints.

    \value TextCursorWidth  (int) Determines the width of the text cursor.

    \value DropShadow       (bool) Determines whether the drop shadow effect for
                            tooltips or whatsthis is enabled.

    \value MaximumScrollBarDragDistance (int) Determines the value returned by
                            QStyle::pixelMetric(PM_MaximumDragDistance)

    \sa themeHint(), QStyle::pixelMetric()
*/

QPlatformMenu *QPlatformTheme::createPlatformMenu(QMenu *menu) const
{
    Q_UNUSED(menu);
    return 0;
}

QPlatformMenuBar *QPlatformTheme::createPlatformMenuBar(QMenuBar *menuBar) const
{
    Q_UNUSED(menuBar);
    return 0;
}

bool QPlatformTheme::usePlatformNativeDialog(DialogType type) const
{
    Q_UNUSED(type);
    return false;
}

QPlatformDialogHelper *QPlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    Q_UNUSED(type);
    return 0;
}

QVariant QPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case TextCursorWidth:
        return QVariant(1);
    case DropShadow:
        return QVariant(false);
    case MaximumScrollBarDragDistance:
        return QVariant(-1);
    }
    return QVariant();
}

QT_END_NAMESPACE
