// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QWindow>
#include <QImage>

class Window : public QWindow
{
public:
    Window(QWindow *parent = nullptr);
    Window(QScreen *screen);

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *event);

    void keyPressEvent(QKeyEvent *);

    void exposeEvent(QExposeEvent *);
    void resizeEvent(QResizeEvent *);

    void timerEvent(QTimerEvent *);

private:
    void render();
    void scheduleRender();
    void initialize();

    QString m_text;
    QImage m_image;
    QPoint m_lastPos;
    int m_backgroundColorIndex;
    QBackingStore *m_backingStore;
    int m_renderTimer;
    QPoint scrollOffset;
};
