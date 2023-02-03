// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qmimedata.h>

#include "qutimimeconverter.h"
#include "qmacmimeregistry_p.h"
#include "qguiapplication.h"
#include "private/qcore_mac_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QMacMimeRegistry {

typedef QList<QUtiMimeConverter*> MimeList;
Q_GLOBAL_STATIC(MimeList, globalMimeList)
Q_GLOBAL_STATIC(QStringList, globalDraggedTypesList)

// implemented in qutimimeconverter.mm
void registerBuiltInTypes();

/*!
    \fn void qRegisterDraggedTypes(const QStringList &types)
    \relates QUtiMimeConverter

    Registers the given \a types as custom pasteboard types.

    This function should be called to enable the Drag and Drop events
    for custom pasteboard types on Cocoa implementations. This is required
    in addition to a QUtiMimeConverter subclass implementation. By default
    drag and drop is enabled for all standard pasteboard types.

   \sa QUtiMimeConverter
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
  Returns a MIME type of for scope \a scope for \a uti, or \nullptr if none exists.
*/
QString flavorToMime(QUtiMimeConverter::HandlerScope scope, const QString &uti)
{
    const MimeList &mimes = *globalMimeList();
    for (const auto &mime : mimes) {
        const bool relevantScope = mime->scope() & scope;
#ifdef DEBUG_MIME_MAPS
        qDebug("QMacMimeRegistry::flavorToMime: attempting (%d) for uti %s [%s]",
               relevantScope, qPrintable(uti), qPrintable((*it)->mimeForUti(uti)));
#endif
        if (relevantScope) {
            const QString mimeType = mime->mimeForUti(uti);
            if (!mimeType.isNull())
                return mimeType;
        }
    }
    return QString();
}

void registerMimeConverter(QUtiMimeConverter *macMime)
{
    // globalMimeList is in decreasing priority order. Recently added
    // converters take prioity over previously added converters: prepend
    // to the list.
    globalMimeList()->prepend(macMime);
}

void unregisterMimeConverter(QUtiMimeConverter *macMime)
{
    if (!QGuiApplication::closingDown())
        globalMimeList()->removeAll(macMime);
}


/*
  Returns a list of all currently defined QUtiMimeConverter objects for scope \a scope.
*/
QList<QUtiMimeConverter *> all(QUtiMimeConverter::HandlerScope scope)
{
    MimeList ret;
    const MimeList &mimes = *globalMimeList();
    for (const auto &mime : mimes) {
        if (mime->scope() & scope)
            ret.append(mime);
    }
    return ret;
}

} // namespace QMacMimeRegistry

QT_END_NAMESPACE
