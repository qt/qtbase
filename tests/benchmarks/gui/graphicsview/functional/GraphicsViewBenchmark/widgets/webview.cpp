/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "webview.h"
#include "webview_p.h"
#include <QtGui>

static const int MotionEndWaitTime = 2000;
static const int TileSideLength = 128;

WebViewPrivate::WebViewPrivate(WebView *w)
    : q(w)
    , cache(0)
{
    web = new QGraphicsWebView;

    web->setParentItem(q->viewport());

    web->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    web->page()->mainFrame()->setScrollBarPolicy(
        Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    web->page()->mainFrame()->setScrollBarPolicy(
        Qt::Vertical, Qt::ScrollBarAlwaysOff);
    web->setZValue(3);

//    cache = new WebViewCache(web);
//    web->setGraphicsEffect(cache);

    adjustSize();
}

void WebViewPrivate::adjustSize()
{
    QSizeF contentSize = web->page()->mainFrame()->contentsSize();
    QPointF pos = web->pos();

    qreal w = qMax(contentSize.width(), q->viewport()->boundingRect().width());
    qreal h = qMax(contentSize.height(), q->viewport()->boundingRect().height());

    if (web->boundingRect().size() != QSizeF(w, h)) {
        //qDebug() << "WebView: adjustSize:" << QSizeF(w, h);

        web->resize(w, h);
        web->setPos(pos);

        if (w > q->viewport()->boundingRect().width())
            q->horizontalScrollBar()->setSliderSize(w);
        else
            q->horizontalScrollBar()->setSliderSize(0.0);

        if (h > q->viewport()->boundingRect().height())
            q->verticalScrollBar()->setSliderSize(h);
        else
            q->verticalScrollBar()->setSliderSize(0.0);
    }
}

void WebViewPrivate::_q_loadStarted()
{
    qDebug() << "WebView: load started";
    adjustSize();
}

void WebViewPrivate::_q_loadProgress(int progress)
{
    Q_UNUSED(progress)
//    qDebug() << "WebView: load progress" << progress;
    adjustSize();
}

void WebViewPrivate::_q_loadFinished(bool ok)
{
    qDebug() << "WebView: load finished" << (ok ? "ok" : "not ok");
    adjustSize();
}

void WebViewPrivate::_q_viewportChanged(QGraphicsWidget* viewport)
{
    web->setParentItem(viewport);
    viewport->setFlag(QGraphicsItem::ItemClipsChildrenToShape,
                      true);
    adjustSize();
}

void WebViewPrivate::_q_motionEnded()
{
    motionTimer.stop();
    qDebug() << "Motion ended";
    q->prepareGeometryChange();
}

WebViewCache::WebViewCache(QGraphicsWebView *webView)
    : m_webView(webView)
{
}

WebViewCache::~WebViewCache()
{
}

void WebViewCache::draw(QPainter * painter, QGraphicsEffectSource * source)
{
    const QGraphicsItem *item = source->graphicsItem();

    QSizeF itemSize = item->boundingRect().size();

    if (!qFuzzyCompare(itemSize.width(), m_itemSize.width()) ||
            !qFuzzyCompare(itemSize.height(), m_itemSize.height())) {
        qDebug() << "Refresh tile cache, for new size" << itemSize;

        for (int i = 0; i < m_tilePixmaps.size(); i++) {
            QPixmapCache::remove(m_tilePixmaps[i]);
        }

        m_tilePixmaps.clear();
        m_tileRects.clear();

        int itemWidth = itemSize.width() + 0.5;
        int itemHeight = itemSize.height() + 0.5;

        int tilesX = itemWidth / TileSideLength;
        int tilesY = itemHeight / TileSideLength;

        if ((itemWidth % TileSideLength) != 0) {
            ++tilesX;
        }

        if ((itemHeight % TileSideLength) != 0) {
            ++tilesY;
        }

        int tilesCount = tilesX * tilesY;

        m_tilePixmaps.resize(tilesCount);
        m_tileRects.resize(tilesCount);

        for (int i = 0; i < tilesX; i++) {
            for (int j = 0; j < tilesY; j++) {
                int x = i * TileSideLength;
                int y = j * TileSideLength;

                m_tileRects[i + j * tilesX]
                    = QRectF(x, y, TileSideLength, TileSideLength);
            }
        }

        m_itemSize = itemSize;
    }

    const QGraphicsItem *parentItem = item->parentItem();
    QPointF itemPos = item->pos();
    QRectF parentRect = parentItem->boundingRect();

    for (int i = 0; i < m_tileRects.size(); i++) {
        QRectF tileRect = m_tileRects[i].translated(itemPos);

        if (!tileRect.intersects(parentRect) && !tileRect.contains(parentRect)) {
            continue;
        }

        QPixmap tilePixmap;

        if (!QPixmapCache::find(m_tilePixmaps[i], &tilePixmap)) {
            tilePixmap = QPixmap(TileSideLength, TileSideLength);

            QWebFrame *webFrame = m_webView->page()->mainFrame();

            QPainter tilePainter(&tilePixmap);
            tilePainter.translate(-m_tileRects[i].left(), -m_tileRects[i].top());
            webFrame->render(&tilePainter, m_tileRects[i].toRect());
            tilePainter.end();

            m_tilePixmaps[i] = QPixmapCache::insert(tilePixmap);
        }

        tileRect = tileRect.translated(-itemPos);

        painter->drawPixmap(tileRect.topLeft(), tilePixmap);
    }
}

WebView::WebView(QGraphicsWidget *parent)
    : AbstractScrollArea(parent)
    , d(new WebViewPrivate(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0, 0, 0, 0);
    connect(d->web->page(), SIGNAL(loadStarted()),
            this, SLOT(_q_loadStarted()));
    connect(d->web->page(), SIGNAL(loadProgress(int)),
            this, SLOT(_q_loadProgress(int)));
    connect(d->web->page(), SIGNAL(loadFinished(bool)),
            this, SLOT(_q_loadFinished(bool)));
    connect(this, SIGNAL(viewportChanged(QGraphicsWidget*)),
            this, SLOT(_q_viewportChanged(QGraphicsWidget*)));
    connect(&d->motionTimer, SIGNAL(timeout()),
            this, SLOT(_q_motionEnded()));
}

WebView::~WebView()
{
    d->web->setGraphicsEffect(0);
    delete d->cache;
}

void WebView::setUrl(const QUrl& url)
{
    d->adjustSize();
    d->web->setUrl(url);
}

void WebView::scrollContentsBy(qreal dx, qreal dy)
{
    if (qFuzzyCompare((float)dy, 0.0f) && qFuzzyCompare((float)dx, 0.0f))
        return;

    if (!d->motionTimer.isActive()) {
        d->motionTimer.start(MotionEndWaitTime);
    }

    QSizeF contentSize = d->web->page()->mainFrame()->contentsSize();
    QRectF viewportRect = viewport()->boundingRect();
    QPointF pos = d->web->pos();

    qreal w = qMax(contentSize.width(), viewportRect.width());
    qreal h = qMax(contentSize.height(), viewportRect.height());

    qreal minx = qMin(0.0f, (float) -(w - viewportRect.width()));
    qreal miny = qMin(0.0f, (float) -(h - viewportRect.height()));

    qreal x = d->web->pos().x() - dx;

    if (x < minx)
        x = minx;
    else if (x > 0)
        x = 0.0;

   qreal y = d->web->pos().y() - dy;

    if (y < miny)
        y = miny;
    else if (y > 0)
        y = 0.0;

    d->web->setPos(x, y);
}

QSizeF WebView::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    if (which == Qt::PreferredSize) {
        QSizeF contentSize = d->web->page()->mainFrame()->contentsSize();
        return contentSize;
    }

    return AbstractScrollArea::sizeHint(which, constraint);
}

void WebView::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    AbstractScrollArea::resizeEvent(event);
    d->adjustSize();
}

#include "moc_webview.cpp"
