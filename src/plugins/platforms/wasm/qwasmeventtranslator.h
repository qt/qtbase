/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QWASMEVENTTRANSLATOR_H
#define QWASMEVENTTRANSLATOR_H

#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>
#include <emscripten/html5.h>
#include "qwasmwindow.h"
#include <QtGui/qinputdevice.h>
#include <QHash>
#include <QCursor>

QT_BEGIN_NAMESPACE

class QWindow;

class QWasmEventTranslator : public QObject
{
    Q_OBJECT

public:

    explicit QWasmEventTranslator();
    ~QWasmEventTranslator();

    template <typename Event>
    QFlags<Qt::KeyboardModifier> translatKeyModifier(const Event *event);

    static Qt::Key translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey);
    QFlags<Qt::KeyboardModifier> translateKeyboardEventModifier(const EmscriptenKeyboardEvent *keyEvent);
    QFlags<Qt::KeyboardModifier> translateMouseEventModifier(const EmscriptenMouseEvent *mouseEvent);
    QFlags<Qt::KeyboardModifier> translateTouchEventModifier(const EmscriptenTouchEvent *touchEvent);
    static Qt::MouseButton translateMouseButton(unsigned short button);
    static QCursor cursorForMode(QWasmCompositor::ResizeMode mode);

    QString getKeyText(const EmscriptenKeyboardEvent *keyEvent, Qt::Key key);
    Qt::Key getKey(const EmscriptenKeyboardEvent *keyEvent);
    void setStickyDeadKey(const EmscriptenKeyboardEvent *keyEvent);

    void setIsMac(bool is_mac) {g_usePlatformMacSpecifics = is_mac;};

Q_SIGNALS:
    void getWindowAt(const QPoint &point, QWindow **window);
private:
    bool g_usePlatformMacSpecifics = false;
    static Qt::Key translateDeadKey(Qt::Key deadKey, Qt::Key accentBaseKey, bool is_mac = false);

private:
    static quint64 getTimestamp();

    Qt::Key m_emDeadKey = Qt::Key_unknown;
    bool m_emStickyDeadKey = false;

};

QT_END_NAMESPACE
#endif // QWASMEVENTTRANSLATOR_H
