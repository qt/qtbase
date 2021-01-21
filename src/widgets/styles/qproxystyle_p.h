/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QPROXYSTYLE_P_H
#define QPROXYSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qcommonstyle.h"
#include "qcommonstyle_p.h"
#include "qproxystyle.h"

#ifndef QT_NO_STYLE_PROXY

QT_BEGIN_NAMESPACE

class QProxyStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QProxyStyle)
public:
    void ensureBaseStyle() const;
private:
    QProxyStylePrivate() :
    QCommonStylePrivate(), baseStyle(nullptr) {}
    mutable QPointer <QStyle> baseStyle;
};

QT_END_NAMESPACE

#endif // QT_NO_STYLE_PROXY

#endif //QPROXYSTYLE_P_H
