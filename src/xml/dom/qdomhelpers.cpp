/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtXml module of the Qt Toolkit.
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

#include "qdomhelpers_p.h"
#include "qdom_p.h"
#include "private/qxml_p.h"

QT_BEGIN_NAMESPACE

/**************************************************************
 *
 * QDomHandler
 *
 **************************************************************/

QDomHandler::QDomHandler(QDomDocumentPrivate *adoc, QXmlSimpleReader *areader,
                         bool namespaceProcessing)
    : errorLine(0),
      errorColumn(0),
      doc(adoc),
      node(adoc),
      cdata(false),
      nsProcessing(namespaceProcessing),
      locator(nullptr),
      reader(areader)
{
}

QDomHandler::~QDomHandler() {}

bool QDomHandler::endDocument()
{
    // ### is this really necessary? (rms)
    if (node != doc)
        return false;
    return true;
}

bool QDomHandler::startDTD(const QString &name, const QString &publicId, const QString &systemId)
{
    doc->doctype()->name = name;
    doc->doctype()->publicId = publicId;
    doc->doctype()->systemId = systemId;
    return true;
}

bool QDomHandler::startElement(const QString &nsURI, const QString &, const QString &qName,
                               const QXmlAttributes &atts)
{
    // tag name
    QDomNodePrivate *n;
    if (nsProcessing) {
        n = doc->createElementNS(nsURI, qName);
    } else {
        n = doc->createElement(qName);
    }

    if (!n)
        return false;

    n->setLocation(locator->lineNumber(), locator->columnNumber());

    node->appendChild(n);
    node = n;

    // attributes
    for (int i = 0; i < atts.length(); i++) {
        if (nsProcessing) {
            ((QDomElementPrivate *)node)->setAttributeNS(atts.uri(i), atts.qName(i), atts.value(i));
        } else {
            ((QDomElementPrivate *)node)->setAttribute(atts.qName(i), atts.value(i));
        }
    }

    return true;
}

bool QDomHandler::endElement(const QString &, const QString &, const QString &)
{
    if (!node || node == doc)
        return false;
    node = node->parent();

    return true;
}

bool QDomHandler::characters(const QString &ch)
{
    // No text as child of some document
    if (node == doc)
        return false;

    QScopedPointer<QDomNodePrivate> n;
    if (cdata) {
        n.reset(doc->createCDATASection(ch));
    } else if (!entityName.isEmpty()) {
        QScopedPointer<QDomEntityPrivate> e(
                new QDomEntityPrivate(doc, nullptr, entityName, QString(), QString(), QString()));
        e->value = ch;
        e->ref.deref();
        doc->doctype()->appendChild(e.data());
        e.take();
        n.reset(doc->createEntityReference(entityName));
    } else {
        n.reset(doc->createTextNode(ch));
    }
    n->setLocation(locator->lineNumber(), locator->columnNumber());
    node->appendChild(n.data());
    n.take();

    return true;
}

bool QDomHandler::processingInstruction(const QString &target, const QString &data)
{
    QDomNodePrivate *n;
    n = doc->createProcessingInstruction(target, data);
    if (n) {
        n->setLocation(locator->lineNumber(), locator->columnNumber());
        node->appendChild(n);
        return true;
    } else
        return false;
}

bool QDomHandler::skippedEntity(const QString &name)
{
    // we can only handle inserting entity references into content
    if (reader && !reader->d_ptr->skipped_entity_in_content)
        return true;

    QDomNodePrivate *n = doc->createEntityReference(name);
    n->setLocation(locator->lineNumber(), locator->columnNumber());
    node->appendChild(n);
    return true;
}

bool QDomHandler::fatalError(const QXmlParseException &exception)
{
    errorMsg = exception.message();
    errorLine = exception.lineNumber();
    errorColumn = exception.columnNumber();
    return QXmlDefaultHandler::fatalError(exception);
}

bool QDomHandler::startCDATA()
{
    cdata = true;
    return true;
}

bool QDomHandler::endCDATA()
{
    cdata = false;
    return true;
}

bool QDomHandler::startEntity(const QString &name)
{
    entityName = name;
    return true;
}

bool QDomHandler::endEntity(const QString &)
{
    entityName.clear();
    return true;
}

bool QDomHandler::comment(const QString &ch)
{
    QDomNodePrivate *n;
    n = doc->createComment(ch);
    n->setLocation(locator->lineNumber(), locator->columnNumber());
    node->appendChild(n);
    return true;
}

bool QDomHandler::unparsedEntityDecl(const QString &name, const QString &publicId,
                                     const QString &systemId, const QString &notationName)
{
    QDomEntityPrivate *e =
            new QDomEntityPrivate(doc, nullptr, name, publicId, systemId, notationName);
    // keep the refcount balanced: appendChild() does a ref anyway.
    e->ref.deref();
    doc->doctype()->appendChild(e);
    return true;
}

bool QDomHandler::externalEntityDecl(const QString &name, const QString &publicId,
                                     const QString &systemId)
{
    return unparsedEntityDecl(name, publicId, systemId, QString());
}

bool QDomHandler::notationDecl(const QString &name, const QString &publicId,
                               const QString &systemId)
{
    QDomNotationPrivate *n = new QDomNotationPrivate(doc, nullptr, name, publicId, systemId);
    // keep the refcount balanced: appendChild() does a ref anyway.
    n->ref.deref();
    doc->doctype()->appendChild(n);
    return true;
}

void QDomHandler::setDocumentLocator(QXmlLocator *locator)
{
    this->locator = locator;
}

QT_END_NAMESPACE
