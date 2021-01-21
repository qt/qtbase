/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QCOCOAVIEWCONTAINER_H
#define QCOCOAVIEWCONTAINER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/QWidget>

Q_FORWARD_DECLARE_OBJC_CLASS(NSView);

QT_BEGIN_NAMESPACE

#if QT_DEPRECATED_SINCE(5, 15)
class QMacCocoaViewContainerPrivate;
class QT_DEPRECATED_X("Use QWindow::fromWinId and QWidget::createWindowContainer instead")
Q_WIDGETS_EXPORT QMacCocoaViewContainer : public QWidget
{
    Q_OBJECT
public:
    QMacCocoaViewContainer(NSView *cocoaViewToWrap, QWidget *parent = nullptr);
    virtual ~QMacCocoaViewContainer();

    void setCocoaView(NSView *view);
    NSView *cocoaView() const;

private:
    Q_DECLARE_PRIVATE(QMacCocoaViewContainer)
};
#endif

QT_END_NAMESPACE

#endif // QCOCOAVIEWCONTAINER_H
