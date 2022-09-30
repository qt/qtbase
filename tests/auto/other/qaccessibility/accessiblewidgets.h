// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


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
    QString text(QAccessible::Text t) const override
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
    KFooButton(const QString &text, QWidget *parent = nullptr)
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
    void *interface_cast(QAccessible::InterfaceType t) override
    {
        if (t == QAccessible::TextInterface)
            return static_cast<QAccessibleTextInterface*>(this);
        return 0;
    }

    // this is mostly to test the base implementation for textBefore/At/After
    QString text(QAccessible::Text t) const override
    {
        if (t == QAccessible::Value)
            return textWidget()->text;
        return QAccessibleWidget::text(t);
    }

    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override
    {
        if (offset == -2)
            offset = textWidget()->cursorPosition;
        return QAccessibleTextInterface::textBeforeOffset(offset, boundaryType, startOffset, endOffset);
    }
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override
    {
        if (offset == -2)
            offset = textWidget()->cursorPosition;
        return QAccessibleTextInterface::textAtOffset(offset, boundaryType, startOffset, endOffset);
    }
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override
    {
        if (offset == -2)
            offset = textWidget()->cursorPosition;
        return QAccessibleTextInterface::textAfterOffset(offset, boundaryType, startOffset, endOffset);
    }

    void selection(int, int *startOffset, int *endOffset) const override
    { *startOffset = *endOffset = -1; }
    int selectionCount() const override { return 0; }
    void addSelection(int, int) override {}
    void removeSelection(int) override {}
    void setSelection(int, int, int) override {}
    int cursorPosition() const override { return textWidget()->cursorPosition; }
    void setCursorPosition(int position) override { textWidget()->cursorPosition = position; }
    QString text(int startOffset, int endOffset) const override { return textWidget()->text.mid(startOffset, endOffset); }
    int characterCount() const override { return textWidget()->text.size(); }
    QRect characterRect(int) const override { return QRect(); }
    int offsetAtPoint(const QPoint &) const override { return 0; }
    void scrollToSubstring(int, int) override {}
    QString attributes(int, int *, int *) const override
    { return QString(); }

private:
    CustomTextWidget *textWidget() const { return qobject_cast<CustomTextWidget *>(widget()); }
};

#endif // ACCESSIBLEWIDGETS_H
