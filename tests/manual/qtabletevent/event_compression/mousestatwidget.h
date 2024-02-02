// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MOUSESTATWIDGET_H
#define MOUSESTATWIDGET_H

#include <QWidget>

class QTabletEvent;
class QMouseEvent;
class QTimerEvent;
class QPaintEvent;

class MouseStatWidget : public QWidget
{
public:
    MouseStatWidget(bool acceptTabletEvent = true);
protected:
    void tabletEvent(QTabletEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void timerEvent(QTimerEvent *);
    void paintEvent(QPaintEvent *);
private:
    const bool acceptTabletEvent;
    int receivedMouseEventCount;
    int receivedMouseEventCountToPaint;
    int receivedTabletEventCount;
    int receivedTabletEventCountToPaint;
};

#endif // MOUSESTATWIDGET_H
