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

#ifndef QINPUTMETHOD_H
#define QINPUTMETHOD_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QInputMethodPrivate;
class QWindow;
class QRectF;
class QTransform;
class QInputMethodQueryEvent;

class Q_GUI_EXPORT QInputMethod : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QInputMethod)
    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QRectF anchorRectangle READ anchorRectangle NOTIFY anchorRectangleChanged)
    Q_PROPERTY(QRectF keyboardRectangle READ keyboardRectangle NOTIFY keyboardRectangleChanged)
    Q_PROPERTY(QRectF inputItemClipRectangle READ inputItemClipRectangle NOTIFY inputItemClipRectangleChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool animating READ isAnimating NOTIFY animatingChanged)
    Q_PROPERTY(QLocale locale READ locale NOTIFY localeChanged)
    Q_PROPERTY(Qt::LayoutDirection inputDirection READ inputDirection NOTIFY inputDirectionChanged)

public:
    QTransform inputItemTransform() const;
    void setInputItemTransform(const QTransform &transform);

    QRectF inputItemRectangle() const;
    void setInputItemRectangle(const QRectF &rect);

    // in window coordinates
    QRectF cursorRectangle() const; // ### what if we have rotations for the item?
    QRectF anchorRectangle() const; // ### ditto

    // keyboard geometry in window coords
    QRectF keyboardRectangle() const;

    QRectF inputItemClipRectangle() const;

    enum Action {
        Click,
        ContextMenu
    };
    Q_ENUM(Action)

    bool isVisible() const;
    void setVisible(bool visible);

    bool isAnimating() const;

    QLocale locale() const;
    Qt::LayoutDirection inputDirection() const;

    static QVariant queryFocusObject(Qt::InputMethodQuery query, QVariant argument); // ### Qt 6: QVariant by const-ref

public Q_SLOTS:
    void show();
    void hide();

    void update(Qt::InputMethodQueries queries);
    void reset();
    void commit();

    void invokeAction(Action a, int cursorPosition);

Q_SIGNALS:
    void cursorRectangleChanged();
    void anchorRectangleChanged();
    void keyboardRectangleChanged();
    void inputItemClipRectangleChanged();
    void visibleChanged();
    void animatingChanged();
    void localeChanged();
    void inputDirectionChanged(Qt::LayoutDirection newDirection);

private:
    friend class QGuiApplication;
    friend class QGuiApplicationPrivate;
    friend class QPlatformInputContext;
    QInputMethod();
    ~QInputMethod();
};

QT_END_NAMESPACE

#endif
