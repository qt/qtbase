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
    : cdata(false), reader(areader), domBuilder(adoc, &locator, namespaceProcessing)
{
}

QDomHandler::~QDomHandler() {}

bool QDomHandler::endDocument()
{
    return domBuilder.endDocument();
}

bool QDomHandler::startDTD(const QString &name, const QString &publicId, const QString &systemId)
{
    return domBuilder.startDTD(name, publicId, systemId);
}

bool QDomHandler::startElement(const QString &nsURI, const QString &, const QString &qName,
                               const QXmlAttributes &atts)
{
    return domBuilder.startElement(nsURI, qName, atts);
}

bool QDomHandler::endElement(const QString &, const QString &, const QString &)
{
    return domBuilder.endElement();
}

bool QDomHandler::characters(const QString &ch)
{
    return domBuilder.characters(ch, cdata);
}

bool QDomHandler::processingInstruction(const QString &target, const QString &data)
{
    return domBuilder.processingInstruction(target, data);
}

bool QDomHandler::skippedEntity(const QString &name)
{
    // we can only handle inserting entity references into content
    if (reader && !reader->d_ptr->skipped_entity_in_content)
        return true;

    return domBuilder.skippedEntity(name);
}

bool QDomHandler::fatalError(const QXmlParseException &exception)
{
    domBuilder.errorMsg = exception.message();
    domBuilder.errorLine = exception.lineNumber();
    domBuilder.errorColumn = exception.columnNumber();
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
    return domBuilder.startEntity(name);
}

bool QDomHandler::endEntity(const QString &)
{
    return domBuilder.endEntity();
}

bool QDomHandler::comment(const QString &ch)
{
    return domBuilder.comment(ch);
}

bool QDomHandler::unparsedEntityDecl(const QString &name, const QString &publicId,
                                     const QString &systemId, const QString &notationName)
{
    return domBuilder.unparsedEntityDecl(name, publicId, systemId, notationName);
}

bool QDomHandler::externalEntityDecl(const QString &name, const QString &publicId,
                                     const QString &systemId)
{
    return unparsedEntityDecl(name, publicId, systemId, QString());
}

bool QDomHandler::notationDecl(const QString &name, const QString &publicId,
                               const QString &systemId)
{
    return domBuilder.notationDecl(name, publicId, systemId);
}

void QDomHandler::setDocumentLocator(QXmlLocator *locator)
{
    this->locator.setLocator(locator);
}

QDomBuilder::ErrorInfo QDomHandler::errorInfo() const
{
    return domBuilder.error();
}

/**************************************************************
 *
 * QXmlDocumentLocators
 *
 **************************************************************/

void QSAXDocumentLocator::setLocator(QXmlLocator *l)
{
    locator = l;
}

int QSAXDocumentLocator::column() const
{
    if (!locator)
        return 0;

    return static_cast<int>(locator->columnNumber());
}

int QSAXDocumentLocator::line() const
{
    if (!locator)
        return 0;

    return static_cast<int>(locator->lineNumber());
}

/**************************************************************
 *
 * QDomBuilder
 *
 **************************************************************/

QDomBuilder::QDomBuilder(QDomDocumentPrivate *d, QXmlDocumentLocator *l, bool namespaceProcessing)
    : errorLine(0),
      errorColumn(0),
      doc(d),
      node(d),
      locator(l),
      nsProcessing(namespaceProcessing)
{
}

QDomBuilder::~QDomBuilder() {}

bool QDomBuilder::endDocument()
{
    // ### is this really necessary? (rms)
    if (node != doc)
        return false;
    return true;
}

bool QDomBuilder::startDTD(const QString &name, const QString &publicId, const QString &systemId)
{
    doc->doctype()->name = name;
    doc->doctype()->publicId = publicId;
    doc->doctype()->systemId = systemId;
    return true;
}

bool QDomBuilder::startElement(const QString &nsURI, const QString &qName,
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

    n->setLocation(locator->line(), locator->column());

    node->appendChild(n);
    node = n;

    // attributes
    for (int i = 0; i < atts.length(); i++) {
        auto domElement = static_cast<QDomElementPrivate *>(node);
        if (nsProcessing)
            domElement->setAttributeNS(atts.uri(i), atts.qName(i), atts.value(i));
        else
            domElement->setAttribute(atts.qName(i), atts.value(i));
    }

    return true;
}

bool QDomBuilder::endElement()
{
    if (!node || node == doc)
        return false;
    node = node->parent();

    return true;
}

bool QDomBuilder::characters(const QString &characters, bool cdata)
{
    // No text as child of some document
    if (node == doc)
        return false;

    QScopedPointer<QDomNodePrivate> n;
    if (cdata) {
        n.reset(doc->createCDATASection(characters));
    } else if (!entityName.isEmpty()) {
        QScopedPointer<QDomEntityPrivate> e(
                new QDomEntityPrivate(doc, nullptr, entityName, QString(), QString(), QString()));
        e->value = characters;
        e->ref.deref();
        doc->doctype()->appendChild(e.data());
        e.take();
        n.reset(doc->createEntityReference(entityName));
    } else {
        n.reset(doc->createTextNode(characters));
    }
    n->setLocation(locator->line(), locator->column());
    node->appendChild(n.data());
    n.take();

    return true;
}

bool QDomBuilder::processingInstruction(const QString &target, const QString &data)
{
    QDomNodePrivate *n;
    n = doc->createProcessingInstruction(target, data);
    if (n) {
        n->setLocation(locator->line(), locator->column());
        node->appendChild(n);
        return true;
    } else
        return false;
}

bool QDomBuilder::skippedEntity(const QString &name)
{
    QDomNodePrivate *n = doc->createEntityReference(name);
    n->setLocation(locator->line(), locator->column());
    node->appendChild(n);
    return true;
}

void QDomBuilder::fatalError(const QString &message)
{
    errorMsg = message;
    errorLine = static_cast<int>(locator->line());
    errorColumn = static_cast<int>(locator->column());
}

QDomBuilder::ErrorInfo QDomBuilder::error() const
{
    return ErrorInfo(errorMsg, errorLine, errorColumn);
}

bool QDomBuilder::startEntity(const QString &name)
{
    entityName = name;
    return true;
}

bool QDomBuilder::endEntity()
{
    entityName.clear();
    return true;
}

bool QDomBuilder::comment(const QString &characters)
{
    QDomNodePrivate *n;
    n = doc->createComment(characters);
    n->setLocation(locator->line(), locator->column());
    node->appendChild(n);
    return true;
}

bool QDomBuilder::unparsedEntityDecl(const QString &name, const QString &publicId,
                                     const QString &systemId, const QString &notationName)
{
    QDomEntityPrivate *e =
            new QDomEntityPrivate(doc, nullptr, name, publicId, systemId, notationName);
    // keep the refcount balanced: appendChild() does a ref anyway.
    e->ref.deref();
    doc->doctype()->appendChild(e);
    return true;
}

bool QDomBuilder::externalEntityDecl(const QString &name, const QString &publicId,
                                     const QString &systemId)
{
    return unparsedEntityDecl(name, publicId, systemId, QString());
}

bool QDomBuilder::notationDecl(const QString &name, const QString &publicId,
                               const QString &systemId)
{
    QDomNotationPrivate *n = new QDomNotationPrivate(doc, nullptr, name, publicId, systemId);
    // keep the refcount balanced: appendChild() does a ref anyway.
    n->ref.deref();
    doc->doctype()->appendChild(n);
    return true;
}

QT_END_NAMESPACE
