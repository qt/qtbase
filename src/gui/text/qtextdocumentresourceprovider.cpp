/****************************************************************************
**
** Copyright (C) 2020 Alexander Volkov <avolkov@astralinux.ru>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtextdocumentresourceprovider.h"

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

/*!
    \class QTextDocumentResourceProvider
    \inmodule QtGui
    \since 6.1
    \brief The QTextDocumentResourceProvider is the base class of resource providers for QTextDocument.

    Override resource() in a subclass, and set a subclass instance on a text document via
    QTextDocument::setResourceProvider, or on a label via QLabel::setResourceProvider. This
    allows customizing how resources are loaded in rich text documents without having to subclass
    QTextDocument or QLabel, respectively.

    \note An implementation should be thread-safe if it can be accessed from different threads,
    e.g. when the default resource provider lives in the main thread and a QTextDocument lives
    outside the main thread.
*/

static QAtomicPointer<QTextDocumentResourceProvider> qt_provider;

/*!
    Destroys the resource provider.
*/
QTextDocumentResourceProvider::~QTextDocumentResourceProvider()
{
    qt_provider.testAndSetRelease(this, nullptr);
}

/*!
    \fn virtual QVariant QTextDocumentResourceProvider::resource(const QUrl &url) = 0;

    Returns data specified by the \a url.

    \sa QTextDocument::loadResource
*/

/*!
    Returns the default resource provider.

    \sa QTextDocument::loadResource
*/
QTextDocumentResourceProvider *QTextDocumentResourceProvider::defaultProvider()
{
    return qt_provider.loadAcquire();
}

/*!
    Set the default resource provider to \a provider.

    \sa QTextDocument::loadResource
*/
void QTextDocumentResourceProvider::setDefaultProvider(QTextDocumentResourceProvider *provider)
{
    qt_provider.storeRelease(provider);
}

QT_END_NAMESPACE
