/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QCOCOACOLORDIALOGHELPER_H
#define QCOCOACOLORDIALOGHELPER_H

#include <QtWidgets/qtwidgetsglobal.h>

#include <QObject>
#include <qpa/qplatformdialoghelper.h>

QT_REQUIRE_CONFIG(colordialog);

QT_BEGIN_NAMESPACE

class QCocoaColorDialogHelper : public QPlatformColorDialogHelper
{
public:
    QCocoaColorDialogHelper();
    ~QCocoaColorDialogHelper();

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

    void setCurrentColor(const QColor&) override;
    QColor currentColor() const override;
};

QT_END_NAMESPACE

#endif // QCOCOACOLORDIALOGHELPER_H
