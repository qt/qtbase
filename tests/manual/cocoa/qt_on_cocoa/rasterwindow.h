// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QRasterWindow>
#include <QImage>

class RasterWindow : public QRasterWindow
{
public:
    RasterWindow(QRasterWindow *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    void keyPressEvent(QKeyEvent *);

    void exposeEvent(QExposeEvent *);
    void resizeEvent(QResizeEvent *);

    bool event(QEvent *);

private:
    void render();
    void scheduleRender();
    void initialize();

    QString m_text;
    QImage m_image;
    QPoint m_lastPos;
    int m_backgroundColorIndex;
    QBackingStore *m_backingStore;
};
