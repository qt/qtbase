// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMDRAG_H
#define QWASMDRAG_H

#include <qpa/qplatformdrag.h>
#include <private/qsimpledrag_p.h>
#include <private/qstdweb_p.h>
#include <QDrag>
#include "qwasmscreen.h"

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QWasmDrag : public QSimpleDrag
{
public:

    QWasmDrag();
    ~QWasmDrag();

    void drop(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods) override;
    void move(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods) override;

    Qt::MouseButton m_qButton;
    QPoint m_mouseDropPoint;
    QFlags<Qt::KeyboardModifier> m_keyModifiers;
    Qt::DropActions m_dropActions;
    QWasmScreen *m_wasmScreen = nullptr;
    int m_mimeTypesCount = 0;
    QMimeData *m_mimeData = nullptr;
    void qWasmDrop();

private:
    void init();
};


QT_END_NAMESPACE

#endif // QWASMDRAG_H
