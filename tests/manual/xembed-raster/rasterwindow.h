// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RASTERWINDOW_H
#define RASTERWINDOW_H

#include <QtGui>

class RasterWindow : public QWindow
{
    Q_OBJECT
public:
    explicit RasterWindow(QWindow *parent = nullptr);

    virtual void render(QPainter *painter);

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event);

    void resizeEvent(QResizeEvent *event);
    void exposeEvent(QExposeEvent *event);

private:
    QBackingStore *m_backingStore;
    bool m_update_pending;
};

#endif // RASTERWINDOW_H
