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

#ifndef QSYSTEMTRAYICON_H
#define QSYSTEMTRAYICON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>

#ifndef QT_NO_SYSTEMTRAYICON

#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE


class QSystemTrayIconPrivate;

class QMenu;
class QEvent;
class QWheelEvent;
class QMouseEvent;
class QPoint;

class Q_WIDGETS_EXPORT QSystemTrayIcon : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible DESIGNABLE false)

public:
    QSystemTrayIcon(QObject *parent = nullptr);
    QSystemTrayIcon(const QIcon &icon, QObject *parent = nullptr);
    ~QSystemTrayIcon();

    enum ActivationReason {
        Unknown,
        Context,
        DoubleClick,
        Trigger,
        MiddleClick
    };

#if QT_CONFIG(menu)
    void setContextMenu(QMenu *menu);
    QMenu *contextMenu() const;
#endif

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    QString toolTip() const;
    void setToolTip(const QString &tip);

    static bool isSystemTrayAvailable();
    static bool supportsMessages();

    enum MessageIcon { NoIcon, Information, Warning, Critical };

    QRect geometry() const;
    bool isVisible() const;

public Q_SLOTS:
    void setVisible(bool visible);
    inline void show() { setVisible(true); }
    inline void hide() { setVisible(false); }
    void showMessage(const QString &title, const QString &msg, const QIcon &icon, int msecs = 10000);
    void showMessage(const QString &title, const QString &msg,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information, int msecs = 10000);

Q_SIGNALS:
    void activated(QSystemTrayIcon::ActivationReason reason);
    void messageClicked();

protected:
    bool event(QEvent *event) override;

private:
    Q_DISABLE_COPY(QSystemTrayIcon)
    Q_DECLARE_PRIVATE(QSystemTrayIcon)

    Q_PRIVATE_SLOT(d_func(), void _q_emitActivated(QPlatformSystemTrayIcon::ActivationReason))

    friend class QSystemTrayIconSys;
    friend class QBalloonTip;
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON
#endif // QSYSTEMTRAYICON_H
