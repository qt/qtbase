/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWAYLANDCLIPBOARD_H
#define QWAYLANDCLIPBOARD_H

#include <QtGui/QPlatformClipboard>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

class QWaylandDisplay;
class QWaylandSelection;
class QWaylandMimeData;
struct wl_selection_offer;

class QWaylandClipboard : public QPlatformClipboard
{
public:
    QWaylandClipboard(QWaylandDisplay *display);
    ~QWaylandClipboard();

    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard);
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard);
    bool supportsMode(QClipboard::Mode mode) const;

    void unregisterSelection(QWaylandSelection *selection);

    void createSelectionOffer(uint32_t id);

    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const;

private:
    static void offer(void *data,
                      struct wl_selection_offer *selection_offer,
                      const char *type);
    static void keyboardFocus(void *data,
                              struct wl_selection_offer *selection_offer,
                              struct wl_input_device *input_device);
    static const struct wl_selection_offer_listener selectionOfferListener;

    static void syncCallback(void *data);
    static void forceRoundtrip(struct wl_display *display);

    QWaylandDisplay *mDisplay;
    QWaylandSelection *mSelection;
    QWaylandMimeData *mMimeDataIn;
    QList<QWaylandSelection *> mSelections;
    QStringList mOfferedMimeTypes;
    struct wl_selection_offer *mOffer;
};

#endif // QWAYLANDCLIPBOARD_H
