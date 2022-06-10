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

#include <memory>
#include <stack>

QT_BEGIN_NAMESPACE

/**************************************************************
 *
 * QDomBuilder
 *
 **************************************************************/

QDomBuilder::QDomBuilder(QDomDocumentPrivate *d, QXmlStreamReader *r, bool namespaceProcessing)
    : errorLine(0),
      errorColumn(0),
      doc(d),
      node(d),
      reader(r),
      nsProcessing(namespaceProcessing)
{
    Q_ASSERT(doc);
    Q_ASSERT(reader);
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

QString QDomBuilder::dtdInternalSubset(const QString &dtd)
{
    // https://www.w3.org/TR/xml/#NT-intSubset
    // doctypedecl: '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'
    const QString &name = doc->doctype()->name;
    QStringView tmp = QStringView(dtd).sliced(dtd.indexOf(name) + name.size());

    const QString &publicId = doc->doctype()->publicId;
    if (!publicId.isEmpty())
        tmp = tmp.sliced(tmp.indexOf(publicId) + publicId.size());

    const QString &systemId = doc->doctype()->systemId;
    if (!systemId.isEmpty())
        tmp = tmp.sliced(tmp.indexOf(systemId) + systemId.size());

    const qsizetype obra = tmp.indexOf(u'[');
    const qsizetype cbra = tmp.lastIndexOf(u']');
    if (obra >= 0 && cbra >= 0)
        return tmp.left(cbra).sliced(obra + 1).toString();

    return QString();
}

bool QDomBuilder::parseDTD(const QString &dtd)
{
    doc->doctype()->internalSubset = dtdInternalSubset(dtd);
    return true;
}

bool QDomBuilder::startElement(const QString &nsURI, const QString &qName,
                               const QXmlStreamAttributes &atts)
{
    QDomNodePrivate *n =
            nsProcessing ? doc->createElementNS(nsURI, qName) : doc->createElement(qName);
    if (!n)
        return false;

    n->setLocation(int(reader->lineNumber()), int(reader->columnNumber()));

    node->appendChild(n);
    node = n;

    // attributes
    for (const auto &attr : atts) {
        auto domElement = static_cast<QDomElementPrivate *>(node);
        if (nsProcessing) {
            domElement->setAttributeNS(attr.namespaceUri().toString(),
                                       attr.qualifiedName().toString(),
                                       attr.value().toString());
        } else {
            domElement->setAttribute(attr.qualifiedName().toString(),
                                     attr.value().toString());
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

    std::unique_ptr<QDomNodePrivate> n;
    if (cdata) {
        n.reset(doc->createCDATASection(characters));
    } else if (!entityName.isEmpty()) {
        auto e = std::make_unique<QDomEntityPrivate>(
                    doc, nullptr, entityName, QString(), QString(), QString());
        e->value = characters;
        e->ref.deref();
        doc->doctype()->appendChild(e.get());
        Q_UNUSED(e.release());
        n.reset(doc->createEntityReference(entityName));
    } else {
        n.reset(doc->createTextNode(characters));
    }
    n->setLocation(int(reader->lineNumber()), int(reader->columnNumber()));
    node->appendChild(n.get());
    Q_UNUSED(n.release());

    return true;
}

bool QDomBuilder::processingInstruction(const QString &target, const QString &data)
{
    QDomNodePrivate *n;
    n = doc->createProcessingInstruction(target, data);
    if (n) {
        n->setLocation(int(reader->lineNumber()), int(reader->columnNumber()));
        node->appendChild(n);
        return true;
    } else
        return false;
}

bool QDomBuilder::skippedEntity(const QString &name)
{
    QDomNodePrivate *n = doc->createEntityReference(name);
    n->setLocation(int(reader->lineNumber()), int(reader->columnNumber()));
    node->appendChild(n);
    return true;
}

void QDomBuilder::fatalError(const QString &message)
{
    errorMsg = message;
    errorLine = static_cast<int>(reader->lineNumber());
    errorColumn = static_cast<int>(reader->columnNumber());
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
    n->setLocation(int(reader->lineNumber()), int(reader->columnNumber()));
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
    : reader(r), domBuilder(d, r, namespaceProcessing)
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

            if (!domBuilder.startDTD(reader->dtdName().toString(),
                                     reader->dtdPublicId().toString(),
                                     reader->dtdSystemId().toString())) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing document type declaration"));
                return false;
            }
            if (!domBuilder.parseDTD(reader->text().toString()))
                return false;
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

    std::stack<QString> tagStack;
    while (!reader->atEnd() && !reader->hasError()) {
        switch (reader->tokenType()) {
        case QXmlStreamReader::StartElement:
            tagStack.push(reader->qualifiedName().toString());
            if (!domBuilder.startElement(reader->namespaceUri().toString(),
                                         reader->qualifiedName().toString(),
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
                if (reader->isCDATA() || !reader->text().trimmed().isEmpty()) {
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
        // Entity declarations are created only for External Entities. Internal Entities
        // are parsed, and QXmlStreamReader handles the parsing itself and returns the
        // parsed result. So we don't need to do anything for the Internal Entities.
        if (!entityDecl.publicId().isEmpty() || !entityDecl.systemId().isEmpty()) {
            // External Entity
            if (!domBuilder.unparsedEntityDecl(entityDecl.name().toString(),
                                               entityDecl.publicId().toString(),
                                               entityDecl.systemId().toString(),
                                               entityDecl.notationName().toString())) {
                domBuilder.fatalError(
                        QDomParser::tr("Error occurred while processing entity declaration"));
                return false;
            }
        }
    }

    const auto notations = reader->notationDeclarations();
    for (const auto &notationDecl : notations) {
        if (!domBuilder.notationDecl(notationDecl.name().toString(),
                                     notationDecl.publicId().toString(),
                                     notationDecl.systemId().toString())) {
            domBuilder.fatalError(
                    QDomParser::tr("Error occurred while processing notation declaration"));
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_DOM
