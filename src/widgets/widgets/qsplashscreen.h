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

#ifndef QSPLASHSCREEN_H
#define QSPLASHSCREEN_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtGui/qpixmap.h>
#include <QtWidgets/qwidget.h>

QT_REQUIRE_CONFIG(splashscreen);

QT_BEGIN_NAMESPACE

class QSplashScreenPrivate;

class Q_WIDGETS_EXPORT QSplashScreen : public QWidget
{
    Q_OBJECT
public:
    explicit QSplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
    QSplashScreen(QScreen *screen, const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_VERSION_X_5_15("Use the constructor taking a QScreen *")
    QSplashScreen(QWidget *parent, const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());
#endif
    virtual ~QSplashScreen();

    void setPixmap(const QPixmap &pixmap);
    const QPixmap pixmap() const;
    void finish(QWidget *w);
    void repaint();
    QString message() const;

public Q_SLOTS:
    void showMessage(const QString &message, int alignment = Qt::AlignLeft,
                  const QColor &color = Qt::black);
    void clearMessage();

Q_SIGNALS:
    void messageChanged(const QString &message);

protected:
    bool event(QEvent *e) override;
    virtual void drawContents(QPainter *painter);
    void mousePressEvent(QMouseEvent *) override;

private:
    Q_DISABLE_COPY(QSplashScreen)
    Q_DECLARE_PRIVATE(QSplashScreen)
};

QT_END_NAMESPACE

#endif // QSPLASHSCREEN_H
