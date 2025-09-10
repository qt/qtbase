// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <QtXml/qtxmlglobal.h>

#if QT_CONFIG(dom)

#include "qdomhelpers_p.h"
#include "qdom_p.h"
#include "qxmlstream.h"
#include "private/qxmlstream_p.h"

#include <memory>
#include <stack>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/**************************************************************
 *
 * QDomBuilder
 *
 **************************************************************/

QDomBuilder::QDomBuilder(QDomDocumentPrivate *d, QXmlStreamReader *r,
                         QDomDocument::ParseOptions options)
    : doc(d), node(d), reader(r), parseOptions(options)
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
    const bool nsProcessing =
            parseOptions.testFlag(QDomDocument::ParseOption::UseNamespaceProcessing);
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
    parseResult.errorMessage = message;
    parseResult.errorLine = reader->lineNumber();
    parseResult.errorColumn = reader->columnNumber();
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

QDomParser::QDomParser(QDomDocumentPrivate *d, QXmlStreamReader *r,
                       QDomDocument::ParseOptions options)
    : reader(r), domBuilder(d, r, options)
{
}

bool QDomParser::parse()
{
    return parseProlog() && parseBody();
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
                QString value(u"version='"_s);
                value += reader->documentVersion();
                value += u'\'';
                if (!reader->documentEncoding().isEmpty()) {
                    value += u" encoding='"_s;
                    value += reader->documentEncoding();
                    value += u'\'';
                }
                if (reader->isStandaloneDocument()) {
                    value += u" standalone='yes'"_s;
                } else {
                    // Add the standalone attribute only if it was specified
                    if (reader->hasStandaloneDeclaration())
                        value += u" standalone='no'"_s;
                }

                if (!domBuilder.processingInstruction(u"xml"_s, value)) {
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
            // Skip the content if it contains only spacing characters,
            // unless it's CDATA or PreserveSpacingOnlyNodes was specified.
            if (reader->isCDATA() || domBuilder.preserveSpacingOnlyNodes()
                || !(reader->isWhitespace() || reader->text().trimmed().isEmpty())) {
                if (!domBuilder.characters(reader->text().toString(), reader->isCDATA())) {
                    domBuilder.fatalError(
                            QDomParser::tr("Error occurred while processing the element content"));
                    return false;
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

#endif // feature dom
