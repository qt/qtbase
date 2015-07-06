/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef NODE_H
#define NODE_H

#include <qdir.h>
#include <qmap.h>
#include <qpair.h>
#include <qstringlist.h>

#include "codechunk.h"
#include "doc.h"

QT_BEGIN_NAMESPACE

class Node;
class Tree;
class EnumNode;
class ClassNode;
class Aggregate;
class ExampleNode;
class TypedefNode;
class QmlTypeNode;
class QDocDatabase;
class FunctionNode;
class PropertyNode;
class CollectionNode;
class QmlPropertyNode;

typedef QList<Node*> NodeList;
typedef QList<PropertyNode*> PropNodeList;
typedef QMap<QString, Node*> NodeMap;
typedef QMultiMap<QString, Node*> NodeMultiMap;
typedef QPair<int, int> NodeTypePair;
typedef QList<NodeTypePair> NodeTypeList;
typedef QMap<QString, CollectionNode*> CNMap;
typedef QMultiMap<QString, CollectionNode*> CNMultiMap;

class Node
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Node)

public:
    enum NodeType {
        NoType,
        Namespace,
        Class,
        Document,
        Enum,
        Typedef,
        Function,
        Property,
        Variable,
        Group,
        Module,
        QmlType,
        QmlModule,
        QmlPropertyGroup,
        QmlProperty,
        QmlSignal,
        QmlSignalHandler,
        QmlMethod,
        QmlBasicType,
        LastType
    };

    enum DocSubtype {
        NoSubtype,
        Example,
        HeaderFile,
        File,
        Image,
        Page,
        ExternalPage,
        DitaMap,
        LastSubtype
    };

    enum Genus { DontCare, CPP, JS, QML, DOC };

    enum Access { Public, Protected, Private };

    enum Status {
        Compat,
        Obsolete,
        Deprecated,
        Preliminary,
        Active,
        Internal,
        Intermediate
    }; // don't reorder this enum

    enum ThreadSafeness {
        UnspecifiedSafeness,
        NonReentrant,
        Reentrant,
        ThreadSafe
    };

    enum LinkType {
        StartLink,
        NextLink,
        PreviousLink,
        ContentsLink,
        IndexLink
    };

    enum PageType {
        NoPageType,
        ApiPage,
        ArticlePage,
        ExamplePage,
        HowToPage,
        OverviewPage,
        TutorialPage,
        FAQPage,
        DitaMapPage,
        OnBeyondZebra
    };

    enum FlagValue {
        FlagValueDefault = -1,
        FlagValueFalse = 0,
        FlagValueTrue = 1
    };

    virtual ~Node();

    QString plainName() const;
    QString plainFullName(const Node* relative = 0) const;
    QString fullName(const Node* relative=0) const;

    const QString& fileNameBase() const { return fileNameBase_; }
    bool hasFileNameBase() const { return !fileNameBase_.isEmpty(); }
    void setFileNameBase(const QString& t) { fileNameBase_ = t; }
    Node::Genus genus() const { return (Genus) genus_; }
    void setGenus(Genus t) { genus_ = (unsigned char) t; }

    void setAccess(Access t) { access_ = (unsigned char) t; }
    void setLocation(const Location& t);
    void setDoc(const Doc& doc, bool replace = false);
    void setStatus(Status t) {
        if (status_ == (unsigned char) Obsolete && t == Deprecated)
            return;
        status_ = (unsigned char) t;
    }
    void setThreadSafeness(ThreadSafeness t) { safeness_ = (unsigned char) t; }
    void setSince(const QString &since);
    void setRelates(Aggregate* pseudoParent);
    void setPhysicalModuleName(const QString &name) { physicalModuleName_ = name; }
    void setUrl(const QString& url) { url_ = url; }
    void setTemplateStuff(const QString &t) { templateStuff_ = t; }
    void setReconstitutedBrief(const QString &t) { reconstitutedBrief_ = t; }
    void setPageType(PageType t) { pageType_ = (unsigned char) t; }
    void setPageType(const QString& t);
    void setParent(Aggregate* n) { parent_ = n; }
    void setIndexNodeFlag() { indexNodeFlag_ = true; }
    virtual void setOutputFileName(const QString& ) { }

    bool isQmlNode() const { return genus() == QML; }
    bool isJsNode() const { return genus() == JS; }
    bool isCppNode() const { return genus() == CPP; }

    virtual bool isAggregate() const = 0;
    virtual bool isCollectionNode() const { return false; }
    virtual bool isDocumentNode() const { return false; }
    virtual bool isGroup() const { return false; }
    virtual bool isModule() const { return false; }
    virtual bool isQmlModule() const { return false; }
    virtual bool isJsModule() const { return false; }
    virtual bool isQmlType() const { return false; }
    virtual bool isJsType() const { return false; }
    virtual bool isQmlBasicType() const { return false; }
    virtual bool isJsBasicType() const { return false; }
    virtual bool isEnumType() const { return false; }
    virtual bool isExample() const { return false; }
    virtual bool isExampleFile() const { return false; }
    virtual bool isHeaderFile() const { return false; }
    virtual bool isLeaf() const { return false; }
    virtual bool isReimplemented() const { return false; }
    virtual bool isFunction() const { return false; }
    virtual bool isNamespace() const { return false; }
    virtual bool isClass() const { return false; }
    virtual bool isQtQuickNode() const { return false; }
    virtual bool isAbstract() const { return false; }
    virtual bool isProperty() const { return false; }
    virtual bool isQmlProperty() const { return false; }
    virtual bool isJsProperty() const { return false; }
    virtual bool isQmlPropertyGroup() const { return false; }
    virtual bool isJsPropertyGroup() const { return false; }
    virtual bool isQmlSignal() const { return false; }
    virtual bool isJsSignal() const { return false; }
    virtual bool isQmlSignalHandler() const { return false; }
    virtual bool isJsSignalHandler() const { return false; }
    virtual bool isQmlMethod() const { return false; }
    virtual bool isJsMethod() const { return false; }
    virtual bool isAttached() const { return false; }
    virtual bool isAlias() const { return false; }
    virtual bool isWrapper() const;
    virtual bool isReadOnly() const { return false; }
    virtual bool isDefault() const { return false; }
    virtual bool isExternalPage() const { return false; }
    virtual void addMember(Node* ) { }
    virtual bool hasMembers() const { return false; }
    virtual bool hasNamespaces() const { return false; }
    virtual bool hasClasses() const { return false; }
    virtual void setAbstract(bool ) { }
    virtual void setWrapper() { }
    virtual QString title() const { return name(); }
    virtual QString fullTitle() const { return name(); }
    virtual QString subTitle() const { return QString(); }
    virtual void setTitle(const QString& ) { }
    virtual void setSubTitle(const QString& ) { }
    virtual QmlPropertyNode* hasQmlProperty(const QString& ) const { return 0; }
    virtual QmlPropertyNode* hasQmlProperty(const QString&, bool ) const { return 0; }
    virtual void getMemberNamespaces(NodeMap& ) { }
    virtual void getMemberClasses(NodeMap& ) { }
    virtual bool isInternal() const;
    virtual void setDataType(const QString& ) { }
    virtual void setReadOnly(bool ) { }
    virtual Node* disambiguate(NodeType , DocSubtype ) { return this; }
    virtual bool wasSeen() const { return false; }
    virtual void appendGroupName(const QString& ) { }
    virtual QString element() const { return QString(); }
    virtual Tree* tree() const;
    virtual void findChildren(const QString& , NodeList& nodes) const { nodes.clear(); }
    virtual void setNoAutoList(bool ) { }
    bool isIndexNode() const { return indexNodeFlag_; }
    NodeType type() const { return (NodeType) nodeType_; }
    virtual DocSubtype docSubtype() const { return NoSubtype; }
    bool match(const NodeTypeList& types) const;
    Aggregate* parent() const { return parent_; }
    const Node* root() const;
    Aggregate* relates() const { return relatesTo_; }
    const QString& name() const { return name_; }
    QString physicalModuleName() const;
    QString url() const { return url_; }
    virtual QString nameForLists() const { return name_; }
    virtual QString outputFileName() const { return QString(); }
    virtual QString obsoleteLink() const { return QString(); }
    virtual void setObsoleteLink(const QString& ) { };
    virtual void setQtVariable(const QString& ) { }
    virtual QString qtVariable() const { return QString(); }

    const QMap<LinkType, QPair<QString,QString> >& links() const { return linkMap_; }
    void setLink(LinkType linkType, const QString &link, const QString &desc);

    Access access() const { return (Access) access_; }
    bool isPrivate() const { return (Access) access_ == Private; }
    QString accessString() const;
    const Location& declLocation() const { return declLocation_; }
    const Location& defLocation() const { return defLocation_; }
    const Location& location() const { return (defLocation_.isEmpty() ? declLocation_ : defLocation_); }
    const Doc& doc() const { return doc_; }
    bool hasDoc() const { return !doc_.isEmpty(); }
    Status status() const { return (Status) status_; }
    Status inheritedStatus() const;
    bool isObsolete() const { return (status_ == (unsigned char) Obsolete); }
    ThreadSafeness threadSafeness() const;
    ThreadSafeness inheritedThreadSafeness() const;
    QString since() const { return since_; }
    QString templateStuff() const { return templateStuff_; }
    const QString& reconstitutedBrief() const { return reconstitutedBrief_; }
    PageType pageType() const { return (PageType) pageType_; }
    QString pageTypeString() const;
    QString nodeTypeString() const;
    QString nodeSubtypeString() const;
    virtual void addPageKeywords(const QString& ) { }

    void clearRelated() { relatesTo_ = 0; }

    //QString guid() const;
    QString extractClassName(const QString &string) const;
    virtual QString qmlTypeName() const { return name_; }
    virtual QString qmlFullBaseName() const { return QString(); }
    virtual QString logicalModuleName() const { return QString(); }
    virtual QString logicalModuleVersion() const { return QString(); }
    virtual QString logicalModuleIdentifier() const { return QString(); }
    virtual void setLogicalModuleInfo(const QString& ) { }
    virtual void setLogicalModuleInfo(const QStringList& ) { }
    virtual CollectionNode* logicalModule() const { return 0; }
    virtual void setQmlModule(CollectionNode* ) { }
    virtual ClassNode* classNode() { return 0; }
    virtual void setClassNode(ClassNode* ) { }
    virtual const Node* applyModuleName(const Node* ) const { return 0; }
    virtual QString idNumber() { return "0"; }
    QmlTypeNode* qmlTypeNode();
    ClassNode* declarativeCppNode();
    const QString& outputSubdirectory() const { return outSubDir_; }
    virtual void setOutputSubdirectory(const QString& t) { outSubDir_ = t; }
    QString fullDocumentName() const;
    static QString cleanId(const QString &str);
    //QString idForNode() const;

    static FlagValue toFlagValue(bool b);
    static bool fromFlagValue(FlagValue fv, bool defaultValue);

    static QString pageTypeString(unsigned char t);
    static QString nodeTypeString(unsigned char t);
    static QString nodeSubtypeString(unsigned char t);
    static int incPropertyGroupCount();
    static void clearPropertyGroupCount();
    static void initialize();
    static NodeType goal(const QString& t) { return goals_.value(t); }

protected:
    Node(NodeType type, Aggregate* parent, const QString& name);

private:

    unsigned char nodeType_;
    unsigned char genus_;
    unsigned char access_;
    unsigned char safeness_;
    unsigned char pageType_;
    unsigned char status_;
    bool indexNodeFlag_;

    Aggregate* parent_;
    Aggregate* relatesTo_;
    QString name_;
    Location declLocation_;
    Location defLocation_;
    Doc doc_;
    QMap<LinkType, QPair<QString, QString> > linkMap_;
    QString fileNameBase_;
    QString physicalModuleName_;
    QString url_;
    QString since_;
    QString templateStuff_;
    QString reconstitutedBrief_;
    //mutable QString uuid_;
    QString outSubDir_;
    static QStringMap operators_;
    static int propertyGroupCount_;
    static QMap<QString,Node::NodeType> goals_;
};

class Aggregate : public Node
{
public:
    virtual ~Aggregate();

    Node* findChildNode(const QString& name, Node::Genus genus) const;
    Node* findChildNode(const QString& name, NodeType type);
    virtual void findChildren(const QString& name, NodeList& nodes) const Q_DECL_OVERRIDE;
    FunctionNode* findFunctionNode(const QString& name) const;
    FunctionNode* findFunctionNode(const FunctionNode* clone) const;
    void addInclude(const QString &include);
    void setIncludes(const QStringList &includes);
    void normalizeOverloads();
    void makeUndocumentedChildrenInternal();
    void deleteChildren();
    void removeFromRelated();

    virtual bool isAggregate() const Q_DECL_OVERRIDE { return true; }
    virtual bool isLeaf() const Q_DECL_OVERRIDE { return false; }
    const EnumNode* findEnumNodeForValue(const QString &enumValue) const;
    const NodeList & childNodes() const { return children_; }
    const NodeList & relatedNodes() const { return related_; }

    int count() const { return children_.size(); }
    NodeList overloads(const QString &funcName) const;
    const QStringList& includes() const { return includes_; }

    QStringList primaryKeys();
    QStringList secondaryKeys();
    const QStringList& pageKeywords() const { return pageKeywds; }
    virtual void addPageKeywords(const QString& t) Q_DECL_OVERRIDE { pageKeywds << t; }
    virtual void setOutputFileName(const QString& f) Q_DECL_OVERRIDE { outputFileName_ = f; }
    virtual QString outputFileName() const Q_DECL_OVERRIDE { return outputFileName_; }
    virtual QmlPropertyNode* hasQmlProperty(const QString& ) const Q_DECL_OVERRIDE;
    virtual QmlPropertyNode* hasQmlProperty(const QString&, bool attached) const Q_DECL_OVERRIDE;
    void addChild(Node* child, const QString& title);
    const QStringList& groupNames() const { return groupNames_; }
    virtual void appendGroupName(const QString& t) Q_DECL_OVERRIDE { groupNames_.append(t); }
    void printChildren(const QString& title);
    void addChild(Node* child);
    void removeChild(Node* child);
    void setOutputSubdirectory(const QString& t) Q_DECL_OVERRIDE;
    const NodeMap& primaryFunctionMap() { return primaryFunctionMap_; }
    const QMap<QString, NodeList>& secondaryFunctionMap() { return secondaryFunctionMap_; }

protected:
    Aggregate(NodeType type, Aggregate* parent, const QString& name);

private:
    friend class Node;

    static bool isSameSignature(const FunctionNode* f1, const FunctionNode* f2);
    void removeRelated(Node* pseudoChild);

    QString outputFileName_;
    QStringList pageKeywds;
    QStringList includes_;
    QStringList groupNames_;
    NodeList children_;
    NodeList enumChildren_;
    NodeList related_;
    NodeMap childMap_;
    NodeMap primaryFunctionMap_;
    QMap<QString, NodeList> secondaryFunctionMap_;
};

class LeafNode : public Node
{
public:
    LeafNode();
    virtual ~LeafNode() { }

    virtual bool isAggregate() const Q_DECL_OVERRIDE { return false; }
    virtual bool isLeaf() const Q_DECL_OVERRIDE { return true; }

protected:
    LeafNode(NodeType type, Aggregate* parent, const QString& name);
    LeafNode(Aggregate* parent, NodeType type, const QString& name);
};

class NamespaceNode : public Aggregate
{
public:
    NamespaceNode(Aggregate* parent, const QString& name);
    virtual ~NamespaceNode() { }
    virtual bool isNamespace() const Q_DECL_OVERRIDE { return true; }
    virtual Tree* tree() const Q_DECL_OVERRIDE { return (parent() ? parent()->tree() : tree_); }
    virtual bool wasSeen() const Q_DECL_OVERRIDE { return seen_; }

    void markSeen() { seen_ = true; }
    void markNotSeen() { seen_ = false; }
    void setTree(Tree* t) { tree_ = t; }
    const NodeList& orphans() const { return orphans_; }
    void addOrphan(Node* child) { orphans_.append(child); }

 private:
    bool        seen_;
    Tree*       tree_;
    NodeList    orphans_;
};

struct RelatedClass
{
    RelatedClass() { }
    // constructor for resolved base class
    RelatedClass(Node::Access access, ClassNode* node)
        : access_(access), node_(node) { }
    // constructor for unresolved base class
    RelatedClass(Node::Access access, const QStringList& path, const QString& signature)
        : access_(access), node_(0), path_(path), signature_(signature) { }
    QString accessString() const;
    bool isPrivate() const { return (access_ == Node::Private); }

    Node::Access        access_;
    ClassNode*          node_;
    QStringList         path_;
    QString             signature_;
};

struct UsingClause
{
    UsingClause() { }
    UsingClause(const QString& signature) : node_(0), signature_(signature) { }
    const QString& signature() const { return signature_; }
    const Node* node() { return node_; }
    void setNode(const Node* n) { node_ = n; }

    const Node* node_;
    QString     signature_;
};

class ClassNode : public Aggregate
{
public:
    ClassNode(Aggregate* parent, const QString& name);
    virtual ~ClassNode() { }
    virtual bool isClass() const Q_DECL_OVERRIDE { return true; }
    virtual bool isWrapper() const Q_DECL_OVERRIDE { return wrapper_; }
    virtual QString obsoleteLink() const Q_DECL_OVERRIDE { return obsoleteLink_; }
    virtual void setObsoleteLink(const QString& t) Q_DECL_OVERRIDE { obsoleteLink_ = t; }
    virtual void setWrapper() Q_DECL_OVERRIDE { wrapper_ = true; }

    void addResolvedBaseClass(Access access, ClassNode* node);
    void addDerivedClass(Access access, ClassNode* node);
    void addUnresolvedBaseClass(Access access, const QStringList& path, const QString& signature);
    void addUnresolvedUsingClause(const QString& signature);
    void fixBaseClasses();
    void fixPropertyUsingBaseClasses(PropertyNode* pn);

    QList<RelatedClass>& baseClasses() { return bases_; }
    QList<RelatedClass>& derivedClasses() { return derived_; }
    QList<RelatedClass>& ignoredBaseClasses() { return ignoredBases_; }
    QList<UsingClause>& usingClauses() { return usingClauses_; }

    const QList<RelatedClass> &baseClasses() const { return bases_; }
    const QList<RelatedClass> &derivedClasses() const { return derived_; }
    const QList<RelatedClass> &ignoredBaseClasses() const { return ignoredBases_; }
    const QList<UsingClause>& usingClauses() const { return usingClauses_; }

    QmlTypeNode* qmlElement() { return qmlelement; }
    void setQmlElement(QmlTypeNode* qcn) { qmlelement = qcn; }
    virtual bool isAbstract() const Q_DECL_OVERRIDE { return abstract_; }
    virtual void setAbstract(bool b) Q_DECL_OVERRIDE { abstract_ = b; }
    PropertyNode* findPropertyNode(const QString& name);
    QmlTypeNode* findQmlBaseNode();

private:
    QList<RelatedClass> bases_;
    QList<RelatedClass> derived_;
    QList<RelatedClass> ignoredBases_;
    QList<UsingClause> usingClauses_;
    bool abstract_;
    bool wrapper_;
    QString obsoleteLink_;
    QmlTypeNode* qmlelement;
};

class DocumentNode : public Aggregate
{
public:

    DocumentNode(Aggregate* parent,
             const QString& name,
             DocSubtype docSubtype,
             PageType ptype);
    virtual ~DocumentNode() { }

    virtual bool isDocumentNode() const Q_DECL_OVERRIDE { return true; }
    virtual void setTitle(const QString &title) Q_DECL_OVERRIDE;
    virtual void setSubTitle(const QString &subTitle) Q_DECL_OVERRIDE { subtitle_ = subTitle; }

    DocSubtype docSubtype() const Q_DECL_OVERRIDE { return nodeSubtype_; }
    virtual QString title() const Q_DECL_OVERRIDE { return title_; }
    virtual QString fullTitle() const Q_DECL_OVERRIDE;
    virtual QString subTitle() const Q_DECL_OVERRIDE;
    virtual QString imageFileName() const { return QString(); }
    virtual QString nameForLists() const Q_DECL_OVERRIDE { return title(); }
    virtual void setImageFileName(const QString& ) { }

    virtual bool isHeaderFile() const Q_DECL_OVERRIDE { return (docSubtype() == Node::HeaderFile); }
    virtual bool isExample() const Q_DECL_OVERRIDE { return (docSubtype() == Node::Example); }
    virtual bool isExampleFile() const Q_DECL_OVERRIDE { return (parent() && parent()->isExample()); }
    virtual bool isExternalPage() const Q_DECL_OVERRIDE { return nodeSubtype_ == ExternalPage; }

protected:
    DocSubtype nodeSubtype_;
    QString title_;
    QString subtitle_;
};

class ExampleNode : public DocumentNode
{
public:
    ExampleNode(Aggregate* parent, const QString& name)
        : DocumentNode(parent, name, Node::Example, Node::ExamplePage) { }
    virtual ~ExampleNode() { }
    virtual QString imageFileName() const Q_DECL_OVERRIDE { return imageFileName_; }
    virtual void setImageFileName(const QString& ifn) Q_DECL_OVERRIDE { imageFileName_ = ifn; }

private:
    QString imageFileName_;
};

struct ImportRec {
    QString name_;      // module name
    QString version_;   // <major> . <minor>
    QString importId_;  // "as" name
    QString importUri_; // subdirectory of module directory

    ImportRec(const QString& name,
              const QString& version,
              const QString& importId,
              const QString& importUri)
    : name_(name), version_(version), importId_(importId), importUri_(importUri) { }
    QString& name() { return name_; }
    QString& version() { return version_; }
    QString& importId() { return importId_; }
    QString& importUri() { return importUri_; }
    bool isEmpty() const { return name_.isEmpty(); }
};

typedef QList<ImportRec> ImportList;

class QmlTypeNode : public Aggregate
{
public:
    QmlTypeNode(Aggregate* parent, const QString& name);
    virtual ~QmlTypeNode();
    virtual bool isQmlType() const Q_DECL_OVERRIDE { return genus() == Node::QML; }
    virtual bool isJsType() const Q_DECL_OVERRIDE { return genus() == Node::JS; }
    virtual bool isQtQuickNode() const Q_DECL_OVERRIDE {
        return (logicalModuleName() == QLatin1String("QtQuick"));
    }
    virtual ClassNode* classNode() Q_DECL_OVERRIDE { return cnode_; }
    virtual void setClassNode(ClassNode* cn) Q_DECL_OVERRIDE { cnode_ = cn; }
    virtual bool isAbstract() const Q_DECL_OVERRIDE { return abstract_; }
    virtual bool isWrapper() const Q_DECL_OVERRIDE { return wrapper_; }
    virtual void setAbstract(bool b) Q_DECL_OVERRIDE { abstract_ = b; }
    virtual void setWrapper() Q_DECL_OVERRIDE { wrapper_ = true; }
    virtual bool isInternal() const Q_DECL_OVERRIDE { return (status() == Internal); }
    virtual QString qmlFullBaseName() const Q_DECL_OVERRIDE;
    virtual QString obsoleteLink() const Q_DECL_OVERRIDE { return obsoleteLink_; }
    virtual void setObsoleteLink(const QString& t) Q_DECL_OVERRIDE { obsoleteLink_ = t; };
    virtual QString logicalModuleName() const Q_DECL_OVERRIDE;
    virtual QString logicalModuleVersion() const Q_DECL_OVERRIDE;
    virtual QString logicalModuleIdentifier() const Q_DECL_OVERRIDE;
    virtual CollectionNode* logicalModule() const Q_DECL_OVERRIDE { return logicalModule_; }
    virtual void setQmlModule(CollectionNode* t) Q_DECL_OVERRIDE { logicalModule_ = t; }

    const ImportList& importList() const { return importList_; }
    void setImportList(const ImportList& il) { importList_ = il; }
    const QString& qmlBaseName() const { return qmlBaseName_; }
    void setQmlBaseName(const QString& name) { qmlBaseName_ = name; }
    bool qmlBaseNodeNotSet() const { return (qmlBaseNode_ == 0); }
    QmlTypeNode* qmlBaseNode();
    void setQmlBaseNode(QmlTypeNode* b) { qmlBaseNode_ = b; }
    void requireCppClass() { cnodeRequired_ = true; }
    bool cppClassRequired() const { return cnodeRequired_; }
    static void addInheritedBy(const QString& base, Node* sub);
    static void subclasses(const QString& base, NodeList& subs);
    static void terminate();

public:
    static bool qmlOnly;
    static QMultiMap<QString,Node*> inheritedBy;

private:
    bool abstract_;
    bool cnodeRequired_;
    bool wrapper_;
    ClassNode*    cnode_;
    QString             qmlBaseName_;
    QString             obsoleteLink_;
    CollectionNode*     logicalModule_;
    QmlTypeNode*       qmlBaseNode_;
    ImportList          importList_;
};

class QmlBasicTypeNode : public Aggregate
{
public:
    QmlBasicTypeNode(Aggregate* parent,
                     const QString& name);
    virtual ~QmlBasicTypeNode() { }
    virtual bool isQmlBasicType() const Q_DECL_OVERRIDE { return (genus() == Node::QML); }
    virtual bool isJsBasicType() const Q_DECL_OVERRIDE { return (genus() == Node::JS); }
};

class QmlPropertyGroupNode : public Aggregate
{
public:
    QmlPropertyGroupNode(QmlTypeNode* parent, const QString& name);
    virtual ~QmlPropertyGroupNode() { }
    virtual bool isQtQuickNode() const Q_DECL_OVERRIDE { return parent()->isQtQuickNode(); }
    virtual QString qmlTypeName() const Q_DECL_OVERRIDE { return parent()->qmlTypeName(); }
    virtual QString logicalModuleName() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleName();
    }
    virtual QString logicalModuleVersion() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleVersion();
    }
    virtual QString logicalModuleIdentifier() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleIdentifier();
    }
    virtual QString idNumber() Q_DECL_OVERRIDE;
    virtual bool isQmlPropertyGroup() const Q_DECL_OVERRIDE { return genus() == Node::QML; }
    virtual bool isJsPropertyGroup() const Q_DECL_OVERRIDE { return genus() == Node::JS; }
    virtual QString element() const Q_DECL_OVERRIDE { return parent()->name(); }

 private:
    int     idNumber_;
};

class QmlPropertyNode : public LeafNode
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::QmlPropertyNode)

public:
    QmlPropertyNode(Aggregate *parent,
                    const QString& name,
                    const QString& type,
                    bool attached);
    virtual ~QmlPropertyNode() { }

    virtual void setDataType(const QString& dataType) Q_DECL_OVERRIDE { type_ = dataType; }
    void setStored(bool stored) { stored_ = toFlagValue(stored); }
    void setDesignable(bool designable) { designable_ = toFlagValue(designable); }
    virtual void setReadOnly(bool ro) Q_DECL_OVERRIDE { readOnly_ = toFlagValue(ro); }
    void setDefault() { isdefault_ = true; }

    const QString &dataType() const { return type_; }
    QString qualifiedDataType() const { return type_; }
    bool isReadOnlySet() const { return (readOnly_ != FlagValueDefault); }
    bool isStored() const { return fromFlagValue(stored_,true); }
    bool isDesignable() const { return fromFlagValue(designable_,false); }
    bool isWritable();
    virtual bool isQmlProperty() const Q_DECL_OVERRIDE { return genus() == QML; }
    virtual bool isJsProperty() const Q_DECL_OVERRIDE { return genus() == JS; }
    virtual bool isDefault() const Q_DECL_OVERRIDE { return isdefault_; }
    virtual bool isReadOnly() const Q_DECL_OVERRIDE { return fromFlagValue(readOnly_,false); }
    virtual bool isAlias() const Q_DECL_OVERRIDE { return isAlias_; }
    virtual bool isAttached() const Q_DECL_OVERRIDE { return attached_; }
    virtual bool isQtQuickNode() const Q_DECL_OVERRIDE { return parent()->isQtQuickNode(); }
    virtual QString qmlTypeName() const Q_DECL_OVERRIDE { return parent()->qmlTypeName(); }
    virtual QString logicalModuleName() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleName();
    }
    virtual QString logicalModuleVersion() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleVersion();
    }
    virtual QString logicalModuleIdentifier() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleIdentifier();
    }
    virtual QString element() const Q_DECL_OVERRIDE;

 private:
    PropertyNode* findCorrespondingCppProperty();

private:
    QString type_;
    FlagValue   stored_;
    FlagValue   designable_;
    bool    isAlias_;
    bool    isdefault_;
    bool    attached_;
    FlagValue   readOnly_;
};

class EnumItem
{
public:
    EnumItem() { }
    EnumItem(const QString& name, const QString& value)
        : name_(name), value_(value) { }

    const QString& name() const { return name_; }
    const QString& value() const { return value_; }

private:
    QString name_;
    QString value_;
};

class EnumNode : public LeafNode
{
public:
    EnumNode(Aggregate* parent, const QString& name);
    virtual ~EnumNode() { }

    void addItem(const EnumItem& item);
    void setFlagsType(TypedefNode* typedeff);
    bool hasItem(const QString &name) const { return names_.contains(name); }
    virtual bool isEnumType() const Q_DECL_OVERRIDE { return true; }

    const QList<EnumItem>& items() const { return items_; }
    Access itemAccess(const QString& name) const;
    const TypedefNode* flagsType() const { return flagsType_; }
    QString itemValue(const QString &name) const;

private:
    QList<EnumItem> items_;
    QSet<QString> names_;
    const TypedefNode* flagsType_;
};

class TypedefNode : public LeafNode
{
public:
    TypedefNode(Aggregate* parent, const QString& name);
    virtual ~TypedefNode() { }

    const EnumNode* associatedEnum() const { return associatedEnum_; }

private:
    void setAssociatedEnum(const EnumNode* t);

    friend class EnumNode;

    const EnumNode* associatedEnum_;
};

inline void EnumNode::setFlagsType(TypedefNode* t)
{
    flagsType_ = t;
    t->setAssociatedEnum(this);
}


class Parameter
{
public:
    Parameter() {}
    Parameter(const QString& leftType,
              const QString& rightType = QString(),
              const QString& name = QString(),
              const QString& defaultValue = QString());
    Parameter(const Parameter& p);

    Parameter& operator=(const Parameter& p);

    void setName(const QString& name) { name_ = name; }

    bool hasType() const { return leftType_.length() + rightType_.length() > 0; }
    const QString& leftType() const { return leftType_; }
    const QString& rightType() const { return rightType_; }
    const QString& name() const { return name_; }
    const QString& defaultValue() const { return defaultValue_; }

    QString reconstruct(bool value = false) const;

private:
    QString leftType_;
    QString rightType_;
    QString name_;
    QString defaultValue_;
};

class FunctionNode : public LeafNode
{
public:
    enum Metaness {
        Plain,
        Signal,
        Slot,
        Ctor,
        Dtor,
        MacroWithParams,
        MacroWithoutParams,
        Native };
    enum Virtualness { NonVirtual, NormalVirtual, PureVirtual };

    FunctionNode(Aggregate* parent, const QString &name);
    FunctionNode(NodeType type, Aggregate* parent, const QString &name, bool attached);
    virtual ~FunctionNode() { }

    void setReturnType(const QString& t) { returnType_ = t; }
    void setParentPath(const QStringList& p) { parentPath_ = p; }
    void setMetaness(Metaness t) { metaness_ = t; }
    void setVirtualness(Virtualness v);
    void setConst(bool b) { const_ = b; }
    void setStatic(bool b) { static_ = b; }
    unsigned char overloadNumber() const { return overloadNumber_; }
    void setOverloadFlag(bool b) { overload_ = b; }
    void setOverloadNumber(unsigned char n) { overloadNumber_ = n; }
    void setReimplemented(bool b);
    void addParameter(const Parameter& parameter);
    inline void setParameters(const QList<Parameter>& parameters);
    void borrowParameterNames(const FunctionNode* source);
    void setReimplementedFrom(FunctionNode* from);

    const QString& returnType() const { return returnType_; }
    Metaness metaness() const { return metaness_; }
    bool isMacro() const {
        return metaness_ == MacroWithParams || metaness_ == MacroWithoutParams;
    }
    Virtualness virtualness() const { return virtualness_; }
    bool isConst() const { return const_; }
    bool isStatic() const { return static_; }
    bool isOverload() const { return overload_; }
    bool isReimplemented() const Q_DECL_OVERRIDE { return reimplemented_; }
    bool isFunction() const Q_DECL_OVERRIDE { return true; }
    virtual bool isQmlSignal() const Q_DECL_OVERRIDE {
        return (type() == Node::QmlSignal) && (genus() == Node::QML);
    }
    virtual bool isJsSignal() const Q_DECL_OVERRIDE {
        return (type() == Node::QmlSignal) && (genus() == Node::JS);
    }
    virtual bool isQmlSignalHandler() const Q_DECL_OVERRIDE {
        return (type() == Node::QmlSignalHandler) && (genus() == Node::QML);
    }
    virtual bool isJsSignalHandler() const Q_DECL_OVERRIDE {
        return (type() == Node::QmlSignalHandler) && (genus() == Node::JS);
    }
    virtual bool isQmlMethod() const Q_DECL_OVERRIDE {
        return (type() == Node::QmlMethod) && (genus() == Node::QML);
    }
    virtual bool isJsMethod() const Q_DECL_OVERRIDE {
        return (type() == Node::QmlMethod) && (genus() == Node::JS);
    }
    const QList<Parameter>& parameters() const { return parameters_; }
    void clearParams() { parameters_.clear(); }
    QStringList parameterNames() const;
    QString rawParameters(bool names = false, bool values = false) const;
    const FunctionNode* reimplementedFrom() const { return reimplementedFrom_; }
    const QList<FunctionNode*> &reimplementedBy() const { return reimplementedBy_; }
    const PropNodeList& associatedProperties() const { return associatedProperties_; }
    const QStringList& parentPath() const { return parentPath_; }
    bool hasAssociatedProperties() const { return !associatedProperties_.isEmpty(); }
    bool hasOneAssociatedProperty() const { return (associatedProperties_.size() == 1); }
    PropertyNode* firstAssociatedProperty() const { return associatedProperties_[0]; }
    bool hasActiveAssociatedProperty() const;

    QStringList reconstructParameters(bool values = false) const;
    QString signature(bool values = false) const;
    virtual QString element() const Q_DECL_OVERRIDE { return parent()->name(); }
    virtual bool isAttached() const Q_DECL_OVERRIDE { return attached_; }
    virtual bool isQtQuickNode() const Q_DECL_OVERRIDE { return parent()->isQtQuickNode(); }
    virtual QString qmlTypeName() const Q_DECL_OVERRIDE { return parent()->qmlTypeName(); }
    virtual QString logicalModuleName() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleName();
    }
    virtual QString logicalModuleVersion() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleVersion();
    }
    virtual QString logicalModuleIdentifier() const Q_DECL_OVERRIDE {
        return parent()->logicalModuleIdentifier();
    }
    bool isPrivateSignal() const { return privateSignal_; }
    void setPrivateSignal() { privateSignal_ = true; }

    void debug() const;

private:
    void addAssociatedProperty(PropertyNode* property);

    friend class Aggregate;
    friend class PropertyNode;

    QString     returnType_;
    QStringList parentPath_;
    Metaness    metaness_;
    Virtualness virtualness_;
    bool const_ : 1;
    bool static_ : 1;
    bool reimplemented_: 1;
    bool attached_: 1;
    bool privateSignal_: 1;
    bool overload_ : 1;
    unsigned char overloadNumber_;
    QList<Parameter> parameters_;
    const FunctionNode* reimplementedFrom_;
    PropNodeList        associatedProperties_;
    QList<FunctionNode*> reimplementedBy_;
};

class PropertyNode : public LeafNode
{
public:
    enum FunctionRole { Getter, Setter, Resetter, Notifier };
    enum { NumFunctionRoles = Notifier + 1 };

    PropertyNode(Aggregate* parent, const QString& name);
    virtual ~PropertyNode() { }

    virtual void setDataType(const QString& dataType) Q_DECL_OVERRIDE { type_ = dataType; }
    virtual bool isProperty() const Q_DECL_OVERRIDE { return true; }
    void addFunction(FunctionNode* function, FunctionRole role);
    void addSignal(FunctionNode* function, FunctionRole role);
    void setStored(bool stored) { stored_ = toFlagValue(stored); }
    void setDesignable(bool designable) { designable_ = toFlagValue(designable); }
    void setScriptable(bool scriptable) { scriptable_ = toFlagValue(scriptable); }
    void setWritable(bool writable) { writable_ = toFlagValue(writable); }
    void setUser(bool user) { user_ = toFlagValue(user); }
    void setOverriddenFrom(const PropertyNode* baseProperty);
    void setRuntimeDesFunc(const QString& rdf) { runtimeDesFunc_ = rdf; }
    void setRuntimeScrFunc(const QString& scrf) { runtimeScrFunc_ = scrf; }
    void setConstant() { const_ = true; }
    void setFinal() { final_ = true; }
    void setRevision(int revision) { revision_ = revision; }

    const QString &dataType() const { return type_; }
    QString qualifiedDataType() const;
    NodeList functions() const;
    NodeList functions(FunctionRole role) const { return functions_[(int)role]; }
    NodeList getters() const { return functions(Getter); }
    NodeList setters() const { return functions(Setter); }
    NodeList resetters() const { return functions(Resetter); }
    NodeList notifiers() const { return functions(Notifier); }
    FunctionRole role(const FunctionNode* fn) const;
    bool isStored() const { return fromFlagValue(stored_, storedDefault()); }
    bool isDesignable() const { return fromFlagValue(designable_, designableDefault()); }
    bool isScriptable() const { return fromFlagValue(scriptable_, scriptableDefault()); }
    const QString& runtimeDesignabilityFunction() const { return runtimeDesFunc_; }
    const QString& runtimeScriptabilityFunction() const { return runtimeScrFunc_; }
    bool isWritable() const { return fromFlagValue(writable_, writableDefault()); }
    bool isUser() const { return fromFlagValue(user_, userDefault()); }
    bool isConstant() const { return const_; }
    bool isFinal() const { return final_; }
    const PropertyNode* overriddenFrom() const { return overrides_; }

    bool storedDefault() const { return true; }
    bool userDefault() const { return false; }
    bool designableDefault() const { return !setters().isEmpty(); }
    bool scriptableDefault() const { return true; }
    bool writableDefault() const { return !setters().isEmpty(); }

private:
    QString type_;
    QString runtimeDesFunc_;
    QString runtimeScrFunc_;
    NodeList functions_[NumFunctionRoles];
    FlagValue stored_;
    FlagValue designable_;
    FlagValue scriptable_;
    FlagValue writable_;
    FlagValue user_;
    bool const_;
    bool final_;
    int revision_;
    const PropertyNode* overrides_;
};

inline void FunctionNode::setParameters(const QList<Parameter> &p)
{
    parameters_ = p;
}

inline void PropertyNode::addFunction(FunctionNode* function, FunctionRole role)
{
    functions_[(int)role].append(function);
    function->addAssociatedProperty(this);
}

inline void PropertyNode::addSignal(FunctionNode* function, FunctionRole role)
{
    functions_[(int)role].append(function);
    function->addAssociatedProperty(this);
}

inline NodeList PropertyNode::functions() const
{
    NodeList list;
    for (int i = 0; i < NumFunctionRoles; ++i)
        list += functions_[i];
    return list;
}

class VariableNode : public LeafNode
{
public:
    VariableNode(Aggregate* parent, const QString &name);
    virtual ~VariableNode() { }

    void setLeftType(const QString &leftType) { lrftType_ = leftType; }
    void setRightType(const QString &rightType) { rightType_ = rightType; }
    void setStatic(bool b) { static_ = b; }

    const QString &leftType() const { return lrftType_; }
    const QString &rightType() const { return rightType_; }
    QString dataType() const { return lrftType_ + rightType_; }
    bool isStatic() const { return static_; }

private:
    QString lrftType_;
    QString rightType_;
    bool static_;
};

inline VariableNode::VariableNode(Aggregate* parent, const QString &name)
    : LeafNode(Variable, parent, name), static_(false)
{
    setGenus(Node::CPP);
}

class DitaMapNode : public DocumentNode
{
public:
    DitaMapNode(Aggregate* parent, const QString& name)
        : DocumentNode(parent, name, Node::Page, Node::DitaMapPage) { }
    virtual ~DitaMapNode() { }

    const DitaRefList& map() const { return doc().ditamap(); }
};

class CollectionNode : public Aggregate
{
 public:
 CollectionNode(NodeType type,
                Aggregate* parent,
                const QString& name,
                Genus genus)
     : Aggregate(type, parent, name), seen_(false), noAutoList_(false)
    {
        setPageType(Node::OverviewPage);
        setGenus(genus);
    }
    virtual ~CollectionNode() { }

    virtual bool isCollectionNode() const Q_DECL_OVERRIDE { return true; }
    virtual bool isGroup() const Q_DECL_OVERRIDE { return genus() == Node::DOC; }
    virtual bool isModule() const Q_DECL_OVERRIDE { return genus() == Node::CPP; }
    virtual bool isQmlModule() const Q_DECL_OVERRIDE { return genus() == Node::QML; }
    virtual bool isJsModule() const Q_DECL_OVERRIDE { return genus() == Node::JS; }
    virtual QString qtVariable() const Q_DECL_OVERRIDE { return qtVariable_; }
    virtual void setQtVariable(const QString& v) Q_DECL_OVERRIDE { qtVariable_ = v; }
    virtual void addMember(Node* node) Q_DECL_OVERRIDE;
    virtual bool hasMembers() const Q_DECL_OVERRIDE;
    virtual bool hasNamespaces() const Q_DECL_OVERRIDE;
    virtual bool hasClasses() const Q_DECL_OVERRIDE;
    virtual void getMemberNamespaces(NodeMap& out) Q_DECL_OVERRIDE;
    virtual void getMemberClasses(NodeMap& out) Q_DECL_OVERRIDE;
    virtual bool wasSeen() const Q_DECL_OVERRIDE { return seen_; }
    virtual QString title() const Q_DECL_OVERRIDE { return title_; }
    virtual QString subTitle() const Q_DECL_OVERRIDE { return subtitle_; }
    virtual QString fullTitle() const Q_DECL_OVERRIDE { return title_; }
    virtual QString nameForLists() const Q_DECL_OVERRIDE { return title_; }
    virtual void setTitle(const QString &title) Q_DECL_OVERRIDE;
    virtual void setSubTitle(const QString &subTitle) Q_DECL_OVERRIDE { subtitle_ = subTitle; }

    virtual QString logicalModuleName() const Q_DECL_OVERRIDE { return logicalModuleName_; }
    virtual QString logicalModuleVersion() const Q_DECL_OVERRIDE {
        return logicalModuleVersionMajor_ + "." + logicalModuleVersionMinor_;
    }
    virtual QString logicalModuleIdentifier() const Q_DECL_OVERRIDE {
        return logicalModuleName_ + logicalModuleVersionMajor_;
    }
    virtual void setLogicalModuleInfo(const QString& arg) Q_DECL_OVERRIDE;
    virtual void setLogicalModuleInfo(const QStringList& info) Q_DECL_OVERRIDE;

    const NodeList& members() const { return members_; }
    void printMembers(const QString& title);

    void markSeen() { seen_ = true; }
    void markNotSeen() { seen_ = false; }
    bool noAutoList() const { return noAutoList_; }
    virtual void setNoAutoList(bool b)  Q_DECL_OVERRIDE { noAutoList_ = b; }

 private:
    bool        seen_;
    bool        noAutoList_;
    QString     title_;
    QString     subtitle_;
    NodeList    members_;
    QString     logicalModuleName_;
    QString     logicalModuleVersionMajor_;
    QString     logicalModuleVersionMinor_;
    QString     qtVariable_;
};

QT_END_NAMESPACE

#endif
