// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "scrollbar.h"
#include "abstractscrollarea.h"

class WebViewPrivate;

class WebView : public AbstractScrollArea
{
    Q_OBJECT

public:

    WebView(QGraphicsWidget *parent = nullptr);
    ~WebView();

public:

    void setUrl(const QUrl& url);

private:

    void scrollContentsBy(qreal dx, qreal dy);
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF & constraint) const;

private:

    Q_PRIVATE_SLOT(d, void _q_loadStarted())
    Q_PRIVATE_SLOT(d, void _q_loadProgress(int))
    Q_PRIVATE_SLOT(d, void _q_loadFinished(bool))
    Q_PRIVATE_SLOT(d, void _q_viewportChanged(QGraphicsWidget*))
    Q_PRIVATE_SLOT(d, void _q_motionEnded())

    WebViewPrivate *d;
    friend class WebViewPrivate;
};

#endif // WEBVIEW_H
