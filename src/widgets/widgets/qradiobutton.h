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

#ifndef QRADIOBUTTON_H
#define QRADIOBUTTON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractbutton.h>

QT_REQUIRE_CONFIG(radiobutton);

QT_BEGIN_NAMESPACE


class QRadioButtonPrivate;
class QStyleOptionButton;

class Q_WIDGETS_EXPORT QRadioButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit QRadioButton(QWidget *parent = nullptr);
    explicit QRadioButton(const QString &text, QWidget *parent = nullptr);
    ~QRadioButton();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool event(QEvent *e) override;
    bool hitButton(const QPoint &) const override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void initStyleOption(QStyleOptionButton *button) const;


private:
    Q_DECLARE_PRIVATE(QRadioButton)
    Q_DISABLE_COPY(QRadioButton)
    friend class QAccessibleButton;
};

QT_END_NAMESPACE

#endif // QRADIOBUTTON_H
