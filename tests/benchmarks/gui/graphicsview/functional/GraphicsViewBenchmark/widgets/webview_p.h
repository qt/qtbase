/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
