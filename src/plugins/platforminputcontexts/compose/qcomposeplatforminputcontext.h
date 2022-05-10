// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QCOMPOSEPLATFORMINPUTCONTEXT_H
#define QCOMPOSEPLATFORMINPUTCONTEXT_H

#include <QtCore/QLoggingCategory>

#include <qpa/qplatforminputcontext.h>

#include <xkbcommon/xkbcommon-compose.h>

Q_DECLARE_OPAQUE_POINTER(xkb_context *)
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
    xkb_compose_table *m_composeTable = nullptr;
    xkb_compose_state *m_composeState = nullptr;
    QObject *m_focusObject = nullptr;
    struct xkb_context *m_XkbContext = nullptr;
};

QT_END_NAMESPACE

#endif // QCOMPOSEPLATFORMINPUTCONTEXT_H
