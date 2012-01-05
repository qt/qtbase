/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "scrollbar.h"
#include "abstractscrollarea.h"

class WebViewPrivate;

class WebView : public AbstractScrollArea
{
    Q_OBJECT

public:

    WebView(QGraphicsWidget *parent = 0);
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
