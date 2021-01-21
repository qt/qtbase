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

#ifndef QSCROLLAREA_H
#define QSCROLLAREA_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractscrollarea.h>

QT_REQUIRE_CONFIG(scrollarea);

QT_BEGIN_NAMESPACE

class QScrollAreaPrivate;

class Q_WIDGETS_EXPORT QScrollArea : public QAbstractScrollArea
{
    Q_OBJECT
    Q_PROPERTY(bool widgetResizable READ widgetResizable WRITE setWidgetResizable)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)

public:
    explicit QScrollArea(QWidget *parent = nullptr);
    ~QScrollArea();

    QWidget *widget() const;
    void setWidget(QWidget *widget);
    QWidget *takeWidget();

    bool widgetResizable() const;
    void setWidgetResizable(bool resizable);

    QSize sizeHint() const override;

    bool focusNextPrevChild(bool next) override;

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment);

    void ensureVisible(int x, int y, int xmargin = 50, int ymargin = 50);
    void ensureWidgetVisible(QWidget *childWidget, int xmargin = 50, int ymargin = 50);

protected:
    QScrollArea(QScrollAreaPrivate &dd, QWidget *parent = nullptr);
    bool event(QEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void scrollContentsBy(int dx, int dy) override;

    QSize viewportSizeHint() const override;

private:
    Q_DECLARE_PRIVATE(QScrollArea)
    Q_DISABLE_COPY(QScrollArea)
};

QT_END_NAMESPACE

#endif // QSCROLLAREA_H
