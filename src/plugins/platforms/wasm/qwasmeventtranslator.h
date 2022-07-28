// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include "qwasmevent.h"
#include "qwasmplatform.h"

QT_BEGIN_NAMESPACE

class QWindow;

class QWasmEventTranslator : public QObject
{
    Q_OBJECT

public:

    explicit QWasmEventTranslator();
    ~QWasmEventTranslator();

    static Qt::Key translateEmscriptKey(const EmscriptenKeyboardEvent *emscriptKey);
    static QCursor cursorForMode(QWasmCompositor::ResizeMode mode);

    QString getKeyText(const EmscriptenKeyboardEvent *keyEvent, Qt::Key key);
    Qt::Key getKey(const EmscriptenKeyboardEvent *keyEvent);
    void setStickyDeadKey(const EmscriptenKeyboardEvent *keyEvent);

    void setIsMac(bool is_mac) {g_usePlatformMacSpecifics = is_mac;};
    bool g_usePlatformMacSpecifics = false;

Q_SIGNALS:
    void getWindowAt(const QPoint &point, QWindow **window);
private:
    static Qt::Key translateDeadKey(Qt::Key deadKey, Qt::Key accentBaseKey);

private:
    static quint64 getTimestamp();

    Qt::Key m_emDeadKey = Qt::Key_unknown;
    bool m_emStickyDeadKey = false;

};

QT_END_NAMESPACE
#endif // QWASMEVENTTRANSLATOR_H
