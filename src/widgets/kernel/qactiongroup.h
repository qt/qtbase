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

#ifndef QACTIONGROUP_H
#define QACTIONGROUP_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qaction.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_ACTION

class QActionGroupPrivate;

class Q_WIDGETS_EXPORT QActionGroup : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QActionGroup)

    Q_PROPERTY(QActionGroup::ExclusionPolicy exclusionPolicy READ exclusionPolicy WRITE setExclusionPolicy)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible)

public:
    enum class ExclusionPolicy {
        None,
        Exclusive,
        ExclusiveOptional
    };
    Q_ENUM(ExclusionPolicy)

    explicit QActionGroup(QObject* parent);
    ~QActionGroup();

    QAction *addAction(QAction* a);
    QAction *addAction(const QString &text);
    QAction *addAction(const QIcon &icon, const QString &text);
    void removeAction(QAction *a);
    QList<QAction*> actions() const;

    QAction *checkedAction() const;
    bool isExclusive() const;
    bool isEnabled() const;
    bool isVisible() const;
    ExclusionPolicy exclusionPolicy() const;


public Q_SLOTS:
    void setEnabled(bool);
    inline void setDisabled(bool b) { setEnabled(!b); }
    void setVisible(bool);
    void setExclusive(bool);
    void setExclusionPolicy(ExclusionPolicy policy);

Q_SIGNALS:
    void triggered(QAction *);
    void hovered(QAction *);

private:
    Q_DISABLE_COPY(QActionGroup)
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())
    Q_PRIVATE_SLOT(d_func(), void _q_actionChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_actionHovered())
};

#endif // QT_NO_ACTION

QT_END_NAMESPACE

#endif // QACTIONGROUP_H
