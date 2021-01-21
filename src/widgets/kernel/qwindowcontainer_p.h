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

#ifndef QWINDOWCONTAINER_H
#define QWINDOWCONTAINER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QWindowContainerPrivate;

class Q_WIDGETS_EXPORT QWindowContainer : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWindowContainer)

public:
    explicit QWindowContainer(QWindow *embeddedWindow, QWidget *parent = nullptr, Qt::WindowFlags f = { });
    ~QWindowContainer();
    QWindow *containedWindow() const;

    static void toplevelAboutToBeDestroyed(QWidget *parent);
    static void parentWasChanged(QWidget *parent);
    static void parentWasMoved(QWidget *parent);
    static void parentWasRaised(QWidget *parent);
    static void parentWasLowered(QWidget *parent);

protected:
    bool event(QEvent *ev) override;

private slots:
    void focusWindowChanged(QWindow *focusWindow);
};

QT_END_NAMESPACE

#endif // QWINDOWCONTAINER_H
