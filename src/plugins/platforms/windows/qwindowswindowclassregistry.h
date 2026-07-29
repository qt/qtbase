// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSWINDOWCLASSREGISTRY_H
#define QWINDOWSWINDOWCLASSREGISTRY_H

#include "qtwindowsglobal.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaWindowClass)

class QWindow;
struct QWindowsWindowClassDescription;

class QWindowsWindowClassRegistry
{
    Q_DISABLE_COPY_MOVE(QWindowsWindowClassRegistry)
public:
    QWindowsWindowClassRegistry(WNDPROC defaultProcedure);
    ~QWindowsWindowClassRegistry();

    static QWindowsWindowClassRegistry *instance();

    QString registerWindowClass(const QWindowsWindowClassDescription &description);
    QString registerWindowClass(const QWindow *window);
    QString registerWindowClass(QString name, WNDPROC procedure);

private:
    static QString classNamePrefix();

    bool shouldDecorateWindowClassName(const QString &name, WNDPROC procedure) const;
    void unregisterWindowClasses();

    static QWindowsWindowClassRegistry *m_instance;

    WNDPROC m_defaultProcedure;
    QSet<QString> m_registeredWindowClassNames;
};

QT_END_NAMESPACE

#endif // QWINDOWSWINDOWCLASSREGISTRY_H
