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

#ifndef QSTACKEDLAYOUT_H
#define QSTACKEDLAYOUT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qlayout.h>

QT_BEGIN_NAMESPACE


class QStackedLayoutPrivate;

class Q_WIDGETS_EXPORT QStackedLayout : public QLayout
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QStackedLayout)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
    Q_PROPERTY(StackingMode stackingMode READ stackingMode WRITE setStackingMode)
    QDOC_PROPERTY(int count READ count)

public:
    enum StackingMode {
        StackOne,
        StackAll
    };
    Q_ENUM(StackingMode)

    QStackedLayout();
    explicit QStackedLayout(QWidget *parent);
    explicit QStackedLayout(QLayout *parentLayout);
    ~QStackedLayout();

    int addWidget(QWidget *w);
    int insertWidget(int index, QWidget *w);

    QWidget *currentWidget() const;
    int currentIndex() const;
    using QLayout::widget;
    QWidget *widget(int) const;
    int count() const override;

    StackingMode stackingMode() const;
    void setStackingMode(StackingMode stackingMode);

    // abstract virtual functions:
    void addItem(QLayoutItem *item) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QLayoutItem *itemAt(int) const override;
    QLayoutItem *takeAt(int) override;
    void setGeometry(const QRect &rect) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;

Q_SIGNALS:
    void widgetRemoved(int index);
    void currentChanged(int index);

public Q_SLOTS:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget *w);

private:
    Q_DISABLE_COPY(QStackedLayout)
};

QT_END_NAMESPACE

#endif // QSTACKEDLAYOUT_H
