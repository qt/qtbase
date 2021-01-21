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

#ifndef QSIZEGRIP_H
#define QSIZEGRIP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

QT_REQUIRE_CONFIG(sizegrip);

QT_BEGIN_NAMESPACE

class QSizeGripPrivate;
class Q_WIDGETS_EXPORT QSizeGrip : public QWidget
{
    Q_OBJECT
public:
    explicit QSizeGrip(QWidget *parent);
    ~QSizeGrip();

    QSize sizeHint() const override;
    void setVisible(bool) override;

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *mouseEvent) override;
    void moveEvent(QMoveEvent *moveEvent) override;
    void showEvent(QShowEvent *showEvent) override;
    void hideEvent(QHideEvent *hideEvent) override;
    bool eventFilter(QObject *, QEvent *) override;
    bool event(QEvent *) override;

public:

private:
    Q_DECLARE_PRIVATE(QSizeGrip)
    Q_DISABLE_COPY(QSizeGrip)
    Q_PRIVATE_SLOT(d_func(), void _q_showIfNotHidden())
};

QT_END_NAMESPACE

#endif // QSIZEGRIP_H
