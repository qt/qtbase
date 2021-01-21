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

#ifndef QMACNATIVEWIDGET_H
#define QMACNATIVEWIDGET_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/QWidget>

Q_FORWARD_DECLARE_OBJC_CLASS(NSView);

QT_BEGIN_NAMESPACE

#if QT_DEPRECATED_SINCE(5, 15)
class QT_DEPRECATED_X("Use QWidget::winId instead")
Q_WIDGETS_EXPORT QMacNativeWidget : public QWidget
{
    Q_OBJECT
public:
    QMacNativeWidget(NSView *parentView = nullptr);
    ~QMacNativeWidget();

    QSize sizeHint() const override;
    NSView *nativeView() const;

protected:
    bool event(QEvent *ev) override;
};
#endif

QT_END_NAMESPACE

#endif // QMACNATIVEWIDGET_H
