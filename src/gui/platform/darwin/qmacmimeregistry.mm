// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qmimedata.h>

#include "qmacmime_p.h"
#include "qmacmimeregistry_p.h"
#include "qguiapplication.h"
#include "private/qcore_mac_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QMacMimeRegistry {

typedef QList<QMacMime*> MimeList;
Q_GLOBAL_STATIC(MimeList, globalMimeList)
Q_GLOBAL_STATIC(QStringList, globalDraggedTypesList)

// implemented in qmacmime.mm
void registerBuiltInTypes();

/*!
    \fn void qRegisterDraggedTypes(const QStringList &types)
    \relates QMacMime

    Registers the given \a types as custom pasteboard types.

    This function should be called to enable the Drag and Drop events
    for custom pasteboard types on Cocoa implementations. This is required
    in addition to a QMacMime subclass implementation. By default
    drag and drop is enabled for all standard pasteboard types.

   \sa QMacMime
*/

void registerDraggedTypes(const QStringList &types)
{
    (*globalDraggedTypesList()) += types;
}

const QStringList& enabledDraggedTypes()
{
    return (*globalDraggedTypesList());
}

/*****************************************************************************
  QDnD debug facilities
 *****************************************************************************/
//#define DEBUG_MIME_MAPS

/*!
  \class QMacMimeRegistry
  \internal
  \ingroup draganddrop
*/

/*!
  \internal

  This is an internal function.
*/
void initializeMimeTypes()
{
    if (globalMimeList()->isEmpty())
        registerBuiltInTypes();
}

/*!
  \internal
*/
void destroyMimeTypes()
{
    MimeList *mimes = globalMimeList();
    while (!mimes->isEmpty())
        delete mimes->takeFirst();
}

/*
  Returns a MIME type of for scope \a scope for \a flav, or \nullptr if none exists.
*/
QString flavorToMime(QMacMime::HandlerScope scope, const QString &flav)
{
    MimeList *mimes = globalMimeList();
    for (MimeList::const_iterator it = mimes->constBegin(); it != mimes->constEnd(); ++it) {
        const bool relevantScope = uchar((*it)->scope()) & uchar(scope);
#ifdef DEBUG_MIME_MAPS
        qDebug("QMacMimeRegistry::flavorToMime: attempting (%d) for flavor %s [%s]",
               relevantScope, qPrintable(flav), qPrintable((*it)->mimeForFlavor(flav)));
#endif
        if (relevantScope) {
            QString mimeType = (*it)->mimeForFlavor(flav);
            if (!mimeType.isNull())
                return mimeType;
        }
    }
    return QString();
}

void registerMimeConverter(QMacMime *macMime)
{
    // globalMimeList is in decreasing priority order. Recently added
    // converters take prioity over previously added converters: prepend
    // to the list.
    globalMimeList()->prepend(macMime);
}

void unregisterMimeConverter(QMacMime *macMime)
{
    if (!QGuiApplication::closingDown())
        globalMimeList()->removeAll(macMime);
}


/*
  Returns a list of all currently defined QMacMime objects for scope \a scope.
*/
QList<QMacMime *> all(QMacMime::HandlerScope scope)
{
    MimeList ret;
    MimeList *mimes = globalMimeList();
    for (MimeList::const_iterator it = mimes->constBegin(); it != mimes->constEnd(); ++it) {
        const bool relevantScope = uchar((*it)->scope()) & uchar(scope);
        if (relevantScope)
            ret.append((*it));
    }
    return ret;
}

} // namespace QMacMimeRegistry

QT_END_NAMESPACE
