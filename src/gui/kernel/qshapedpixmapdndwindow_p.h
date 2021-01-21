/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QSHAPEDPIXMAPDNDWINDOW_H
#define QSHAPEDPIXMAPDNDWINDOW_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/QRasterWindow>
#include <QtGui/QPixmap>

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QShapedPixmapWindow : public QRasterWindow
{
    Q_OBJECT
public:
    explicit QShapedPixmapWindow(QScreen *screen = nullptr);
    ~QShapedPixmapWindow();

    void setUseCompositing(bool on) { m_useCompositing = on; }
    void setPixmap(const QPixmap &pixmap);
    void setHotspot(const QPoint &hotspot);

    void updateGeometry(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QPixmap m_pixmap;
    QPoint m_hotSpot;
    bool m_useCompositing;
};

QT_END_NAMESPACE

#endif // QSHAPEDPIXMAPDNDWINDOW_H
