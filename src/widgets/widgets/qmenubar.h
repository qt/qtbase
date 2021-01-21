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

#ifndef QMENUBAR_H
#define QMENUBAR_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qmenu.h>

QT_REQUIRE_CONFIG(menubar);

QT_BEGIN_NAMESPACE

class QMenuBarPrivate;
class QStyleOptionMenuItem;
class QWindowsStyle;
class QPlatformMenuBar;

class Q_WIDGETS_EXPORT QMenuBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool defaultUp READ isDefaultUp WRITE setDefaultUp)
    Q_PROPERTY(bool nativeMenuBar READ isNativeMenuBar WRITE setNativeMenuBar)

public:
    explicit QMenuBar(QWidget *parent = nullptr);
    ~QMenuBar();

    using QWidget::addAction;
    QAction *addAction(const QString &text);
    QAction *addAction(const QString &text, const QObject *receiver, const char* member);

#ifdef Q_CLANG_QDOC
    template<typename Obj, typename PointerToMemberFunctionOrFunctor>
    QAction *addAction(const QString &text, const Obj *receiver, PointerToMemberFunctionOrFunctor method);
    template<typename Functor>
    QAction *addAction(const QString &text, Functor functor);
#else
    // addAction(QString): Connect to a QObject slot / functor or function pointer (with context)
    template<typename Obj, typename Func1>
    inline typename std::enable_if<!std::is_same<const char*, Func1>::value
        && QtPrivate::IsPointerToTypeDerivedFromQObject<Obj*>::Value, QAction *>::type
        addAction(const QString &text, const Obj *object, Func1 slot)
    {
        QAction *result = addAction(text);
        connect(result, &QAction::triggered, object, std::move(slot));
        return result;
    }
    // addAction(QString): Connect to a functor or function pointer (without context)
    template <typename Func1>
    inline QAction *addAction(const QString &text, Func1 slot)
    {
        QAction *result = addAction(text);
        connect(result, &QAction::triggered, std::move(slot));
        return result;
    }
#endif // !Q_CLANG_QDOC

    QAction *addMenu(QMenu *menu);
    QMenu *addMenu(const QString &title);
    QMenu *addMenu(const QIcon &icon, const QString &title);


    QAction *addSeparator();
    QAction *insertSeparator(QAction *before);

    QAction *insertMenu(QAction *before, QMenu *menu);

    void clear();

    QAction *activeAction() const;
    void setActiveAction(QAction *action);

    void setDefaultUp(bool);
    bool isDefaultUp() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int) const override;

    QRect actionGeometry(QAction *) const;
    QAction *actionAt(const QPoint &) const;

    void setCornerWidget(QWidget *w, Qt::Corner corner = Qt::TopRightCorner);
    QWidget *cornerWidget(Qt::Corner corner = Qt::TopRightCorner) const;

#if defined(Q_OS_MACOS) || defined(Q_CLANG_QDOC)
    NSMenu* toNSMenu();
#endif

    bool isNativeMenuBar() const;
    void setNativeMenuBar(bool nativeMenuBar);
    QPlatformMenuBar *platformMenuBar();
public Q_SLOTS:
    void setVisible(bool visible) override;

Q_SIGNALS:
    void triggered(QAction *action);
    void hovered(QAction *action);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void actionEvent(QActionEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void timerEvent(QTimerEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;
    bool event(QEvent *) override;
    void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const;

private:
    Q_DECLARE_PRIVATE(QMenuBar)
    Q_DISABLE_COPY(QMenuBar)
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())
    Q_PRIVATE_SLOT(d_func(), void _q_actionHovered())
    Q_PRIVATE_SLOT(d_func(), void _q_internalShortcutActivated(int))
    Q_PRIVATE_SLOT(d_func(), void _q_updateLayout())

    friend class QMenu;
    friend class QMenuPrivate;
    friend class QWindowsStyle;
};

QT_END_NAMESPACE

#endif // QMENUBAR_H
