/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    explicit ControllerWidget(QWidget *parent = 0);
    ~ControllerWidget();
private:
    QScopedPointer<QWindow> m_testWindow;
};

#endif // CONTROLLERWIDGET_H
