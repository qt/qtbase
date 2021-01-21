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

#ifndef QCHECKBOX_H
#define QCHECKBOX_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractbutton.h>

QT_REQUIRE_CONFIG(checkbox);

QT_BEGIN_NAMESPACE


class QCheckBoxPrivate;
class QStyleOptionButton;

class Q_WIDGETS_EXPORT QCheckBox : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate)

public:
    explicit QCheckBox(QWidget *parent = nullptr);
    explicit QCheckBox(const QString &text, QWidget *parent = nullptr);
    ~QCheckBox();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setTristate(bool y = true);
    bool isTristate() const;

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

Q_SIGNALS:
    void stateChanged(int);

protected:
    bool event(QEvent *e) override;
    bool hitButton(const QPoint &pos) const override;
    void checkStateSet() override;
    void nextCheckState() override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void initStyleOption(QStyleOptionButton *option) const;


private:
    Q_DECLARE_PRIVATE(QCheckBox)
    Q_DISABLE_COPY(QCheckBox)
    friend class QAccessibleButton;
};

QT_END_NAMESPACE

#endif // QCHECKBOX_H
