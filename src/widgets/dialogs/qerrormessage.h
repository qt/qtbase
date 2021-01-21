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

#ifndef QERRORMESSAGE_H
#define QERRORMESSAGE_H

#include <QtWidgets/qtwidgetsglobal.h>

#include <QtWidgets/qdialog.h>

QT_REQUIRE_CONFIG(errormessage);

QT_BEGIN_NAMESPACE

class QErrorMessagePrivate;

class Q_WIDGETS_EXPORT QErrorMessage: public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QErrorMessage)
public:
    explicit QErrorMessage(QWidget* parent = nullptr);
    ~QErrorMessage();

    static QErrorMessage * qtHandler();

public Q_SLOTS:
    void showMessage(const QString &message);
    void showMessage(const QString &message, const QString &type);

protected:
    void done(int) override;
    void changeEvent(QEvent *e) override;

private:
    Q_DISABLE_COPY(QErrorMessage)
};

QT_END_NAMESPACE

#endif // QERRORMESSAGE_H
