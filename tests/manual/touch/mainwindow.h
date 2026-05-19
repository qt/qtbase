// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gestures.h"

#include <QEvent>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QWidget>

#ifdef Q_OS_WIN
#  include <QDialog>
#endif

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QPaintEvent;
class QPlainTextEdit;
QT_END_NAMESPACE

enum PointType {
    TouchPoint,
    MousePress,
    MouseRelease
};

struct Point
{
    Point(const QPointF &p = QPoint(), PointType t = TouchPoint,
          Qt::MouseEventSource s = Qt::MouseEventNotSynthesized,
          QSizeF diameters = QSizeF(4, 4));

    QColor color() const;

    QPointF pos;
    qreal horizontalDiameter;
    qreal verticalDiameter;
    PointType type;
    Qt::MouseEventSource source;
};

typedef QList<QEvent::Type> EventTypeVector;

// Globals populated in main() and read by the widgets/event filter below.
extern bool optIgnoreTouch;
extern QList<Qt::GestureType> optGestures;
extern QWidgetList mainWindows;
extern EventTypeVector eventTypes;

class EventFilter;
extern EventFilter *globalEventFilter;

class EventFilter : public QObject {
    Q_OBJECT
public:
    explicit EventFilter(const EventTypeVector &types, QObject *p)
        : QObject(p), m_types(types) {}

    bool eventFilter(QObject *, QEvent *) override;

signals:
    void eventReceived(const QString &);

private:
    const EventTypeVector m_types;
};

class TouchTestWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool drawPoints READ drawPoints WRITE setDrawPoints)
public:
    explicit TouchTestWidget(QWidget *parent = nullptr);

    bool drawPoints() const { return m_drawPoints; }

public slots:
    void clearPoints();
    void setDrawPoints(bool drawPoints);

signals:
    void logMessage(const QString &);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *) override;

private:
    void handleGestureEvent(QGestureEvent *gestureEvent);

    QList<Point> m_points;
    GesturePtrs m_gestures;
    bool m_drawPoints;
};

#ifdef Q_OS_WIN
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent);

private slots:
    void touchTypeToggled();

private:
    QCheckBox *m_fineCheckBox;
    QCheckBox *m_palmCheckBox;
};
#endif // Q_OS_WIN

class MainWindow : public QMainWindow
{
    Q_OBJECT
    MainWindow();
public:
    static MainWindow *createMainWindow();

    QWidget *touchWidget() const { return m_touchWidget; }

    void setVisible(bool visible) override;

public slots:
    void appendToLog(const QString &text);
    void dumpTouchDevices();

private slots:
    void settingsDialog();

private:
    void updateScreenLabel();
    void newWindow() { MainWindow::createMainWindow(); }

    TouchTestWidget *m_touchWidget;
    QPlainTextEdit *m_logTextEdit;
    QLabel *m_screenLabel;
};

#endif // MAINWINDOW_H
