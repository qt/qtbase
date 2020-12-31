/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <qstringconverter.h>
#include <qxmlstream.h>
#include "qxmlstreamgrammar_p.h"

#include <memory>

#ifndef QXMLSTREAM_P_H
#define QXMLSTREAM_P_H

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class XmlStringRef
{
public:
    const QString *m_string = nullptr;
    qsizetype m_pos = 0;
    qsizetype m_size = 0;

    constexpr XmlStringRef() = default;
    constexpr inline XmlStringRef(const QString *string, int pos, int length)
        : m_string(string), m_pos(pos), m_size(length)
    {
    }
    XmlStringRef(const QString *string)
        : XmlStringRef(string, 0, string->length())
    {
    }

    operator QXmlString() const {
        if (!m_string)
            return QXmlString();
        QStringPrivate d = m_string->data_ptr();
        d.setBegin(d.data() + m_pos);
        d.size = m_size;
        return QXmlString(std::move(d));
    }
    operator QStringView() const { return view(); }

    void clear() { m_string = nullptr; m_pos = 0; m_size= 0; }
    QStringView view() const { return m_string ? QStringView(m_string->data() + m_pos, m_size) : QStringView(); }
    bool isEmpty() const { return m_size == 0; }
    bool isNull() const { return !m_string; }
    QString toString() const { return view().toString(); }

#define MAKE_OP(op) \
    friend auto operator op(const XmlStringRef &lhs, const XmlStringRef &rhs) noexcept { return lhs.view() op rhs.view(); } \
    /*end*/
    MAKE_OP(==)
    MAKE_OP(!=)
    MAKE_OP(<=)
    MAKE_OP(>=)
    MAKE_OP(<)
    MAKE_OP(>)
#undef MAKE_OP
#define MAKE_OP(op) \
    friend auto operator op(const XmlStringRef &lhs, QStringView rhs) noexcept { return lhs.view() op rhs; } \
    friend auto operator op(QStringView lhs, const XmlStringRef &rhs) noexcept { return lhs op rhs.view(); } \
    /*end*/
    MAKE_OP(==)
    MAKE_OP(!=)
    MAKE_OP(<=)
    MAKE_OP(>=)
    MAKE_OP(<)
    MAKE_OP(>)
#undef MAKE_OP
};

}

using namespace QtPrivate;

template <typename T> class QXmlStreamSimpleStack
{
    T *data;
    qsizetype tos, cap;
public:
    inline QXmlStreamSimpleStack()
        : data(nullptr), tos(-1), cap(0)
    {}
    inline ~QXmlStreamSimpleStack()
    {
        if (data) {
            std::destroy_n(data, size());
            free(data);
        }
    }

    inline void reserve(qsizetype extraCapacity)
    {
        if (tos + extraCapacity + 1 > cap) {
            cap = qMax(tos + extraCapacity + 1, cap << 1 );
            void *ptr = realloc(static_cast<void *>(data), cap * sizeof(T));
            data = reinterpret_cast<T *>(ptr);
            Q_CHECK_PTR(data);
        }
    }

    inline T &push() { reserve(1); return rawPush(); }
    inline T &rawPush() {  return *new (data + (++tos)) T; }
    inline const T &top() const { return data[tos]; }
    inline T &top() { return data[tos]; }
    inline T pop() { T t = std::move(data[tos]); std::destroy_at(data + tos); --tos; return t; }
    inline T &operator[](qsizetype index) { return data[index]; }
    inline const T &at(qsizetype index) const { return data[index]; }
    inline qsizetype size() const { return tos + 1; }
    inline void resize(qsizetype s) { tos = s - 1; }
    inline bool isEmpty() const { return tos < 0; }
    inline void clear() { tos = -1; }

    using const_iterator = const T*;
    using iterator = T*;
    T *begin() { return data; }
    const T *begin() const { return data; }
    const T *cbegin() const { return begin(); }
    T *end() { return data + size(); }
    const T *end() const { return data + size(); }
    const T *cend() const { return end(); }
};


class QXmlStream
{
    Q_DECLARE_TR_FUNCTIONS(QXmlStream)
};

class QXmlStreamPrivateTagStack {
public:
    struct NamespaceDeclaration
    {
        XmlStringRef prefix;
        XmlStringRef namespaceUri;
    };

    struct Tag
    {
        XmlStringRef name;
        XmlStringRef qualifiedName;
        NamespaceDeclaration namespaceDeclaration;
        int tagStackStringStorageSize;
        qsizetype namespaceDeclarationsSize;
    };


    QXmlStreamPrivateTagStack();
    QXmlStreamSimpleStack<NamespaceDeclaration> namespaceDeclarations;
    QString tagStackStringStorage;
    int tagStackStringStorageSize;
    int initialTagStackStringStorageSize;
    bool tagsDone;

    XmlStringRef addToStringStorage(QStringView s)
    {
        int pos = tagStackStringStorageSize;
        int sz = s.size();
        if (pos != tagStackStringStorage.size())
            tagStackStringStorage.resize(pos);
        tagStackStringStorage.append(s.data(), sz);
        tagStackStringStorageSize += sz;
        return XmlStringRef(&tagStackStringStorage, pos, sz);
    }

    QXmlStreamSimpleStack<Tag> tagStack;


    inline Tag tagStack_pop() {
        Tag tag = tagStack.pop();
        tagStackStringStorageSize = tag.tagStackStringStorageSize;
        namespaceDeclarations.resize(tag.namespaceDeclarationsSize);
        tagsDone = tagStack.isEmpty();
        return tag;
    }
    inline Tag &tagStack_push() {
        Tag &tag = tagStack.push();
        tag.tagStackStringStorageSize = tagStackStringStorageSize;
        tag.namespaceDeclarationsSize = namespaceDeclarations.size();
        return tag;
    }
};


class QXmlStreamEntityResolver;
class QXmlStreamReaderPrivate : public QXmlStreamGrammar, public QXmlStreamPrivateTagStack
{
    QXmlStreamReader *q_ptr;
    Q_DECLARE_PUBLIC(QXmlStreamReader)
public:
    QXmlStreamReaderPrivate(QXmlStreamReader *q);
    ~QXmlStreamReaderPrivate();
    void init();

    QByteArray rawReadBuffer;
    QByteArray dataBuffer;
    uchar firstByte;
    qint64 nbytesread;
    QString readBuffer;
    int readBufferPos;
    QXmlStreamSimpleStack<uint> putStack;
    struct Entity {
        Entity() = default;
        Entity(const QString &name, const QString &value)
          :  name(name), value(value), external(false), unparsed(false), literal(false),
             hasBeenParsed(false), isCurrentlyReferenced(false){}
        static inline Entity createLiteral(QLatin1String name, QLatin1String value)
            { Entity result(name, value); result.literal = result.hasBeenParsed = true; return result; }
        QString name, value;
        uint external : 1;
        uint unparsed : 1;
        uint literal : 1;
        uint hasBeenParsed : 1;
        uint isCurrentlyReferenced : 1;
    };
    // these hash tables use a QStringView as a key to avoid creating QStrings
    // just for lookup. The keys are usually views into Entity::name and thus
    // are guaranteed to have the same lifetime as the referenced data:
    QHash<QStringView, Entity> entityHash;
    QHash<QStringView, Entity> parameterEntityHash;
    struct QEntityReference
    {
        QHash<QStringView, Entity> *hash;
        QStringView name;
    };
    QXmlStreamSimpleStack<QEntityReference> entityReferenceStack;
    int entityExpansionLimit = 4096;
    int entityLength = 0;
    inline bool referenceEntity(QHash<QStringView, Entity> *hash, Entity &entity)
    {
        Q_ASSERT(hash);
        if (entity.isCurrentlyReferenced) {
            raiseWellFormedError(QXmlStream::tr("Self-referencing entity detected."));
            return false;
        }
        // entityLength represents the amount of additional characters the
        // entity expands into (can be negative for e.g. &amp;). It's used to
        // avoid DoS attacks through recursive entity expansions
        entityLength += entity.value.size() - entity.name.size() - 2;
        if (entityLength > entityExpansionLimit) {
            raiseWellFormedError(QXmlStream::tr("Entity expands to more characters than the entity expansion limit."));
            return false;
        }
        entity.isCurrentlyReferenced = true;
        entityReferenceStack.push() = { hash, entity.name };
        injectToken(ENTITY_DONE);
        return true;
    }


    QIODevice *device;
    bool deleteDevice;
    QStringDecoder decoder;
    bool atEnd;

    /*!
      \sa setType()
     */
    QXmlStreamReader::TokenType type;
    QXmlStreamReader::Error error;
    QString errorString;
    QString unresolvedEntity;

    qint64 lineNumber, lastLineStart, characterOffset;


    void write(const QString &);
    void write(const char *);


    QXmlStreamAttributes attributes;
    XmlStringRef namespaceForPrefix(QStringView prefix);
    void resolveTag();
    void resolvePublicNamespaces();
    void resolveDtd();
    uint resolveCharRef(int symbolIndex);
    bool checkStartDocument();
    void startDocument();
    void parseError();
    void checkPublicLiteral(QStringView publicId);

    bool scanDtd;
    XmlStringRef lastAttributeValue;
    bool lastAttributeIsCData;
    struct DtdAttribute {
        XmlStringRef tagName;
        XmlStringRef attributeQualifiedName;
        XmlStringRef attributePrefix;
        XmlStringRef attributeName;
        XmlStringRef defaultValue;
        bool isCDATA;
        bool isNamespaceAttribute;
    };
    QXmlStreamSimpleStack<DtdAttribute> dtdAttributes;
    struct NotationDeclaration {
        XmlStringRef name;
        XmlStringRef publicId;
        XmlStringRef systemId;
    };
    QXmlStreamSimpleStack<NotationDeclaration> notationDeclarations;
    QXmlStreamNotationDeclarations publicNotationDeclarations;
    QXmlStreamNamespaceDeclarations publicNamespaceDeclarations;

    struct EntityDeclaration {
        XmlStringRef name;
        XmlStringRef notationName;
        XmlStringRef publicId;
        XmlStringRef systemId;
        XmlStringRef value;
        bool parameter;
        bool external;
        inline void clear() {
            name.clear();
            notationName.clear();
            publicId.clear();
            systemId.clear();
            value.clear();
            parameter = external = false;
        }
    };
    QXmlStreamSimpleStack<EntityDeclaration> entityDeclarations;
    QXmlStreamEntityDeclarations publicEntityDeclarations;

    XmlStringRef text;

    XmlStringRef prefix, namespaceUri, qualifiedName, name;
    XmlStringRef processingInstructionTarget, processingInstructionData;
    XmlStringRef dtdName, dtdPublicId, dtdSystemId;
    XmlStringRef documentVersion, documentEncoding;
    uint isEmptyElement : 1;
    uint isWhitespace : 1;
    uint isCDATA : 1;
    uint standalone : 1;
    uint hasCheckedStartDocument : 1;
    uint normalizeLiterals : 1;
    uint hasSeenTag : 1;
    uint inParseEntity : 1;
    uint referenceToUnparsedEntityDetected : 1;
    uint referenceToParameterEntityDetected : 1;
    uint hasExternalDtdSubset : 1;
    uint lockEncoding : 1;
    uint namespaceProcessing : 1;

    int resumeReduction;
    void resume(int rule);

    inline bool entitiesMustBeDeclared() const {
        return (!inParseEntity
                && (standalone
                    || (!referenceToUnparsedEntityDetected
                        && !referenceToParameterEntityDetected // Errata 13 as of 2006-04-25
                        && !hasExternalDtdSubset)));
    }

    // qlalr parser
    int tos;
    int stack_size;
    struct Value {
        int pos;
        int len;
        int prefix;
        ushort c;
    };

    Value *sym_stack;
    int *state_stack;
    inline void reallocateStack();
    inline Value &sym(int index) const
    { return sym_stack[tos + index - 1]; }
    QString textBuffer;
    inline void clearTextBuffer() {
        if (!scanDtd) {
            textBuffer.resize(0);
            textBuffer.reserve(256);
        }
    }
    struct Attribute {
        Value key;
        Value value;
    };
    QXmlStreamSimpleStack<Attribute> attributeStack;

    inline XmlStringRef symString(int index) {
        const Value &symbol = sym(index);
        return XmlStringRef(&textBuffer, symbol.pos + symbol.prefix, symbol.len - symbol.prefix);
    }
    QStringView symView(int index) const
    {
        const Value &symbol = sym(index);
        return QStringView(textBuffer.data() + symbol.pos, symbol.len).mid(symbol.prefix);
    }
    inline XmlStringRef symName(int index) {
        const Value &symbol = sym(index);
        return XmlStringRef(&textBuffer, symbol.pos, symbol.len);
    }
    inline XmlStringRef symString(int index, int offset) {
        const Value &symbol = sym(index);
        return XmlStringRef(&textBuffer, symbol.pos + symbol.prefix + offset, symbol.len - symbol.prefix -  offset);
    }
    inline XmlStringRef symPrefix(int index) {
        const Value &symbol = sym(index);
        if (symbol.prefix)
            return XmlStringRef(&textBuffer, symbol.pos, symbol.prefix - 1);
        return XmlStringRef();
    }
    inline XmlStringRef symString(const Value &symbol) {
        return XmlStringRef(&textBuffer, symbol.pos + symbol.prefix, symbol.len - symbol.prefix);
    }
    inline XmlStringRef symName(const Value &symbol) {
        return XmlStringRef(&textBuffer, symbol.pos, symbol.len);
    }
    inline XmlStringRef symPrefix(const Value &symbol) {
        if (symbol.prefix)
            return XmlStringRef(&textBuffer, symbol.pos, symbol.prefix - 1);
        return XmlStringRef();
    }

    inline void clearSym() { Value &val = sym(1); val.pos = textBuffer.size(); val.len = 0; }


    short token;
    uint token_char;

    uint filterCarriageReturn();
    inline uint getChar();
    inline uint peekChar();
    inline void putChar(uint c) { putStack.push() = c; }
    inline void putChar(QChar c) { putStack.push() =  c.unicode(); }
    void putString(QStringView s, qsizetype from = 0);
    void putStringLiteral(QStringView s);
    void putReplacement(QStringView s);
    void putReplacementInAttributeValue(QStringView s);
    uint getChar_helper();

    bool scanUntil(const char *str, short tokenToInject = -1);
    bool scanString(const char *str, short tokenToInject, bool requireSpace = true);
    inline void injectToken(ushort tokenToInject) {
        putChar(int(tokenToInject) << 16);
    }

    QString resolveUndeclaredEntity(const QString &name);
    void parseEntity(const QString &value);
    std::unique_ptr<QXmlStreamReaderPrivate> entityParser;

    bool scanAfterLangleBang();
    bool scanPublicOrSystem();
    bool scanNData();
    bool scanAfterDefaultDecl();
    bool scanAttType();


    // scan optimization functions. Not strictly necessary but LALR is
    // not very well suited for scanning fast
    int fastScanLiteralContent();
    int fastScanSpace();
    int fastScanContentCharList();
    int fastScanName(int *prefix = nullptr);
    inline int fastScanNMTOKEN();


    bool parse();
    inline void consumeRule(int);

    void raiseError(QXmlStreamReader::Error error, const QString& message = QString());
    void raiseWellFormedError(const QString &message);

    QXmlStreamEntityResolver *entityResolver;

private:
    /*! \internal
       Never assign to variable type directly. Instead use this function.

       This prevents errors from being ignored.
     */
    inline void setType(const QXmlStreamReader::TokenType t)
    {
        if (type != QXmlStreamReader::Invalid)
            type = t;
    }
};

QT_END_NAMESPACE

#endif // QXMLSTREAM_P_H

