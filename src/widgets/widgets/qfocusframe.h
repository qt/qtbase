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

#ifndef QFOCUSFRAME_H
#define QFOCUSFRAME_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE


class QFocusFramePrivate;
class QStyleOption;

class Q_WIDGETS_EXPORT QFocusFrame : public QWidget
{
    Q_OBJECT
public:
    QFocusFrame(QWidget *parent = nullptr);
    ~QFocusFrame();

    void setWidget(QWidget *widget);
    QWidget *widget() const;

protected:
    bool event(QEvent *e) override;

    bool eventFilter(QObject *, QEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void initStyleOption(QStyleOption *option) const;

private:
    Q_DECLARE_PRIVATE(QFocusFrame)
    Q_DISABLE_COPY(QFocusFrame)
};

QT_END_NAMESPACE

#endif // QFOCUSFRAME_H
