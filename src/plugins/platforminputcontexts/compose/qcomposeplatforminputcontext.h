/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
