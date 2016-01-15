/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QWindow>
#include <QImage>

class Window : public QWindow
{
public:
    Window(QWindow *parent = 0);
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
