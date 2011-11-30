/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QACCESSIBLEWIDGETS_H
#define QACCESSIBLEWIDGETS_H

#include <QtGui/qaccessible2.h>
#include <QtWidgets/qaccessiblewidget.h>

#ifndef QT_NO_ACCESSIBILITY

#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

class QTextEdit;
class QStackedWidget;
class QToolBox;
class QMdiArea;
class QMdiSubWindow;
class QWorkspace;
class QRubberBand;
class QTextBrowser;
class QCalendarWidget;
class QAbstractItemView;
class QDockWidget;
class QDockWidgetLayout;
class QMainWindow;

#ifndef QT_NO_TEXTEDIT
class QAccessibleTextEdit : public QAccessibleWidget, public QAccessibleTextInterface,
                            public QAccessibleEditableTextInterface
{
public:
    explicit QAccessibleTextEdit(QWidget *o);

    QString text(Text t) const;
    void setText(Text t, const QString &text);

    QVariant invokeMethod(QAccessible::Method method, const QVariantList &params);
    void *interface_cast(QAccessible::InterfaceType t);

    // QAccessibleTextInterface
    void addSelection(int startOffset, int endOffset);
    QString attributes(int offset, int *startOffset, int *endOffset);
    int cursorPosition();
    QRect characterRect(int offset, QAccessible2::CoordinateType coordType);
    int selectionCount();
    int offsetAtPoint(const QPoint &point, QAccessible2::CoordinateType coordType);
    void selection(int selectionIndex, int *startOffset, int *endOffset);
    QString text(int startOffset, int endOffset);
    QString textBeforeOffset (int offset, QAccessible2::BoundaryType boundaryType,
            int *startOffset, int *endOffset);
    QString textAfterOffset(int offset, QAccessible2::BoundaryType boundaryType,
            int *startOffset, int *endOffset);
    QString textAtOffset(int offset, QAccessible2::BoundaryType boundaryType,
            int *startOffset, int *endOffset);
    void removeSelection(int selectionIndex);
    void setCursorPosition(int position);
    void setSelection(int selectionIndex, int startOffset, int endOffset);
    int characterCount();
    void scrollToSubstring(int startIndex, int endIndex);

    // QAccessibleEditableTextInterface
    void copyText(int startOffset, int endOffset);
    void deleteText(int startOffset, int endOffset);
    void insertText(int offset, const QString &text);
    void cutText(int startOffset, int endOffset);
    void pasteText(int offset);
    void replaceText(int startOffset, int endOffset, const QString &text);
    void setAttributes(int startOffset, int endOffset, const QString &attributes);

protected:
    QTextEdit *textEdit() const;

private:
    int childOffset;
};
#endif // QT_NO_TEXTEDIT

class QAccessibleStackedWidget : public QAccessibleWidget
{
public:
    explicit QAccessibleStackedWidget(QWidget *widget);

    QVariant invokeMethod(QAccessible::Method method, int child, const QVariantList &params);
    int childAt(int x, int y) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    QAccessibleInterface *child(int index) const;
    int navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const;

protected:
    QStackedWidget *stackedWidget() const;
};

class QAccessibleToolBox : public QAccessibleWidget
{
public:
    explicit QAccessibleToolBox(QWidget *widget);

// FIXME we currently expose the toolbox but it is not keyboard navigatable
// and the accessible hierarchy is not exactly beautiful.
//    int childCount() const;
//    QAccessibleInterface *child(int index) const;
//    int indexOfChild(const QAccessibleInterface *child) const;

protected:
    QToolBox *toolBox() const;
};

#ifndef QT_NO_MDIAREA
class QAccessibleMdiArea : public QAccessibleWidget
{
public:
    explicit QAccessibleMdiArea(QWidget *widget);

    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    int navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const;

protected:
    QMdiArea *mdiArea() const;
};

class QAccessibleMdiSubWindow : public QAccessibleWidget
{
public:
    explicit QAccessibleMdiSubWindow(QWidget *widget);

    QString text(Text textType) const;
    void setText(Text textType, const QString &text);
    State state() const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    int navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const;
    QRect rect() const;
    int childAt(int x, int y) const;

protected:
    QMdiSubWindow *mdiSubWindow() const;
};
#endif // QT_NO_MDIAREA

#ifndef QT_NO_WORKSPACE
class QAccessibleWorkspace : public QAccessibleWidget
{
public:
    explicit QAccessibleWorkspace(QWidget *widget);

    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    int navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const;

protected:
    QWorkspace *workspace() const;
};
#endif

class QAccessibleDialogButtonBox : public QAccessibleWidget
{
public:
    explicit QAccessibleDialogButtonBox(QWidget *widget);

    QVariant invokeMethod(QAccessible::Method method, int child, const QVariantList &params);
};

#ifndef QT_NO_TEXTBROWSER
class QAccessibleTextBrowser : public QAccessibleTextEdit
{
public:
    explicit QAccessibleTextBrowser(QWidget *widget);

    Role role() const;
};
#endif // QT_NO_TEXTBROWSER

#ifndef QT_NO_CALENDARWIDGET
class QAccessibleCalendarWidget : public QAccessibleWidget
{
public:
    explicit QAccessibleCalendarWidget(QWidget *widget);

    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    int navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const;

    QAccessibleInterface *child(int index) const;


    QVariant invokeMethod(QAccessible::Method method, int child, const QVariantList &params);
protected:
    QCalendarWidget *calendarWidget() const;

private:
    QAbstractItemView *calendarView() const;
    QWidget *navigationBar() const;
};
#endif // QT_NO_CALENDARWIDGET

#ifndef QT_NO_DOCKWIDGET
class QAccessibleDockWidget: public QAccessibleWidget
{
public:
    explicit QAccessibleDockWidget(QWidget *widget);
    QAccessibleInterface *child(int index) const;
    int indexOfChild(const QAccessibleInterface *child) const;
    int childCount() const;
    QRect rect () const;
    Role role() const;

    QDockWidget *dockWidget() const;
};

class QAccessibleTitleBar : public QAccessibleInterface
{
public:
    explicit QAccessibleTitleBar(QDockWidget *widget);

    QAccessibleInterface *parent() const;
    QAccessibleInterface *child(int index) const;
    int navigate(RelationFlag relation, int entry, QAccessibleInterface **iface) const;
    int indexOfChild(const QAccessibleInterface *child) const;
    int childCount() const;
    Relation relationTo(const QAccessibleInterface *other) const;
    void setText(Text t, const QString &text);
    QString text(Text t) const;
    Role role() const;
    QRect rect () const;
    State state() const;
    int childAt(int x, int y) const;
    QObject *object() const;
    bool isValid() const;

    QPointer<QDockWidget> m_dockWidget;

    QDockWidget *dockWidget() const;
    QDockWidgetLayout *dockWidgetLayout() const;
};

#endif // QT_NO_DOCKWIDGET

#ifndef QT_NO_MAINWINDOW
class QAccessibleMainWindow : public QAccessibleWidget
{
public:
    explicit QAccessibleMainWindow(QWidget *widget);

    QAccessibleInterface *child(int index) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *iface) const;
    int childAt(int x, int y) const;
    QMainWindow *mainWindow() const;

    QVariant invokeMethod(QAccessible::Method method, int child, const QVariantList &params);
};
#endif //QT_NO_MAINWINDOW

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QACESSIBLEWIDGETS_H
