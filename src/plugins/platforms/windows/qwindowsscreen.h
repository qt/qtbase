/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSSCREEN_H
#define QWINDOWSSCREEN_H

#include "qwindowscursor.h"

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtGui/QPlatformScreen>

QT_BEGIN_NAMESPACE

struct QWindowsScreenData
{
    enum Flags
    {
        PrimaryScreen = 0x1,
        VirtualDesktop = 0x2
    };

    QWindowsScreenData();

    QRect geometry;
    QRect availableGeometry;
    QDpi dpi;
    QSizeF physicalSizeMM;
    int depth;
    QImage::Format format;
    unsigned flags;
    QString name;
    Qt::ScreenOrientation orientation;
};

class QWindowsScreen : public QPlatformScreen
{
public:
    explicit QWindowsScreen(const QWindowsScreenData &data);

    static QWindowsScreen *screenOf(const QWindow *w = 0);

    virtual QRect geometry() const { return m_data.geometry; }
    virtual QRect availableGeometry() const { return m_data.availableGeometry; }
    virtual int depth() const { return m_data.depth; }
    virtual QImage::Format format() const { return m_data.format; }
    virtual QSizeF physicalSize() const { return m_data.physicalSizeMM; }
    virtual QDpi logicalDpi() const { return m_data.dpi; }
    virtual QString name() const { return m_data.name; }
    virtual Qt::ScreenOrientation primaryOrientation() { return m_data.orientation; }
    virtual QList<QPlatformScreen *> virtualSiblings() const;
    virtual QWindow *topLevelAt(const QPoint &point) const
        {  return QWindowsScreen::findTopLevelAt(point, CWP_SKIPINVISIBLE);  }

    static QWindow *findTopLevelAt(const QPoint &point, unsigned flags);
    static QWindow *windowAt(const QPoint &point, unsigned flags = CWP_SKIPINVISIBLE);
    static QWindow *windowUnderMouse(unsigned flags = CWP_SKIPINVISIBLE);

    virtual QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    inline void handleChanges(const QWindowsScreenData &newData);

    const QWindowsCursor &cursor() const    { return m_cursor; }
    QWindowsCursor &cursor()                { return m_cursor; }

    const QWindowsScreenData &data() const  { return m_data; }

private:
    QWindowsScreenData m_data;
    QWindowsCursor m_cursor;
};

class QWindowsScreenManager
{
public:
    typedef QList<QWindowsScreen *> WindowsScreenList;

    QWindowsScreenManager();

    inline void clearScreens() {
        // Delete screens in reverse order to avoid crash in case of multiple screens
        while (!m_screens.isEmpty())
            delete m_screens.takeLast();
    }

    void handleScreenChanges();
    bool handleDisplayChange(WPARAM wParam, LPARAM lParam);
    const WindowsScreenList &screens() const { return m_screens; }

private:
    WindowsScreenList m_screens;
    int m_lastDepth;
    WORD m_lastHorizontalResolution;
    WORD m_lastVerticalResolution;
};

QT_END_NAMESPACE

#endif // QWINDOWSSCREEN_H
