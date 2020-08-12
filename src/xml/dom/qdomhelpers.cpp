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

#include <QtXml/qtxmlglobal.h>

#ifndef QT_NO_DOM

#include "qdomhelpers_p.h"
#include "qdom_p.h"
#include "qxmlstream.h"
#include "private/qxml_p.h"

QT_BEGIN_NAMESPACE

#if QT_DEPRECATED_SINCE(5, 15)

/**************************************************************
 *
 * QDomHandler
 *
 **************************************************************/
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
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
QT_WARNING_POP

#endif // QT_DEPRECATED_SINCE(5, 15)

/**************************************************************
 *
 * QXmlDocumentLocators
 *
 **************************************************************/

int QDomDocumentLocator::column() const
{
    Q_ASSERT(reader);
    return static_cast<int>(reader->columnNumber());
}

int QDomDocumentLocator::line() const
{
    Q_ASSERT(reader);
    return static_cast<int>(reader->lineNumber());
}

#if QT_DEPRECATED_SINCE(5, 15)

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

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

QT_WARNING_POP

#endif // QT_DEPRECATED_SINCE(5, 15)

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

#if QT_DEPRECATED_SINCE(5, 15)

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
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
QT_WARNING_POP

#endif // QT_DEPRECATED_SINCE(5, 15)

inline QString stringRefToString(const QStringRef &stringRef)
{
    // Calling QStringRef::toString() on a NULL QStringRef in some cases returns
    // an empty string (i.e. QString("")) instead of a NULL string (i.e. QString()).
    // QDom implementation differentiates between NULL and empty strings, so
    // we need this as workaround to keep the current behavior unchanged.
    return stringRef.isNull() ? QString() : stringRef.toString();
}

bool QDomBuilder::startElement(const QString &nsURI, const QString &qName,
                               const QXmlStreamAttributes &atts)
{
    QDomNodePrivate *n =
            nsProcessing ? doc->createElementNS(nsURI, qName) : doc->createElement(qName);
    if (!n)
        return false;

    n->setLocation(locator->line(), locator->column());

    node->appendChild(n);
    node = n;

    // attributes
    for (const auto &attr : atts) {
        auto domElement = static_cast<QDomElementPrivate *>(node);
        if (nsProcessing) {
            domElement->setAttributeNS(stringRefToString(attr.namespaceUri()),
                                       stringRefToString(attr.qualifiedName()),
                                       stringRefToString(attr.value()));
        } else {
            domElement->setAttribute(stringRefToString(attr.qualifiedName()),
                                     stringRefToString(attr.value()));
        }
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

/**************************************************************
 *
 * QDomParser
 *
 **************************************************************/

QDomParser::QDomParser(QDomDocumentPrivate *d, QXmlStreamReader *r, bool namespaceProcessing)
    : reader(r), locator(r), domBuilder(d, &locator, namespaceProcessing)
{
}

bool QDomParser::parse()
{
    return parseProlog() && parseBody();
}

QDomBuilder::ErrorInfo QDomParser::errorInfo() const
{
    return domBuilder.error();
}

bool QDomParser::parseProlog()
{
    Q_ASSERT(reader);

    bool foundDtd = false;

    while (!reader->atEnd()) {
        reader->readNext();

        if (reader->hasError()) {
            domBuilder.fatalError(reader->errorString());
            return false;
        }

        switch (reader->tokenType()) {
        case QXmlStreamReader::StartDocument:
            if (!reader->documentVersion().isEmpty()) {
                QString value(QLatin1String("version='"));
                value += reader->documentVersion();
                value += QLatin1Char('\'');
                if (!reader->documentEncoding().isEmpty()) {
                    value += QLatin1String(" encoding='");
                    value += reader->documentEncoding();
                    value += QLatin1Char('\'');
                }
                if (reader->isStandaloneDocument()) {
                    value += QLatin1String(" standalone='yes'");
                } else {
                    // TODO: Add standalone='no', if 'standalone' is specified. With the current
                    // QXmlStreamReader there is no way to figure out if it was specified or not.
                    // QXmlStreamReader needs to be modified for handling that case correctly.
                }

                if (!domBuilder.processingInstruction(QLatin1String("xml"), value)) {
                    domBuilder.fatalError(
                            QDomParser::tr("Error occurred while processing XML declaration"));
                    return false;
                }
            }
            break;
        case QXmlStreamReader::DTD:
            if (foundDtd) {
                domBuilder.fatalError(QDomParser::tr("Multiple DTD sections are not allowed"));
                return false;
            }
            foundDtd = true;

            if (!domBuilder.startDTD(stringRefToString(reader->dtdName()),
                                     stringRefToString(reader->dtdPublicId()),
                                     stringRefToString(reader->dtdSystemId()))) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing document type declaration"));
                return false;
            }
            if (!parseMarkupDecl())
                return false;
            break;
        case QXmlStreamReader::Comment:
            if (!domBuilder.comment(reader->text().toString())) {
                domBuilder.fatalError(QDomParser::tr("Error occurred while processing comment"));
                return false;
            }
            break;
        case QXmlStreamReader::ProcessingInstruction:
            if (!domBuilder.processingInstruction(reader->processingInstructionTarget().toString(),
                                                  reader->processingInstructionData().toString())) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing a processing instruction"));
                return false;
            }
            break;
        default:
            // If the token is none of the above, prolog processing is done.
            return true;
        }
    }

    return true;
}

bool QDomParser::parseBody()
{
    Q_ASSERT(reader);

    std::stack<QStringRef> tagStack;
    while (!reader->atEnd() && !reader->hasError()) {
        switch (reader->tokenType()) {
        case QXmlStreamReader::StartElement:
            tagStack.push(reader->qualifiedName());
            if (!domBuilder.startElement(stringRefToString(reader->namespaceUri()),
                                         stringRefToString(reader->qualifiedName()),
                                         reader->attributes())) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing a start element"));
                return false;
            }
            break;
        case QXmlStreamReader::EndElement:
            if (tagStack.empty() || reader->qualifiedName() != tagStack.top()) {
                domBuilder.fatalError(
                        QDomParser::tr("Unexpected end element '%1'").arg(reader->name()));
                return false;
            }
            tagStack.pop();
            if (!domBuilder.endElement()) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing an end element"));
                return false;
            }
            break;
        case QXmlStreamReader::Characters:
            if (!reader->isWhitespace()) { // Skip the content consisting of only whitespaces
                if (!reader->text().toString().trimmed().isEmpty()) {
                    if (!domBuilder.characters(reader->text().toString(), reader->isCDATA())) {
                        domBuilder.fatalError(QDomParser::tr(
                                "Error occurred while processing the element content"));
                        return false;
                    }
                }
            }
            break;
        case QXmlStreamReader::Comment:
            if (!domBuilder.comment(reader->text().toString())) {
                domBuilder.fatalError(QDomParser::tr("Error occurred while processing comments"));
                return false;
            }
            break;
        case QXmlStreamReader::ProcessingInstruction:
            if (!domBuilder.processingInstruction(reader->processingInstructionTarget().toString(),
                                                  reader->processingInstructionData().toString())) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing a processing instruction"));
                return false;
            }
            break;
        case QXmlStreamReader::EntityReference:
            if (!domBuilder.skippedEntity(reader->name().toString())) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing an entity reference"));
                return false;
            }
            break;
        default:
            domBuilder.fatalError(QDomParser::tr("Unexpected token"));
            return false;
        }

        reader->readNext();
    }

    if (reader->hasError()) {
        domBuilder.fatalError(reader->errorString());
        reader->readNext();
        return false;
    }

    if (!tagStack.empty()) {
        domBuilder.fatalError(QDomParser::tr("Tag mismatch"));
        return false;
    }

    return true;
}

bool QDomParser::parseMarkupDecl()
{
    Q_ASSERT(reader);

    const auto entities = reader->entityDeclarations();
    for (const auto &entityDecl : entities) {
        // Entity declarations are created only for Extrenal Entities. Internal Entities
        // are parsed, and QXmlStreamReader handles the parsing itself and returns the
        // parsed result. So we don't need to do anything for the Internal Entities.
        if (!entityDecl.publicId().isEmpty() || !entityDecl.systemId().isEmpty()) {
            // External Entity
            if (!domBuilder.unparsedEntityDecl(stringRefToString(entityDecl.name()),
                                               stringRefToString(entityDecl.publicId()),
                                               stringRefToString(entityDecl.systemId()),
                                               stringRefToString(entityDecl.notationName()))) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing entity declaration"));
                return false;
            }
        }
    }

    const auto notations = reader->notationDeclarations();
    for (const auto &notationDecl : notations) {
        if (!domBuilder.notationDecl(stringRefToString(notationDecl.name()),
                                     stringRefToString(notationDecl.publicId()),
                                     stringRefToString(notationDecl.systemId()))) {
            domBuilder.fatalError(
                    QDomParser::tr("Error occurred while processing notation declaration"));
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_DOM
