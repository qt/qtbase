// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAINPUTCONTEXT_H
#define QCOCOAINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <QtCore/QLocale>
#include <QtCore/QPointer>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class QCocoaInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    explicit QCocoaInputContext();
    ~QCocoaInputContext();

    bool isValid() const override { return true; }

    void setFocusObject(QObject *object) override;

    void commit() override;
    void reset() override;

    QLocale locale() const override { return m_locale; }
    void updateLocale();

private:
    QPointer<QWindow> m_focusWindow;
    QLocale m_locale;
    QMacNotificationObserver m_inputSourceObserver;
};

QT_END_NAMESPACE

#endif // QCOCOAINPUTCONTEXT_H
