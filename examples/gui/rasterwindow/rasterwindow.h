// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RASTERWINDOW_H
#define RASTERWINDOW_H

//! [1]
#include <QtGui>
#include <QScopedPointer>

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
    bool event(QEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    QScopedPointer<QBackingStore> m_backingStore;
};
//! [1]
#endif // RASTERWINDOW_H
