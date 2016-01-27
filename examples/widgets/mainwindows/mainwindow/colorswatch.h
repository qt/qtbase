/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef COLORSWATCH_H
#define COLORSWATCH_H

#include <QDockWidget>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QMenu)

class ColorSwatch : public QDockWidget
{
    Q_OBJECT

public:
    explicit ColorSwatch(const QString &colorName, QMainWindow *parent = Q_NULLPTR, Qt::WindowFlags flags = 0);

    void setCustomSizeHint(const QSize &size);
    QMenu *colorSwatchMenu() const { return menu; }

protected:
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;

private slots:
    void changeClosable(bool on);
    void changeMovable(bool on);
    void changeFloatable(bool on);
    void changeFloating(bool on);
    void changeVerticalTitleBar(bool on);
    void updateContextMenu();

    void allowLeft(bool a);
    void allowRight(bool a);
    void allowTop(bool a);
    void allowBottom(bool a);

    void placeLeft(bool p);
    void placeRight(bool p);
    void placeTop(bool p);
    void placeBottom(bool p);

    void splitInto(QAction *action);
    void tabInto(QAction *action);

private:
    void allow(Qt::DockWidgetArea area, bool allow);
    void place(Qt::DockWidgetArea area, bool place);

    QAction *closableAction;
    QAction *movableAction;
    QAction *floatableAction;
    QAction *floatingAction;
    QAction *verticalTitleBarAction;

    QActionGroup *allowedAreasActions;
    QAction *allowLeftAction;
    QAction *allowRightAction;
    QAction *allowTopAction;
    QAction *allowBottomAction;

    QActionGroup *areaActions;
    QAction *leftAction;
    QAction *rightAction;
    QAction *topAction;
    QAction *bottomAction;

    QMenu *tabMenu;
    QMenu *splitHMenu;
    QMenu *splitVMenu;
    QMenu *menu;

    QMainWindow *mainWindow;
};

class BlueTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit BlueTitleBar(QWidget *parent = Q_NULLPTR);

    QSize sizeHint() const Q_DECL_OVERRIDE { return minimumSizeHint(); }
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

public slots:
    void updateMask();

private:
    const QPixmap leftPm;
    const QPixmap centerPm;
    const QPixmap rightPm;
};

#endif // COLORSWATCH_H
