/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWSDISPLAY_QWS_H
#define QWSDISPLAY_QWS_H

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>
#include <QtGui/qregion.h>
#include <QtGui/qimage.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qlist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QWSEvent;
class QWSMouseEvent;
class QWSQCopMessageEvent;
class QVariant;
class QLock;

class QWSWindowInfo
{

public:

    int winid;
    unsigned int clientid;
    QString name;

};

#define QT_QWS_PROPERTY_CONVERTSELECTION 999
#define QT_QWS_PROPERTY_WINDOWNAME 998
#define QT_QWS_PROPERTY_MARKEDTEXT 997

class QWSDisplay;
extern Q_GUI_EXPORT QWSDisplay *qt_fbdpy;

class Q_GUI_EXPORT QWSDisplay
{
public:
    QWSDisplay();
    ~QWSDisplay();

    static QWSDisplay* instance() { return qt_fbdpy; }

    bool eventPending() const;
    QWSEvent *getEvent();
//    QWSRegionManager *regionManager() const;

    uchar* frameBuffer() const;
    int width() const;
    int height() const;
    int depth() const;
    int pixmapDepth() const;
    bool supportsDepth(int) const;

    uchar *sharedRam() const;
    int sharedRamSize() const;

#ifndef QT_NO_QWS_PROPERTIES
    void addProperty(int winId, int property);
    void setProperty(int winId, int property, int mode, const QByteArray &data);
    void setProperty(int winId, int property, int mode, const char * data);
    void removeProperty(int winId, int property);
    bool getProperty(int winId, int property, char *&data, int &len);
#endif // QT_NO_QWS_PROPERTIES

    QList<QWSWindowInfo> windowList();
    int windowAt(const QPoint &);

    void setIdentity(const QString &appName);
    void nameRegion(int winId, const QString& n, const QString &c);
    void requestRegion(int winId, const QString &surfacekey,
                       const QByteArray &surfaceData,
                       const QRegion &region);
    void repaintRegion(int winId, int windowFlags, bool opaque, QRegion);
    void moveRegion(int winId, int dx, int dy);
    void destroyRegion(int winId);
    void requestFocus(int winId, bool get);
    void setAltitude(int winId, int altitude, bool fixed = false);
    void setOpacity(int winId, int opacity);
    int takeId();
    void setSelectionOwner(int winId, const QTime &time);
    void convertSelection(int winId, int selectionProperty, const QString &mimeTypes);
    void defineCursor(int id, const QBitmap &curs, const QBitmap &mask,
                        int hotX, int hotY);
    void destroyCursor(int id);
    void selectCursor(QWidget *w, unsigned int id);
    void setCursorPosition(int x, int y);
    void grabMouse(QWidget *w, bool grab);
    void grabKeyboard(QWidget *w, bool grab);
    void playSoundFile(const QString&);
    void registerChannel(const QString &channel);
    void sendMessage(const QString &channel, const QString &msg,
                       const QByteArray &data);
    void flushCommands();
#ifndef QT_NO_QWS_INPUTMETHODS
    void sendIMUpdate(int type, int winId, int widgetid);
    void resetIM();
    void sendIMResponse(int winId, int property, const QVariant &result);
    void sendIMMouseEvent(int index, bool isPress);
#endif
    QWSQCopMessageEvent* waitForQCopResponse();
    void sendFontCommand(int type, const QByteArray &fontName);

    void setWindowCaption(QWidget *w, const QString &);

    // Lock display for access only by this process
    static bool initLock(const QString &filename, bool create = false);
    static bool grabbed();
    static void grab();
    static void grab(bool write);
    static void ungrab();

    static void setTransformation(int transformation, int screenNo = -1);
    static void setRawMouseEventFilter(void (*filter)(QWSMouseEvent *));

private:
    friend int qt_fork_qapplication();
    friend void qt_app_reinit( const QString& newAppName );
    friend class QApplication;
    friend class QCopChannel;
    friend class QWSEmbedWidget;
    friend class QWSEmbedWidgetPrivate;
    class Data;
    friend class Data;
    Data *d;

    friend class QWSMemorySurface;
    friend class QWSOnScreenSurface;
    friend class QWSDirectPainterSurface;
    int getPropertyLen;
    char *getPropertyData;
    static QLock *lock;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWSDISPLAY_QWS_H
