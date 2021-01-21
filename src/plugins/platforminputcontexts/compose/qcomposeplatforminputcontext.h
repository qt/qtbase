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
#ifndef QCOMPOSEPLATFORMINPUTCONTEXT_H
#define QCOMPOSEPLATFORMINPUTCONTEXT_H

#include <QtCore/QLoggingCategory>

#include <qpa/qplatforminputcontext.h>

#include <xkbcommon/xkbcommon-compose.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcXkbCompose)

class QEvent;

class QComposeInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    QComposeInputContext();
    ~QComposeInputContext();

    bool isValid() const override;
    void setFocusObject(QObject *object) override;
    void reset() override;
    void update(Qt::InputMethodQueries) override;

    bool filterEvent(const QEvent *event) override;

    // This invokable is called from QXkbCommon::setXkbContext().
    Q_INVOKABLE void setXkbContext(struct xkb_context *context) { m_XkbContext = context; }

protected:
    void ensureInitialized();

private:
    bool m_initialized = false;
    xkb_context *m_context = nullptr;
    xkb_compose_table *m_composeTable = nullptr;
    xkb_compose_state *m_composeState = nullptr;
    QObject *m_focusObject = nullptr;
    struct xkb_context *m_XkbContext = nullptr;
};

QT_END_NAMESPACE

#endif // QCOMPOSEPLATFORMINPUTCONTEXT_H
