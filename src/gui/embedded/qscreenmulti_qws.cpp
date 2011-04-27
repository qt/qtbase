/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qscreenmulti_qws_p.h"

#ifndef QT_NO_QWS_MULTISCREEN

#include <qlist.h>
#include <qstringlist.h>
#include <qwidget.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QWS_CURSOR

class QMultiScreenCursor : public QScreenCursor
{
public:
    QMultiScreenCursor() : currentCursor(qt_screencursor) { enable = false; }
    ~QMultiScreenCursor() { qt_screencursor = 0; }

    void set(const QImage &image, int hotx, int hoty);
    void move(int x, int y);
    void show();
    void hide();

    void addCursor(QScreenCursor *cursor);

private:
    void setCurrentCursor(QScreenCursor *newCursor);

    QScreenCursor *currentCursor;
    QList<QScreenCursor*> cursors;
};

void QMultiScreenCursor::set(const QImage &image, int hotx, int hoty)
{
    QScreenCursor::set(image, hotx, hoty);
    if (currentCursor)
        currentCursor->set(image, hotx, hoty);
}

void QMultiScreenCursor::setCurrentCursor(QScreenCursor *newCursor)
{
    *((QScreenCursor*)this) = *newCursor;
    currentCursor = newCursor;
}

// XXX: this is a mess!
void QMultiScreenCursor::move(int x, int y)
{
    const int oldIndex = qt_screen->subScreenIndexAt(pos);
    QScreenCursor::move(x, y); // updates pos
    const int newIndex = qt_screen->subScreenIndexAt(pos);

    if (!currentCursor && oldIndex != -1)
        setCurrentCursor(cursors.at(oldIndex));
    QScreenCursor *oldCursor = currentCursor;

    if (oldIndex != -1) {
        const QScreen *oldScreen = qt_screen->subScreens().at(oldIndex);
        if (newIndex == -1 || oldScreen->region().contains(pos)) {
            oldCursor->move(x, y);
            return;
        }
    }

    if (newIndex != -1) {
        QScreenCursor *newCursor = cursors.at(newIndex);
        newCursor->set(cursor, hotspot.x(), hotspot.y());

        if (oldCursor) {
            if (oldCursor->isVisible())
                newCursor->show();
            oldCursor->hide();
        }

        newCursor->move(x, y);

        setCurrentCursor(newCursor);
    }
}

void QMultiScreenCursor::show()
{
    if (currentCursor)
        currentCursor->show();
}

void QMultiScreenCursor::hide()
{
    if (currentCursor)
        currentCursor->hide();
}

void QMultiScreenCursor::addCursor(QScreenCursor *cursor)
{
    cursors.append(cursor);
}

#endif

class QMultiScreenPrivate
{
public:
    QMultiScreenPrivate()
#ifndef QT_NO_QWS_CURSOR
        : cursor(0)
#endif
    {}
    ~QMultiScreenPrivate()
    {
#ifndef QT_NO_QWS_CURSOR
        delete cursor;
#endif
    }

    QList<QScreen*> screens;
    QRegion region;
#ifndef QT_NO_QWS_CURSOR
    QMultiScreenCursor *cursor;
#endif
};

QMultiScreen::QMultiScreen(int displayId)
    : QScreen(displayId, MultiClass), d_ptr(new QMultiScreenPrivate)
{
}

QMultiScreen::~QMultiScreen()
{
    delete d_ptr;
}

bool QMultiScreen::initDevice()
{
    bool ok = true;

#ifndef QT_NO_QWS_CURSOR
    d_ptr->cursor = new QMultiScreenCursor;
#endif

    const int n = d_ptr->screens.count();
    for (int i = 0; i < n; ++i) {
        QScreen *s = d_ptr->screens.at(i);
        ok = s->initDevice() && ok;
#ifndef QT_NO_QWS_CURSOR
        d_ptr->cursor->addCursor(qt_screencursor); // XXX
#endif
    }

#ifndef QT_NO_QWS_CURSOR
    // XXX
    qt_screencursor = d_ptr->cursor;
#endif

    return ok;
}

static int getDisplayId(const QString &spec)
{
    QRegExp regexp(QLatin1String(":(\\d+)\\b"));
    if (regexp.lastIndexIn(spec) != -1) {
        const QString capture = regexp.cap(1);
        return capture.toInt();
    }
    return 0;
}

static QPoint filterDisplayOffset(QString &spec)
{
    QRegExp regexp(QLatin1String(":offset=(\\d+),(\\d+)\\b"));
    if (regexp.indexIn(spec) == -1)
        return QPoint();

    const int x = regexp.cap(1).toInt();
    const int y = regexp.cap(2).toInt();
    spec.remove(regexp.pos(0), regexp.matchedLength());
    return QPoint(x, y);
}

bool QMultiScreen::connect(const QString &displaySpec)
{
    QString dSpec = displaySpec;
    if (dSpec.startsWith(QLatin1String("Multi:"), Qt::CaseInsensitive))
        dSpec = dSpec.mid(QString::fromLatin1("Multi:").size());

    const QString displayIdSpec = QString::fromLatin1(" :%1").arg(displayId);
    if (dSpec.endsWith(displayIdSpec))
        dSpec = dSpec.left(dSpec.size() - displayIdSpec.size());

    QStringList specs = dSpec.split(QLatin1Char(' '), QString::SkipEmptyParts);
    foreach (QString spec, specs) {
        const int id = getDisplayId(spec);
        if (spec.startsWith("vnc:", Qt::CaseInsensitive)) {
            spec.append(":noDisablePainting");
        }
        const QPoint offset = filterDisplayOffset(spec);
        QScreen *s = qt_get_screen(id, spec.toLatin1().constData());
        s->setOffset(offset);
        addSubScreen(s);
    }

    QScreen *firstScreen = d_ptr->screens.at(0);
    Q_ASSERT(firstScreen);

    // XXX
    QScreen::d = firstScreen->depth();

    QScreen::lstep = 0;
    QScreen::data = 0;
    QScreen::size = 0;

    QScreen::w = d_ptr->region.boundingRect().width();
    QScreen::h = d_ptr->region.boundingRect().height();

    QScreen::dw = QScreen::w;
    QScreen::dh = QScreen::h;

    // XXX - Extend the physical size based on the first screen
    // to encompass all screens, so that code that uses the multi
    // screen to calculate dpi values will get the right numbers.
    QScreen::physWidth = firstScreen->physicalWidth() * w / firstScreen->width();
    QScreen::physHeight = firstScreen->physicalHeight() * h / firstScreen->height();

    // XXXXX
    qt_screen = this;

    return true;
}

void QMultiScreen::disconnect()
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->disconnect();
}

void QMultiScreen::shutdownDevice()
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->shutdownDevice();
}

void QMultiScreen::setMode(int, int, int)
{
    return;
}

bool QMultiScreen::supportsDepth(int) const
{
    return false;
}

void QMultiScreen::save()
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->save();
}

void QMultiScreen::restore()
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->restore();
}

void QMultiScreen::blank(bool on)
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->blank(on);
}

bool QMultiScreen::onCard(const unsigned char *ptr) const
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        if (d_ptr->screens.at(i)->onCard(ptr))
            return true;
    return false;
}

bool QMultiScreen::onCard(const unsigned char *ptr, ulong &offset) const
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        if (d_ptr->screens.at(i)->onCard(ptr, offset))
            return true;
    return false;
}

bool QMultiScreen::isInterlaced() const
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        if (d_ptr->screens.at(i)->isInterlaced())
            return true;

    return false;
}

int QMultiScreen::memoryNeeded(const QString &string)
{
    int total = 0;
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        total += d_ptr->screens.at(i)->memoryNeeded(string);
    return total;
}

int QMultiScreen::sharedRamSize(void *arg)
{
    int total = 0;
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        total += d_ptr->screens.at(i)->sharedRamSize(arg);
    return total;
}

void QMultiScreen::haltUpdates()
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->haltUpdates();
}

void QMultiScreen::resumeUpdates()
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i)
        d_ptr->screens.at(i)->resumeUpdates();
}

void QMultiScreen::exposeRegion(QRegion region, int changing)
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i) {
        QScreen *screen = d_ptr->screens.at(i);
        const QRegion r = region & screen->region();
        if (r.isEmpty())
            continue;
        screen->exposeRegion(r, changing);
    }
}

void QMultiScreen::solidFill(const QColor &color, const QRegion &region)
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i) {
        QScreen *screen = d_ptr->screens.at(i);
        const QRegion r = region & screen->region();
        if (r.isEmpty())
            continue;
        screen->solidFill(color, r);
    }
}

void QMultiScreen::blit(const QImage &img, const QPoint &topLeft,
                        const QRegion &region)
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i) {
        QScreen *screen = d_ptr->screens.at(i);
        const QRegion r = region & screen->region();
        if (r.isEmpty())
            continue;
        screen->blit(img, topLeft, r);
    }
}

void QMultiScreen::blit(QWSWindow *bs, const QRegion &clip)
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i) {
        QScreen *screen = d_ptr->screens.at(i);
        const QRegion r = clip & screen->region();
        if (r.isEmpty())
            continue;
        screen->blit(bs, r);
    }
}

void QMultiScreen::setDirty(const QRect &rect)
{
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i) {
        QScreen *screen = d_ptr->screens.at(i);
        const QRegion r = screen->region() & rect;
        if (r.isEmpty())
            continue;
        screen->setDirty(r.boundingRect());
    }
}


QWSWindowSurface* QMultiScreen::createSurface(const QString &key) const
{
    QWSWindowSurface* surf = 0;
    const int n = d_ptr->screens.size();
    for (int i = 0; i < n; ++i) {
        QScreen *screen = d_ptr->screens.at(i);
        surf = screen->createSurface(key);
        if (surf)
            break;
    }
    return surf;
}


QWSWindowSurface* QMultiScreen::createSurface(QWidget *widget) const
{
    const QPoint midpoint = (widget->frameGeometry().topLeft()
                             + widget->frameGeometry().bottomRight()) / 2;
    int index = subScreenIndexAt(midpoint);
    if (index == -1)
        index = 0; // XXX
    return d_ptr->screens.at(index)->createSurface(widget);
}

QList<QScreen*> QMultiScreen::subScreens() const
{
    return d_ptr->screens;
}

QRegion QMultiScreen::region() const
{
    return d_ptr->region;
}

void QMultiScreen::addSubScreen(QScreen *screen)
{
    d_ptr->screens.append(screen);
    d_ptr->region += screen->region();
}

void QMultiScreen::removeSubScreen(QScreen *screen)
{
    d_ptr->screens.removeAll(screen);
    d_ptr->region -= screen->region();
}

QT_END_NAMESPACE

#endif // QT_NO_QWS_MULTISCREEN
