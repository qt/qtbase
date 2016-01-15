/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QEMULATIONPAINTENGINE_P_H
#define QEMULATIONPAINTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qpaintengineex_p.h>

QT_BEGIN_NAMESPACE


class QEmulationPaintEngine : public QPaintEngineEx
{
public:
    QEmulationPaintEngine(QPaintEngineEx *engine);

    virtual bool begin(QPaintDevice *pdev);
    virtual bool end();

    virtual Type type() const;
    virtual QPainterState *createState(QPainterState *orig) const;

    virtual void fill(const QVectorPath &path, const QBrush &brush);
    virtual void stroke(const QVectorPath &path, const QPen &pen);
    virtual void clip(const QVectorPath &path, Qt::ClipOperation op);

    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    virtual void drawTextItem(const QPointF &p, const QTextItem &textItem);
    virtual void drawStaticTextItem(QStaticTextItem *item);
    virtual void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);
    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr, Qt::ImageConversionFlags flags);

    virtual void clipEnabledChanged();
    virtual void penChanged();
    virtual void brushChanged();
    virtual void brushOriginChanged();
    virtual void opacityChanged();
    virtual void compositionModeChanged();
    virtual void renderHintsChanged();
    virtual void transformChanged();

    virtual void setState(QPainterState *s);

    virtual void beginNativePainting();
    virtual void endNativePainting();

    virtual uint flags() const {return QPaintEngineEx::IsEmulationEngine | QPaintEngineEx::DoNotEmulate;}

    inline QPainterState *state() { return (QPainterState *)QPaintEngine::state; }
    inline const QPainterState *state() const { return (const QPainterState *)QPaintEngine::state; }

    QPaintEngineEx *real_engine;
private:
    void fillBGRect(const QRectF &r);
};

QT_END_NAMESPACE

#endif
