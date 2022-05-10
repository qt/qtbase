// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Rick Stockton <rickstockton@reno-computerhelp.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef BUTTONTESTER_H
#define BUTTONTESTER_H

#include <QTextEdit>
#include <QString>
#include <QMouseEvent>
#include <QWheelEvent>

class ButtonTester : public QTextEdit
{
    Q_OBJECT
public:
    using QTextEdit::QTextEdit;
protected:
    void    mousePressEvent(QMouseEvent *event) override;
    void    mouseReleaseEvent(QMouseEvent *event) override;
    void    mouseDoubleClickEvent(QMouseEvent *event) override;
#if QT_CONFIG(wheelevent)
    void    wheelEvent(QWheelEvent *event) override;
#endif
    int     buttonByNumber(const Qt::MouseButton button);
    QString enumNameFromValue(const Qt::MouseButton button);
    QString enumNamesFromMouseButtons(const Qt::MouseButtons buttons);
};

#endif // BUTTONTESTER_H
