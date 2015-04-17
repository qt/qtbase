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

/*
  htmlgenerator.h
*/

#ifndef HTMLGENERATOR_H
#define HTMLGENERATOR_H

#include <qmap.h>
#include <qregexp.h>
#include <qxmlstream.h>
#include "codemarker.h"
#include "config.h"
#include "generator.h"

QT_BEGIN_NAMESPACE

class HelpProjectWriter;

class HtmlGenerator : public Generator
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::HtmlGenerator)

public:
    enum SinceType {
        Namespace,
        Class,
        MemberFunction,
        NamespaceFunction,
        GlobalFunction,
        Macro,
        Enum,
        Typedef,
        Property,
        Variable,
        QmlClass,
        QmlProperty,
        QmlSignal,
        QmlSignalHandler,
        QmlMethod,
        LastSinceType
    };

public:
    HtmlGenerator();
    ~HtmlGenerator();

    virtual void initializeGenerator(const Config& config) Q_DECL_OVERRIDE;
    virtual void terminateGenerator() Q_DECL_OVERRIDE;
    virtual QString format() Q_DECL_OVERRIDE;
    virtual void generateDocs() Q_DECL_OVERRIDE;
    void generateManifestFiles();

    QString protectEnc(const QString &string);
    static QString protect(const QString &string, const QString &encoding = "ISO-8859-1");
    static QString sinceTitle(int i) { return sinceTitles[i]; }

protected:
    virtual void generateQAPage() Q_DECL_OVERRIDE;
    QString generateLinksToLinksPage(const QString& module, CodeMarker* marker);
    QString generateLinksToBrokenLinksPage(CodeMarker* marker, int& count);
    virtual int generateAtom(const Atom *atom,
                             const Node *relative,
                             CodeMarker *marker) Q_DECL_OVERRIDE;
    virtual void generateClassLikeNode(InnerNode* inner, CodeMarker* marker) Q_DECL_OVERRIDE;
    virtual void generateQmlTypePage(QmlTypeNode* qcn, CodeMarker* marker) Q_DECL_OVERRIDE;
    virtual void generateQmlBasicTypePage(QmlBasicTypeNode* qbtn, CodeMarker* marker) Q_DECL_OVERRIDE;
    virtual void generateDocumentNode(DocumentNode* dn, CodeMarker* marker) Q_DECL_OVERRIDE;
    virtual void generateCollectionNode(CollectionNode* cn, CodeMarker* marker) Q_DECL_OVERRIDE;
    virtual QString fileExtension() const Q_DECL_OVERRIDE;
    virtual QString refForNode(const Node *node);
    virtual QString linkForNode(const Node *node, const Node *relative);

    void generateManifestFile(const QString &manifest, const QString &element);
    void readManifestMetaContent(const Config &config);
    void generateKeywordAnchors(const Node* node);

private:
    enum SubTitleSize { SmallSubTitle, LargeSubTitle };
    enum ExtractionMarkType {
        BriefMark,
        DetailedDescriptionMark,
        MemberMark,
        EndMark
    };

    struct ManifestMetaFilter
    {
        QSet<QString> names;
        QSet<QString> attributes;
        QSet<QString> tags;
    };

    const QPair<QString,QString> anchorForNode(const Node *node);
    void generateNavigationBar(const QString& title,
                             const Node *node,
                             CodeMarker *marker);
    void generateHeader(const QString& title,
                        const Node *node = 0,
                        CodeMarker *marker = 0);
    void generateTitle(const QString& title,
                       const Text &subTitle,
                       SubTitleSize subTitleSize,
                       const Node *relative,
                       CodeMarker *marker);
    void generateFooter(const Node *node = 0);
    void generateRequisites(InnerNode *inner,
                            CodeMarker *marker);
    void generateQmlRequisites(QmlTypeNode *qcn,
                            CodeMarker *marker);
    void generateBrief(const Node *node,
                       CodeMarker *marker,
                       const Node *relative = 0);
    void generateIncludes(const InnerNode *inner, CodeMarker *marker);
    void generateTableOfContents(const Node *node,
                                 CodeMarker *marker,
                                 QList<Section>* sections = 0);
    void generateSidebar();
    QString generateListOfAllMemberFile(const InnerNode *inner,
                                        CodeMarker *marker);
    QString generateAllQmlMembersFile(QmlTypeNode* qml_cn, CodeMarker* marker);
    QString generateLowStatusMemberFile(InnerNode *inner,
                                        CodeMarker *marker,
                                        CodeMarker::Status status);
    QString generateQmlMemberFile(QmlTypeNode* qcn,
                                  CodeMarker *marker,
                                  CodeMarker::Status status);
    void generateClassHierarchy(const Node *relative, NodeMap &classMap);
    void generateAnnotatedList(const Node* relative, CodeMarker* marker, const NodeMultiMap& nodeMap);
    void generateAnnotatedList(const Node* relative, CodeMarker* marker, const NodeList& nodes);
    void generateCompactList(ListType listType,
                             const Node *relative,
                             const NodeMultiMap &classMap,
                             bool includeAlphabet,
                             QString commonPrefix);
    void generateFunctionIndex(const Node *relative);
    void generateLegaleseList(const Node *relative, CodeMarker *marker);
    void generateList(const Node* relative, CodeMarker* marker, const QString& selector);
    void generateSectionList(const Section& section,
                             const Node *relative,
                             CodeMarker *marker,
                             CodeMarker::SynopsisStyle style);
    void generateQmlSummary(const Section& section,
                            const Node *relative,
                            CodeMarker *marker);
    void generateQmlItem(const Node *node,
                         const Node *relative,
                         CodeMarker *marker,
                         bool summary);
    void generateDetailedQmlMember(Node *node,
                                   const InnerNode *relative,
                                   CodeMarker *marker);
    void generateQmlInherits(QmlTypeNode* qcn, CodeMarker* marker) Q_DECL_OVERRIDE;
    void generateQmlInstantiates(QmlTypeNode* qcn, CodeMarker* marker);
    void generateInstantiatedBy(ClassNode* cn, CodeMarker* marker);

    void generateRequisitesTable(const QStringList& requisitesOrder, QMap<QString, Text>& requisites);
    void generateSection(const NodeList& nl,
                         const Node *relative,
                         CodeMarker *marker,
                         CodeMarker::SynopsisStyle style);
    void generateSynopsis(const Node *node,
                          const Node *relative,
                          CodeMarker *marker,
                          CodeMarker::SynopsisStyle style,
                          bool alignNames = false,
                          const QString* prefix = 0);
    void generateSectionInheritedList(const Section& section, const Node *relative);
    QString highlightedCode(const QString& markedCode,
                            const Node* relative,
                            bool alignNames = false);

    void generateFullName(const Node *apparentNode, const Node *relative, const Node *actualNode = 0);
    void generateDetailedMember(const Node *node,
                                const InnerNode *relative,
                                CodeMarker *marker);
    void generateLink(const Atom *atom, CodeMarker *marker);
    void generateStatus(const Node *node, CodeMarker *marker);

    QString getLink(const Atom *atom, const Node *relative, const Node** node);
    QString getAutoLink(const Atom *atom, const Node *relative, const Node** node);

    QString registerRef(const QString& ref);
    virtual QString fileBase(const Node *node) const Q_DECL_OVERRIDE;
    QString fileName(const Node *node);
    static int hOffset(const Node *node);
    static bool isThreeColumnEnumValueTable(const Atom *atom);
#ifdef GENERATE_MAC_REFS
    void generateMacRef(const Node *node, CodeMarker *marker);
#endif
    void beginLink(const QString &link, const Node *node, const Node *relative);
    void endLink();
    void generateExtractionMark(const Node *node, ExtractionMarkType markType);
    void reportOrphans(const InnerNode* parent);

    void beginDitamapPage(const InnerNode* node, const QString& fileName);
    void endDitamapPage();
    void writeDitaMap(const DitaMapNode* node);
    void writeDitaRefs(const DitaRefList& ditarefs);
    QXmlStreamWriter& xmlWriter();

    QMap<QString, QString> refMap;
    int codeIndent;
    HelpProjectWriter *helpProjectWriter;
    bool inObsoleteLink;
    QRegExp funcLeftParen;
    QString style;
    QString headerScripts;
    QString headerStyles;
    QString endHeader;
    QString postHeader;
    QString postPostHeader;
    QString prologue;
    QString footer;
    QString address;
    bool pleaseGenerateMacRef;
    bool noNavigationBar;
    QString project;
    QString projectDescription;
    QString projectUrl;
    QString navigationLinks;
    QString manifestDir;
    QString examplesPath;
    QStringList stylesheets;
    QStringList customHeadElements;
    bool obsoleteLinks;
    QStack<QXmlStreamWriter*> xmlWriterStack;
    static int id;
    QList<ManifestMetaFilter> manifestMetaContent;
    QString homepage;
    QString landingpage;
    QString cppclassespage;
    QString qmltypespage;
    QString buildversion;
    QString qflagsHref_;
    int tocDepth;

public:
    static bool debugging_on;
    static QString divNavTop;
};

#define HTMLGENERATOR_ADDRESS           "address"
#define HTMLGENERATOR_FOOTER            "footer"
#define HTMLGENERATOR_GENERATEMACREFS   "generatemacrefs" // ### document me
#define HTMLGENERATOR_POSTHEADER        "postheader"
#define HTMLGENERATOR_POSTPOSTHEADER    "postpostheader"
#define HTMLGENERATOR_PROLOGUE          "prologue"
#define HTMLGENERATOR_NONAVIGATIONBAR   "nonavigationbar"
#define HTMLGENERATOR_NOSUBDIRS         "nosubdirs"
#define HTMLGENERATOR_TOCDEPTH          "tocdepth"


QT_END_NAMESPACE

#endif
