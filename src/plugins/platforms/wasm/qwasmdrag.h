/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
