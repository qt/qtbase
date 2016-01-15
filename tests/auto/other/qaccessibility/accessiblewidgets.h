/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef ACCESSIBLEWIDGETS_H
#define ACCESSIBLEWIDGETS_H

#include <QtWidgets/qaccessiblewidget.h>
#include <QtWidgets/qpushbutton.h>

class QtTestAccessibleWidget: public QWidget
{
    Q_OBJECT
public:
    QtTestAccessibleWidget(QWidget *parent, const char *name): QWidget(parent)
    {
        setObjectName(name);
    }
};

class QtTestAccessibleWidgetIface: public QAccessibleWidget
{
public:
    QtTestAccessibleWidgetIface(QtTestAccessibleWidget *w): QAccessibleWidget(w) {}
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE
    {
        if (t == QAccessible::Help)
            return QString::fromLatin1("Help yourself");
        return QAccessibleWidget::text(t);
    }
    static QAccessibleInterface *ifaceFactory(const QString &key, QObject *o)
    {
        if (key == "QtTestAccessibleWidget")
            return new QtTestAccessibleWidgetIface(static_cast<QtTestAccessibleWidget*>(o));
        return 0;
    }
};

class QtTestAccessibleWidgetSubclass: public QtTestAccessibleWidget
{
    Q_OBJECT
public:
    QtTestAccessibleWidgetSubclass(QWidget *parent, const char *name): QtTestAccessibleWidget(parent, name)
    {}
};


class KFooButton: public QPushButton
{
    Q_OBJECT
public:
    KFooButton(const QString &text, QWidget* parent = 0)
        : QPushButton(text, parent)
    {}
};


class CustomTextWidget : public QWidget
{
    Q_OBJECT
public:
    int cursorPosition;
    QString text;
};

class CustomTextWidgetIface: public QAccessibleWidget, public QAccessibleTextInterface
{
public:
    static QAccessibleInterface *ifaceFactory(const QString &key, QObject *o)
    {
        if (key == "CustomTextWidget")
            return new CustomTextWidgetIface(static_cast<CustomTextWidget*>(o));
        return 0;
    }
    CustomTextWidgetIface(CustomTextWidget *w): QAccessibleWidget(w) {}
    void *interface_cast(QAccessible::InterfaceType t) {
        if (t == QAccessible::TextInterface)
            return static_cast<QAccessibleTextInterface*>(this);
        return 0;
    }

    // this is mostly to test the base implementation for textBefore/At/After
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE
    {
        if (t == QAccessible::Value)
            return textWidget()->text;
        return QAccessibleWidget::text(t);
    }

    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
    {
        if (offset == -2)
            offset = textWidget()->cursorPosition;
        return QAccessibleTextInterface::textBeforeOffset(offset, boundaryType, startOffset, endOffset);
    }
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
    {
        if (offset == -2)
            offset = textWidget()->cursorPosition;
        return QAccessibleTextInterface::textAtOffset(offset, boundaryType, startOffset, endOffset);
    }
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
    {
        if (offset == -2)
            offset = textWidget()->cursorPosition;
        return QAccessibleTextInterface::textAfterOffset(offset, boundaryType, startOffset, endOffset);
    }

    void selection(int, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE
    { *startOffset = *endOffset = -1; }
    int selectionCount() const Q_DECL_OVERRIDE { return 0; }
    void addSelection(int, int) Q_DECL_OVERRIDE {}
    void removeSelection(int) Q_DECL_OVERRIDE {}
    void setSelection(int, int, int) Q_DECL_OVERRIDE {}
    int cursorPosition() const Q_DECL_OVERRIDE { return textWidget()->cursorPosition; }
    void setCursorPosition(int position) Q_DECL_OVERRIDE { textWidget()->cursorPosition = position; }
    QString text(int startOffset, int endOffset) const Q_DECL_OVERRIDE { return textWidget()->text.mid(startOffset, endOffset); }
    int characterCount() const Q_DECL_OVERRIDE { return textWidget()->text.length(); }
    QRect characterRect(int) const Q_DECL_OVERRIDE { return QRect(); }
    int offsetAtPoint(const QPoint &) const Q_DECL_OVERRIDE { return 0; }
    void scrollToSubstring(int, int) Q_DECL_OVERRIDE {}
    QString attributes(int, int *, int *) const Q_DECL_OVERRIDE
    { return QString(); }

private:
    CustomTextWidget *textWidget() const { return qobject_cast<CustomTextWidget *>(widget()); }
};

#endif // ACCESSIBLEWIDGETS_H
