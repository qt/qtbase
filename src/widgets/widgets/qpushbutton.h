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

#ifndef QPUSHBUTTON_H
#define QPUSHBUTTON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractbutton.h>

QT_REQUIRE_CONFIG(pushbutton);

QT_BEGIN_NAMESPACE


class QPushButtonPrivate;
class QMenu;
class QStyleOptionButton;

class Q_WIDGETS_EXPORT QPushButton : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(bool autoDefault READ autoDefault WRITE setAutoDefault)
    Q_PROPERTY(bool default READ isDefault WRITE setDefault)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat)

public:
    explicit QPushButton(QWidget *parent = nullptr);
    explicit QPushButton(const QString &text, QWidget *parent = nullptr);
    QPushButton(const QIcon& icon, const QString &text, QWidget *parent = nullptr);
    ~QPushButton();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    bool autoDefault() const;
    void setAutoDefault(bool);
    bool isDefault() const;
    void setDefault(bool);

#if QT_CONFIG(menu)
    void setMenu(QMenu* menu);
    QMenu* menu() const;
#endif

    void setFlat(bool);
    bool isFlat() const;

public Q_SLOTS:
#if QT_CONFIG(menu)
    void showMenu();
#endif

protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void initStyleOption(QStyleOptionButton *option) const;
    bool hitButton(const QPoint &pos) const override;
    QPushButton(QPushButtonPrivate &dd, QWidget* parent = nullptr);

public:

private:
    Q_DISABLE_COPY(QPushButton)
    Q_DECLARE_PRIVATE(QPushButton)
#if QT_CONFIG(menu)
    Q_PRIVATE_SLOT(d_func(), void _q_popupPressed())
#endif
};

QT_END_NAMESPACE

#endif // QPUSHBUTTON_H
