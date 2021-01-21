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

#ifndef QCOMMANDLINKBUTTON_H
#define QCOMMANDLINKBUTTON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qpushbutton.h>

QT_REQUIRE_CONFIG(commandlinkbutton);

QT_BEGIN_NAMESPACE


class QCommandLinkButtonPrivate;

class Q_WIDGETS_EXPORT QCommandLinkButton: public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat DESIGNABLE false)

public:
    explicit QCommandLinkButton(QWidget *parent = nullptr);
    explicit QCommandLinkButton(const QString &text, QWidget *parent = nullptr);
    explicit QCommandLinkButton(const QString &text, const QString &description, QWidget *parent = nullptr);
    ~QCommandLinkButton();

    QString description() const;
    void setDescription(const QString &description);

    // QTBUG-68722
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
protected:
#else
public:
#endif
    QSize sizeHint() const override;
    int heightForWidth(int) const override;
    QSize minimumSizeHint() const override;
protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;

private:
    Q_DISABLE_COPY(QCommandLinkButton)
    Q_DECLARE_PRIVATE(QCommandLinkButton)
};

QT_END_NAMESPACE

#endif // QCOMMANDLINKBUTTON
