/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QCOCOAINPUTCONTEXT_H
#define QCOCOAINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <QtCore/QLocale>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

class QCocoaInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    explicit QCocoaInputContext();
    ~QCocoaInputContext();

    bool isValid() const override { return true; }

    void reset() override;

    QLocale locale() const override { return m_locale; }
    void updateLocale();

private Q_SLOTS:
    void connectSignals();
    void focusObjectChanged(QObject *focusObject);

private:
    QPointer<QWindow> mWindow;
    QLocale m_locale;
};

QT_END_NAMESPACE

#endif // QCOCOAINPUTCONTEXT_H
