// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CONTROLLERWIDGET_H
#define CONTROLLERWIDGET_H

#include <QMainWindow>
#include <QGroupBox>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
class QSpinBox;
class QLabel;
class QGridLayout;
QT_END_NAMESPACE

class TypeControl;
class HintControl;

// A control for editing points or sizes
class CoordinateControl : public QWidget
{
    Q_OBJECT

public:
    CoordinateControl(const QString &sep);

    void setPointValue(const QPoint &p)  { setCoordinates(p.x(), p.y()); }
    QPoint pointValue() const            { const QPair<int, int> t = coordinates(); return QPoint(t.first, t.second); }

    void setSizeValue(const QSize &s)    { setCoordinates(s.width(), s.height()); }
    QSize sizeValue() const              { const QPair<int, int> t = coordinates(); return QSize(t.first, t.second); }

signals:
    void pointValueChanged(const QPoint &p);
    void sizeValueChanged(const QSize &s);

private slots:
    void spinBoxChanged();

private:
    void setCoordinates(int x, int y);
    QPair<int, int> coordinates() const;

    QSpinBox *m_x;
    QSpinBox *m_y;
};

// A control for editing QRect
class RectControl : public QGroupBox
{
    Q_OBJECT
public:
    RectControl();
    void setRectValue(const QRect &r);
    QRect rectValue() const;

signals:
    void changed(const QRect &r);
    void sizeChanged(const QSize &s);
    void positionChanged(const QPoint &s);

private slots:
    void handleChanged();

private:
    CoordinateControl *m_point;
    CoordinateControl *m_size;
};

// Base class for controlling the position of a Window (QWindow or QWidget)
class BaseWindowControl : public QGroupBox
{
    Q_OBJECT

protected:
    explicit BaseWindowControl(QObject *w);

public:
    virtual bool eventFilter(QObject *, QEvent *);
    virtual void refresh();

private slots:
    void posChanged(const QPoint &);
    void sizeChanged(const QSize &);
    void framePosChanged(const QPoint &);

protected:
    QGridLayout *m_layout;
    QObject *m_object;

private:
    virtual QRect objectGeometry(const QObject *o) const = 0;
    virtual void setObjectGeometry(QObject *o, const QRect &) const = 0;

    virtual QPoint objectFramePosition(const QObject *o) const = 0;
    virtual void setObjectFramePosition(QObject *o, const QPoint &) const = 0;

    virtual QPoint objectMapToGlobal(const QObject *o, const QPoint &) const = 0;

    RectControl *m_geometry;
    CoordinateControl *m_framePosition;
    QLabel *m_moveEventLabel;
    QLabel *m_resizeEventLabel;
    QLabel *m_mouseEventLabel;
    unsigned m_moveCount;
    unsigned m_resizeCount;
};

class ControllerWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ControllerWidget(QWidget *parent = nullptr);
    ~ControllerWidget();
private:
    QScopedPointer<QWindow> m_testWindow;
};

#endif // CONTROLLERWIDGET_H
