// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWS11STYLE_P_H
#define QWINDOWS11STYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <qwindowsvistastyle_p_p.h>

QT_BEGIN_NAMESPACE

class QWindows11StylePrivate;
class QWindows11Style;

class QWindows11Style : public QWindowsVistaStyle
{
    Q_OBJECT
public:
    QWindows11Style();
    ~QWindows11Style() override;

    void polish(QWidget* widget) override;
protected:
    QWindows11Style(QWindows11StylePrivate &dd);
private:
    Q_DISABLE_COPY_MOVE(QWindows11Style)
    Q_DECLARE_PRIVATE(QWindows11Style)
    friend class QStyleFactory;

    const QFont assetFont = QFont("Segoe Fluent Icons"); //Font to load icons from
};

class QWindows11StylePrivate : public QWindowsVistaStylePrivate {
    Q_DECLARE_PUBLIC(QWindows11Style)
};

QT_END_NAMESPACE

#endif // QWINDOWS11STYLE_P_H
