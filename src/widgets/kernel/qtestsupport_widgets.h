/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QTESTSUPPORT_WIDGETS_H
#define QTESTSUPPORT_WIDGETS_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtGui/qtestsupport_gui.h>

QT_BEGIN_NAMESPACE

class QPointingDevice;
class QWidget;

namespace QTest {

[[nodiscard]] Q_WIDGETS_EXPORT bool qWaitForWindowActive(QWidget *widget, int timeout = 5000);
[[nodiscard]] Q_WIDGETS_EXPORT bool qWaitForWindowExposed(QWidget *widget, int timeout = 5000);

class Q_WIDGETS_EXPORT QTouchEventWidgetSequence : public QTouchEventSequence
{
public:
    ~QTouchEventWidgetSequence() override;
    QTouchEventWidgetSequence& press(int touchId, const QPoint &pt, QWidget *widget = nullptr);
    QTouchEventWidgetSequence& move(int touchId, const QPoint &pt, QWidget *widget = nullptr);
    QTouchEventWidgetSequence& release(int touchId, const QPoint &pt, QWidget *widget = nullptr);
    QTouchEventWidgetSequence& stationary(int touchId) override;

    void commit(bool processEvents = true) override;

private:
    QTouchEventWidgetSequence(QWidget *widget, QPointingDevice *aDevice, bool autoCommit);

    QPoint mapToScreen(QWidget *widget, const QPoint &pt);

    QWidget *targetWidget = nullptr;

    friend QTouchEventWidgetSequence touchEvent(QWidget *widget, QPointingDevice *device, bool autoCommit);
};

} // namespace QTest

QT_END_NAMESPACE

#endif
