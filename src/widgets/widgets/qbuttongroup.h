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

#ifndef QBUTTONGROUP_H
#define QBUTTONGROUP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(buttongroup);

QT_BEGIN_NAMESPACE

class QAbstractButton;
class QAbstractButtonPrivate;
class QButtonGroupPrivate;

class Q_WIDGETS_EXPORT QButtonGroup : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool exclusive READ exclusive WRITE setExclusive)
public:
    explicit QButtonGroup(QObject *parent = nullptr);
    ~QButtonGroup();

    void setExclusive(bool);
    bool exclusive() const;

    void addButton(QAbstractButton *, int id = -1);
    void removeButton(QAbstractButton *);

    QList<QAbstractButton*> buttons() const;

    QAbstractButton * checkedButton() const;
    // no setter on purpose!

    QAbstractButton *button(int id) const;
    void setId(QAbstractButton *button, int id);
    int id(QAbstractButton *button) const;
    int checkedId() const;

Q_SIGNALS:
    void buttonClicked(QAbstractButton *);
    void buttonPressed(QAbstractButton *);
    void buttonReleased(QAbstractButton *);
    void buttonToggled(QAbstractButton *, bool);
    void idClicked(int);
    void idPressed(int);
    void idReleased(int);
    void idToggled(int, bool);
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_VERSION_X_5_15("Use QButtonGroup::idClicked(int) instead")
    void buttonClicked(int);
    QT_DEPRECATED_VERSION_X_5_15("Use QButtonGroup::idPressed(int) instead")
    void buttonPressed(int);
    QT_DEPRECATED_VERSION_X_5_15("Use QButtonGroup::idReleased(int) instead")
    void buttonReleased(int);
    QT_DEPRECATED_VERSION_X_5_15("Use QButtonGroup::idToggled(int, bool) instead")
    void buttonToggled(int, bool);
#endif

private:
    Q_DISABLE_COPY(QButtonGroup)
    Q_DECLARE_PRIVATE(QButtonGroup)
    friend class QAbstractButton;
    friend class QAbstractButtonPrivate;
};

QT_END_NAMESPACE

#endif // QBUTTONGROUP_H
