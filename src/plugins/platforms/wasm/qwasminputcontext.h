/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWASMINPUTCONTEXT_H
#define QWASMINPUTCONTEXT_H


#include <qpa/qplatforminputcontext.h>
#include <QtCore/qpointer.h>
#include <private/qstdweb_p.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>

class QWasmInputContext : public QPlatformInputContext
{
    Q_DISABLE_COPY(QWasmInputContext)
    Q_OBJECT
public:
    explicit QWasmInputContext();
    ~QWasmInputContext() override;

    void update(Qt::InputMethodQueries) override;

    void showInputPanel() override;
    void hideInputPanel() override;
    bool isValid() const override { return true; }

    void focusWindowChanged(QWindow *focusWindow);
    emscripten::val focusCanvas();
    void inputStringChanged(QString &, QWasmInputContext *context);

private:
    bool m_inputPanelVisible = false;

    QPointer<QWindow> m_focusWindow;
    emscripten::val m_inputElement = emscripten::val::null();
    std::unique_ptr<qstdweb::EventCallback> m_blurEventHandler;
    std::unique_ptr<qstdweb::EventCallback> m_inputEventHandler;
    static int androidKeyboardCallback(int eventType,
                                       const EmscriptenKeyboardEvent *keyEvent, void *userData);
    bool inputPanelIsOpen = false;
};

#endif // QWASMINPUTCONTEXT_H
