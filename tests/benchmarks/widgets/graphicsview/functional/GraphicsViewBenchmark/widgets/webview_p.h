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

#ifndef WEBVIEW_P_H
#define WEBVIEW_P_H

#include "webview.h"
#include <QtWebKit/qgraphicswebview.h>
#include <QtWebKit/qwebpage.h>
#include <QtWebKit/qwebframe.h>
#include <QGraphicsEffect>
#include <QPainter>
#include <QPixmapCache>
#include <QTimer>
#include <QDebug>

class WebViewCache;

class WebViewPrivate {
public:

    WebViewPrivate(WebView *w);
    void adjustSize();
    void _q_loadStarted();
    void _q_loadProgress(int);
    void _q_loadFinished(bool);
    void _q_viewportChanged(QGraphicsWidget*);
    void _q_motionEnded();

    WebView *q;
    QGraphicsWebView *web;
    WebViewCache *cache;
    QTimer motionTimer;
};

class WebViewCache : public QGraphicsEffect
{
    Q_OBJECT

public:

    WebViewCache(QGraphicsWebView *webView);
    virtual ~WebViewCache();

public:

    void refresh();

    void draw(QPainter * painter, QGraphicsEffectSource * source);

private:

    QVector<QRectF> m_tileRects;
    QVector<QPixmapCache::Key> m_tilePixmaps;
    QSizeF m_itemSize;
    QGraphicsWebView *m_webView;

    friend class WebViewPrivate;
};

#endif // WEBVIEW_P_H
