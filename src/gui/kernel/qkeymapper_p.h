// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QKEYMAPPER_P_H
#define QKEYMAPPER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <qobject.h>
#include <private/qobject_p.h>
#include <qlist.h>
#include <qlocale.h>
#include <qevent.h>

#include <QtCore/qnativeinterface.h>

QT_BEGIN_NAMESPACE

class QKeyMapperPrivate;
class Q_GUI_EXPORT QKeyMapper : public QObject
{
    Q_OBJECT
public:
    explicit QKeyMapper();
    ~QKeyMapper();

    static QKeyMapper *instance();
    static void changeKeyboard();
    static QList<int> possibleKeys(QKeyEvent *e);

    QT_DECLARE_NATIVE_INTERFACE_ACCESSOR(QKeyMapper)

private:
    friend QKeyMapperPrivate *qt_keymapper_private();
    Q_DECLARE_PRIVATE(QKeyMapper)
    Q_DISABLE_COPY_MOVE(QKeyMapper)
};

struct KeyboardLayoutItem;
class QKeyEvent;

class QKeyMapperPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QKeyMapper)
public:
    QKeyMapperPrivate();
    ~QKeyMapperPrivate();

    QList<int> possibleKeys(QKeyEvent *e);

    QLocale keyboardInputLocale;
    Qt::LayoutDirection keyboardInputDirection;
};

QKeyMapperPrivate *qt_keymapper_private(); // from qkeymapper.cpp

// ----------------- QNativeInterface -----------------

namespace QNativeInterface::Private {

#if QT_CONFIG(evdev) || defined(Q_QDOC)
struct Q_GUI_EXPORT QEvdevKeyMapper
{
    QT_DECLARE_NATIVE_INTERFACE(QEvdevKeyMapper, 1, QKeyMapper)
    virtual void loadKeymap(const QString &filename) = 0;
    virtual void switchLang() = 0;
};
#endif

} // QNativeInterface::Private


QT_END_NAMESPACE

#endif // QKEYMAPPER_P_H
