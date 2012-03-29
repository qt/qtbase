/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  ditaxmlgenerator.cpp
*/

#include <QDebug>
#include <QList>
#include <qiterator.h>
#include <QTextCodec>
#include <QUuid>
#include "codemarker.h"
#include "codeparser.h"
#include "ditaxmlgenerator.h"
#include "node.h"
#include "quoter.h"
#include "separator.h"
#include "tree.h"
#include <ctype.h>

QT_BEGIN_NAMESPACE

#define COMMAND_VERSION                         Doc::alias("version")
int DitaXmlGenerator::id = 0;

/*
  The strings in this array must appear in the same order as
  the values in enum DitaXmlGenerator::DitaTag.
 */
QString DitaXmlGenerator::ditaTags[] =
{
    "",
    "alt",
    "apiDesc",
    "APIMap",
    "apiName",
    "apiRelation",
    "audience",
    "author",
    "b",
    "body",
    "bodydiv",
    "brand",
    "category",
    "codeblock",
    "comment",
    "component",
    "copyrholder",
    "copyright",
    "copyryear",
    "created",
    "critdates",
    "cxxAPIMap",
    "cxxClass",
    "cxxClassAbstract",
    "cxxClassAccessSpecifier",
    "cxxClassAPIItemLocation",
    "cxxClassBaseClass",
    "cxxClassDeclarationFile",
    "cxxClassDeclarationFileLine",
    "cxxClassDeclarationFileLineStart",
    "cxxClassDeclarationFileLineEnd",
    "cxxClassDefinition",
    "cxxClassDerivation",
    "cxxClassDerivationAccessSpecifier",
    "cxxClassDerivations",
    "cxxClassDetail",
    "cxxClassNested",
    "cxxClassNestedClass",
    "cxxClassNestedDetail",
    "cxxDefine",
    "cxxDefineAccessSpecifier",
    "cxxDefineAPIItemLocation",
    "cxxDefineDeclarationFile",
    "cxxDefineDeclarationFileLine",
    "cxxDefineDefinition",
    "cxxDefineDetail",
    "cxxDefineNameLookup",
    "cxxDefineParameter",
    "cxxDefineParameterDeclarationName",
    "cxxDefineParameters",
    "cxxDefinePrototype",
    "cxxDefineReimplemented",
    "cxxEnumeration",
    "cxxEnumerationAccessSpecifier",
    "cxxEnumerationAPIItemLocation",
    "cxxEnumerationDeclarationFile",
    "cxxEnumerationDeclarationFileLine",
    "cxxEnumerationDeclarationFileLineStart",
    "cxxEnumerationDeclarationFileLineEnd",
    "cxxEnumerationDefinition",
    "cxxEnumerationDetail",
    "cxxEnumerationNameLookup",
    "cxxEnumerationPrototype",
    "cxxEnumerationScopedName",
    "cxxEnumerator",
    "cxxEnumeratorInitialiser",
    "cxxEnumeratorNameLookup",
    "cxxEnumeratorPrototype",
    "cxxEnumerators",
    "cxxEnumeratorScopedName",
    "cxxFunction",
    "cxxFunctionAccessSpecifier",
    "cxxFunctionAPIItemLocation",
    "cxxFunctionConst",
    "cxxFunctionConstructor",
    "cxxFunctionDeclarationFile",
    "cxxFunctionDeclarationFileLine",
    "cxxFunctionDeclaredType",
    "cxxFunctionDefinition",
    "cxxFunctionDestructor",
    "cxxFunctionDetail",
    "cxxFunctionNameLookup",
    "cxxFunctionParameter",
    "cxxFunctionParameterDeclarationName",
    "cxxFunctionParameterDeclaredType",
    "cxxFunctionParameterDefaultValue",
    "cxxFunctionParameters",
    "cxxFunctionPrototype",
    "cxxFunctionPureVirtual",
    "cxxFunctionReimplemented",
    "cxxFunctionScopedName",
    "cxxFunctionStorageClassSpecifierStatic",
    "cxxFunctionVirtual",
    "cxxTypedef",
    "cxxTypedefAccessSpecifier",
    "cxxTypedefAPIItemLocation",
    "cxxTypedefDeclarationFile",
    "cxxTypedefDeclarationFileLine",
    "cxxTypedefDefinition",
    "cxxTypedefDetail",
    "cxxTypedefNameLookup",
    "cxxTypedefScopedName",
    "cxxVariable",
    "cxxVariableAccessSpecifier",
    "cxxVariableAPIItemLocation",
    "cxxVariableDeclarationFile",
    "cxxVariableDeclarationFileLine",
    "cxxVariableDeclaredType",
    "cxxVariableDefinition",
    "cxxVariableDetail",
    "cxxVariableNameLookup",
    "cxxVariablePrototype",
    "cxxVariableReimplemented",
    "cxxVariableScopedName",
    "cxxVariableStorageClassSpecifierStatic",
    "data",
    "data-about",
    "dd",
    "dl",
    "dlentry",
    "dt",
    "entry",
    "fig",
    "i",
    "image",
    "keyword",
    "keywords",
    "li",
    "link",
    "linktext",
    "lq",
    "map",
    "mapref",
    "metadata",
    "note",
    "ol",
    "othermeta",
    "p",
    "parameter",
    "permissions",
    "ph",
    "platform",
    "pre",
    "prodinfo",
    "prodname",
    "prolog",
    "publisher",
    "related-links",
    "resourceid",
    "revised",
    "row",
    "section",
    "sectiondiv",
    "shortdesc",
    "simpletable",
    "source",
    "stentry",
    "sthead",
    "strow",
    "sub",
    "sup",
    "table",
    "tbody",
    "tgroup",
    "thead",
    "title",
    "tm",
    "topic",
    "topicmeta",
    "topicref",
    "tt",
    "u",
    "uicontrol",
    "ul",
    "unknown",
    "vrm",
    "vrmlist",
    "xref",
    ""
};

static bool showBrokenLinks = false;

/*!
  Quick, dirty, and very ugly. Unescape \a text
  so QXmlStreamWriter::writeCharacters() can put
  the escapes back in again!
 */
void DitaXmlGenerator::writeCharacters(const QString& text)
{
    QString t = text;
    t = t.replace("&lt;","<");
    t = t.replace("&gt;",">");
    t = t.replace("&amp;","&");
    t = t.replace("&quot;","\"");
    xmlWriter().writeCharacters(t);
}

/*!
  Appends an <xref> element to the current XML stream
  with the \a href attribute and the \a text.
 */
void DitaXmlGenerator::addLink(const QString& href,
                               const QStringRef& text,
                               DitaTag t)
{
    if (!href.isEmpty()) {
        writeStartTag(t);
        // formathtml
        writeHrefAttribute(href);
        writeCharacters(text.toString());
        writeEndTag(); // </t>
    }
    else {
        writeCharacters(text.toString());
    }
}

/*!
  Push \a t onto the dita tag stack and write the appropriate
  start tag to the DITA XML file.
 */
void DitaXmlGenerator::writeStartTag(DitaTag t)
{
    xmlWriter().writeStartElement(ditaTags[t]);
    tagStack.push(t);
}

/*!
  Pop the current DITA tag off the stack, and write the
  appropriate end tag to the DITA XML file. If \a t is
  not \e DT_NONE (default), then \a t contains the enum
  value of the tag that should be on top of the stack.

  If the stack is empty, no end tag is written and false
  is returned. Otherwise, an end tag is written and true
  is returned.
 */
bool DitaXmlGenerator::writeEndTag(DitaTag t)
{
    if (tagStack.isEmpty())
        return false;
    DitaTag top = tagStack.pop();
    if (t > DT_NONE && top != t)
        qDebug() << "Expected:" << t << "ACTUAL:" << top;
    xmlWriter().writeEndElement();
    return true;
}

/*!
  Return the current DITA element tag, the one
  on top of the stack.
 */
DitaXmlGenerator::DitaTag DitaXmlGenerator::currentTag()
{
    return tagStack.top();
}

/*!
  Write the start tag \c{<apiDesc>}. if \a title is not
  empty, generate a GUID from it and write the GUID as the
  value of the \e{id} attribute.

  Then if \a outputclass is not empty, write it as the value
  of the \a outputclass attribute.

  Fiunally, set the section nesting level to 1 and return 1.
 */
int DitaXmlGenerator::enterApiDesc(const QString& outputclass, const QString& title)
{
    writeStartTag(DT_apiDesc);
    if (!title.isEmpty()) {
        writeGuidAttribute(title);
        //Are there cases where the spectitle is required?
        //xmlWriter().writeAttribute("spectitle",title);
    }
    if (!outputclass.isEmpty())
        xmlWriter().writeAttribute("outputclass",outputclass);
    sectionNestingLevel = 1;
    return sectionNestingLevel;
}

/*!
  If the section nesting level is 0, output a \c{<section>}
  element with an \e id attribute generated from \a title and
  an \e outputclass attribute set to \a outputclass.
  If \a title is null, no \e id attribute is output.
  If \a outputclass is empty, no \e outputclass attribute
  is output.

  Finally, increment the section nesting level and return
  the new value.
 */
int DitaXmlGenerator::enterSection(const QString& outputclass, const QString& title)
{
    if (sectionNestingLevel == 0) {
        writeStartTag(DT_section);
        if (!title.isEmpty())
            writeGuidAttribute(title);
        if (!outputclass.isEmpty())
            xmlWriter().writeAttribute("outputclass",outputclass);
    }
    else if (!title.isEmpty()) {
        writeStartTag(DT_p);
        writeGuidAttribute(title);
        if (!outputclass.isEmpty())
            xmlWriter().writeAttribute("outputclass",outputclass);
        writeCharacters(title);
        writeEndTag(); // </p>
    }
    return ++sectionNestingLevel;
}

/*!
  If the section nesting level is greater than 0, decrement
  it. If it becomes 0, output a \c {</section>}. Return the
  decremented section nesting level.
 */
int DitaXmlGenerator::leaveSection()
{
    if (sectionNestingLevel > 0) {
        --sectionNestingLevel;
        if (sectionNestingLevel == 0)
            writeEndTag(); // </section> or </apiDesc>
    }
    return sectionNestingLevel;
}

/*!
  The default constructor.
 */
DitaXmlGenerator::DitaXmlGenerator()
    : inContents(false),
      inDetailedDescription(false),
      inLegaleseText(false),
      inLink(false),
      inObsoleteLink(false),
      inSectionHeading(false),
      inTableHeader(false),
      inTableBody(false),
      noLinks(false),
      obsoleteLinks(false),
      offlineDocs(true),
      threeColumnEnumValueTable(true),
      codeIndent(0),
      numTableRows(0),
      divNestingLevel(0),
      sectionNestingLevel(0),
      tableColumnCount(0),
      funcLeftParen("\\S(\\()"),
      tree_(0),
      nodeTypeMaps(Node::LastType,0),
      nodeSubtypeMaps(Node::LastSubtype,0),
      pageTypeMaps(Node::OnBeyondZebra,0)
{
    // nothing yet.
}

/*!
  The destructor has nothing to do.
 */
DitaXmlGenerator::~DitaXmlGenerator()
{
    GuidMaps::iterator i = guidMaps.begin();
    while (i != guidMaps.end()) {
        delete i.value();
        ++i;
    }
}

/*!
  A lot of internal structures are initialized.
 */
void DitaXmlGenerator::initializeGenerator(const Config &config)
{
    Generator::initializeGenerator(config);
    obsoleteLinks = config.getBool(QLatin1String(CONFIG_OBSOLETELINKS));
    setImageFileExtensions(QStringList() << "png" << "jpg" << "jpeg" << "gif");

    style = config.getString(DitaXmlGenerator::format() +
                             Config::dot +
                             DITAXMLGENERATOR_STYLE);
    postHeader = config.getString(DitaXmlGenerator::format() +
                                  Config::dot +
                                  DITAXMLGENERATOR_POSTHEADER);
    postPostHeader = config.getString(DitaXmlGenerator::format() +
                                      Config::dot +
                                      DITAXMLGENERATOR_POSTPOSTHEADER);
    footer = config.getString(DitaXmlGenerator::format() +
                              Config::dot +
                              DITAXMLGENERATOR_FOOTER);
    address = config.getString(DitaXmlGenerator::format() +
                               Config::dot +
                               DITAXMLGENERATOR_ADDRESS);
    pleaseGenerateMacRef = config.getBool(DitaXmlGenerator::format() +
                                          Config::dot +
                                          DITAXMLGENERATOR_GENERATEMACREFS);

    project = config.getString(CONFIG_PROJECT);
    projectDescription = config.getString(CONFIG_DESCRIPTION);
    if (projectDescription.isEmpty() && !project.isEmpty())
        projectDescription = project + " Reference Documentation";

    projectUrl = config.getString(CONFIG_URL);

    outputEncoding = config.getString(CONFIG_OUTPUTENCODING);
    if (outputEncoding.isEmpty())
        outputEncoding = QLatin1String("ISO-8859-1");
    outputCodec = QTextCodec::codecForName(outputEncoding.toLocal8Bit());

    naturalLanguage = config.getString(CONFIG_NATURALLANGUAGE);
    if (naturalLanguage.isEmpty())
        naturalLanguage = QLatin1String("en");

    config.subVarsAndValues("dita.metadata.default",metadataDefaults);
    QSet<QString> editionNames = config.subVars(CONFIG_EDITION);
    QSet<QString>::ConstIterator edition = editionNames.begin();
    while (edition != editionNames.end()) {
        QString editionName = *edition;
        QStringList editionModules = config.getStringList(CONFIG_EDITION +
                                                          Config::dot +
                                                          editionName +
                                                          Config::dot +
                                                          "modules");
        QStringList editionGroups = config.getStringList(CONFIG_EDITION +
                                                         Config::dot +
                                                         editionName +
                                                         Config::dot +
                                                         "groups");

        if (!editionModules.isEmpty())
            editionModuleMap[editionName] = editionModules;
        if (!editionGroups.isEmpty())
            editionGroupMap[editionName] = editionGroups;

        ++edition;
    }

    stylesheets = config.getStringList(DitaXmlGenerator::format() +
                                       Config::dot +
                                       DITAXMLGENERATOR_STYLESHEETS);
    customHeadElements = config.getStringList(DitaXmlGenerator::format() +
                                              Config::dot +
                                              DITAXMLGENERATOR_CUSTOMHEADELEMENTS);
    codeIndent = config.getInt(CONFIG_CODEINDENT);
    version = config.getString(CONFIG_VERSION);
    vrm = version.split(".");
}

/*!
  All this does is call the same function in the base class.
 */
void DitaXmlGenerator::terminateGenerator()
{
    Generator::terminateGenerator();
}

/*!
  Returns "DITAXML".
 */
QString DitaXmlGenerator::format()
{
    return "DITAXML";
}

/*!
  Calls lookupGuid() to get a GUID for \a text, then writes
  it to the XML stream as an "id" attribute, and returns it.
 */
QString DitaXmlGenerator::writeGuidAttribute(QString text)
{
    QString guid = lookupGuid(outFileName(),text);
    xmlWriter().writeAttribute("id",guid);
    return guid;
}


/*!
  Write's the GUID for the \a node to the current XML stream
  as an "id" attribute. If the \a node doesn't yet have a GUID,
  one is generated.
 */
void DitaXmlGenerator::writeGuidAttribute(Node* node)
{
    xmlWriter().writeAttribute("id",node->guid());
}

/*!
  Looks up \a text in the GUID map. If it finds \a text,
  it returns the associated GUID. Otherwise it inserts
  \a text into the map with a new GUID, and it returns
  the new GUID.
 */
QString DitaXmlGenerator::lookupGuid(QString text)
{
    QMap<QString, QString>::const_iterator i = name2guidMap.find(text);
    if (i != name2guidMap.end())
        return i.value();
#if 0
    QString t = QUuid::createUuid().toString();
    QString guid = "id-" + t.mid(1,t.length()-2);
#endif
    QString guid = Node::cleanId(text);
    name2guidMap.insert(text,guid);
    return guid;
}

/*!
  First, look up the GUID map for \a fileName. If there isn't
  a GUID map for \a fileName, create one and insert it into
  the map of GUID maps. Then look up \a text in that GUID map.
  If \a text is found, return the associated GUID. Otherwise,
  insert \a text into the GUID map with a new GUID, and return
  the new GUID.
 */
QString DitaXmlGenerator::lookupGuid(const QString& fileName, const QString& text)
{
    GuidMap* gm = lookupGuidMap(fileName);
    GuidMap::const_iterator i = gm->find(text);
    if (i != gm->end())
        return i.value();
#if 0
    QString t = QUuid::createUuid().toString();
    QString guid = "id-" + t.mid(1,t.length()-2);
#endif
    QString guid = Node::cleanId(text);
    gm->insert(text,guid);
    return guid;
}

/*!
  Looks up \a fileName in the map of GUID maps. If it finds
  \a fileName, it returns a pointer to the associated GUID
  map. Otherwise it creates a new GUID map and inserts it
  into the map of GUID maps with \a fileName as its key.
 */
GuidMap* DitaXmlGenerator::lookupGuidMap(const QString& fileName)
{
    GuidMaps::const_iterator i = guidMaps.find(fileName);
    if (i != guidMaps.end())
        return i.value();
    GuidMap* gm = new GuidMap;
    guidMaps.insert(fileName,gm);
    return gm;
}

/*!
  This is where the DITA XML files are written.
  \note The file is created in PageGenerator::generateTree().
 */
void DitaXmlGenerator::generateTree(const Tree *tree)
{
    tree_ = tree;
    nonCompatClasses.clear();
    mainClasses.clear();
    compatClasses.clear();
    obsoleteClasses.clear();
    moduleClassMap.clear();
    moduleNamespaceMap.clear();
    funcIndex.clear();
    legaleseTexts.clear();
    serviceClasses.clear();
    qmlClasses.clear();
    findAllClasses(tree->root());
    findAllFunctions(tree->root());
    findAllLegaleseTexts(tree->root());
    findAllNamespaces(tree->root());
    findAllSince(tree->root());

    Generator::generateTree(tree);
    writeDitaMap(tree);
}

void DitaXmlGenerator::startText(const Node* /* relative */,
                                 CodeMarker* /* marker */)
{
    inLink = false;
    inContents = false;
    inSectionHeading = false;
    inTableHeader = false;
    numTableRows = 0;
    threeColumnEnumValueTable = true;
    link.clear();
    sectionNumber.clear();
}

static int countTableColumns(const Atom* t)
{
    int result = 0;
    if (t->type() == Atom::TableHeaderLeft) {
        while (t->type() == Atom::TableHeaderLeft) {
            int count = 0;
            t = t->next();
            while (t->type() != Atom::TableHeaderRight) {
                if (t->type() == Atom::TableItemLeft)
                    ++count;
                t = t->next();
            }
            if (count > result)
                result = count;
            t = t->next();
        }
    }
    else if (t->type() == Atom::TableRowLeft) {
        while (t->type() != Atom::TableRowRight) {
            if (t->type() == Atom::TableItemLeft)
                ++result;
            t = t->next();
        }
    }
    return result;
}

/*!
  Generate html from an instance of Atom.
 */
int DitaXmlGenerator::generateAtom(const Atom *atom,
                                   const Node *relative,
                                   CodeMarker *marker)
{
    int skipAhead = 0;
    QString hx, str;
    static bool in_para = false;
    QString guid, hc, attr;

    switch (atom->type()) {
    case Atom::AbstractLeft:
        break;
    case Atom::AbstractRight:
        break;
    case Atom::AutoLink:
        if (!noLinks && !inLink && !inContents && !inSectionHeading) {
            const Node* node = 0;
            QString link = getLink(atom, relative, marker, &node);
            if (!link.isEmpty()) {
                beginLink(link);
                generateLink(atom, relative, marker);
                endLink();
            }
            else {
                writeCharacters(protectEnc(atom->string()));
            }
        }
        else {
            writeCharacters(protectEnc(atom->string()));
        }
        break;
    case Atom::BaseName:
        break;
    case Atom::BriefLeft:
        //if (relative->type() == Node::Fake) {
        //skipAhead = skipAtoms(atom, Atom::BriefRight);
        //break;
        //}
        if (inSection()) {
            writeStartTag(DT_p);
            xmlWriter().writeAttribute("outputclass","brief");
        }
        else {
            noLinks = true;
            writeStartTag(DT_shortdesc);
        }
        if (relative->type() == Node::Property ||
                relative->type() == Node::Variable) {
            xmlWriter().writeCharacters("This ");
            if (relative->type() == Node::Property)
                xmlWriter().writeCharacters("property");
            else if (relative->type() == Node::Variable)
                xmlWriter().writeCharacters("variable");
            xmlWriter().writeCharacters(" holds ");
        }
        if (noLinks) {
            atom = atom->next();
            while (atom != 0 && atom->type() != Atom::BriefRight) {
                if (atom->type() == Atom::String ||
                        atom->type() == Atom::AutoLink)
                    str += atom->string();
                skipAhead++;
                atom = atom->next();
            }
            str[0] = str[0].toLower();
            if (str.endsWith(QLatin1Char('.')))
                str.truncate(str.length() - 1);
            writeCharacters(str + QLatin1Char('.'));
        }
        break;
    case Atom::BriefRight:
        //        if (relative->type() != Node::Fake)
        writeEndTag(); // </shortdesc> or </p>
        noLinks = false;
        break;
    case Atom::C:
        writeStartTag(DT_tt);
        if (inLink) {
            writeCharacters(protectEnc(plainCode(atom->string())));
        }
        else {
            writeText(atom->string(), marker, relative);
        }
        writeEndTag(); // see writeStartElement() above
        break;
    case Atom::Code:
    {
        writeStartTag(DT_codeblock);
        xmlWriter().writeAttribute("outputclass","cpp");
        QString chars = trimmedTrailing(atom->string());
        writeText(chars, marker, relative);
        writeEndTag(); // </codeblock>
    }
        break;
    case Atom::Qml:
        writeStartTag(DT_codeblock);
        xmlWriter().writeAttribute("outputclass","qml");
        writeText(trimmedTrailing(atom->string()), marker, relative);
        writeEndTag(); // </codeblock>
        break;
    case Atom::CodeNew:
        writeStartTag(DT_p);
        xmlWriter().writeCharacters("you can rewrite it as");
        writeEndTag(); // </p>
        writeStartTag(DT_codeblock);
        writeText(trimmedTrailing(atom->string()), marker, relative);
        writeEndTag(); // </codeblock>
        break;
    case Atom::CodeOld:
        writeStartTag(DT_p);
        xmlWriter().writeCharacters("For example, if you have code like");
        writeEndTag(); // </p>
        // fallthrough
    case Atom::CodeBad:
        writeStartTag(DT_codeblock);
        writeCharacters(trimmedTrailing(plainCode(atom->string())));
        writeEndTag(); // </codeblock>
        break;
    case Atom::DivLeft:
    {
        bool inStartElement = false;
        attr = atom->string();
        DitaTag t = currentTag();
        if ((t == DT_section) || (t == DT_sectiondiv)) {
            writeStartTag(DT_sectiondiv);
            divNestingLevel++;
            inStartElement = true;
        }
        else if ((t == DT_body) || (t == DT_bodydiv)) {
            writeStartTag(DT_bodydiv);
            divNestingLevel++;
            inStartElement = true;
        }
        if (!attr.isEmpty()) {
            if (attr.contains('=')) {
                int index = 0;
                int from = 0;
                QString values;
                while (index >= 0) {
                    index = attr.indexOf('"',from);
                    if (index >= 0) {
                        ++index;
                        from = index;
                        index = attr.indexOf('"',from);
                        if (index > from) {
                            if (!values.isEmpty())
                                values.append(' ');
                            values += attr.mid(from,index-from);
                            from = index+1;
                        }
                    }
                }
                attr = values;
            }
        }
        if (inStartElement)
            xmlWriter().writeAttribute("outputclass", attr);
    }
        break;
    case Atom::DivRight:
        if ((currentTag() == DT_sectiondiv) || (currentTag() == DT_bodydiv)) {
            writeEndTag(); // </sectiondiv>, </bodydiv>, or </p>
            if (divNestingLevel > 0)
                --divNestingLevel;
        }
        break;
    case Atom::FootnoteLeft:
        // ### For now
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        xmlWriter().writeCharacters("<!-- ");
        break;
    case Atom::FootnoteRight:
        // ### For now
        xmlWriter().writeCharacters("-->");
        break;
    case Atom::FormatElse:
    case Atom::FormatEndif:
    case Atom::FormatIf:
        break;
    case Atom::FormattingLeft:
    {
        DitaTag t = DT_LAST;
        if (atom->string() == ATOM_FORMATTING_BOLD)
            t = DT_b;
        else if (atom->string() == ATOM_FORMATTING_PARAMETER)
            t = DT_i;
        else if (atom->string() == ATOM_FORMATTING_ITALIC)
            t = DT_i;
        else if (atom->string() == ATOM_FORMATTING_TELETYPE)
            t = DT_tt;
        else if (atom->string().startsWith("span ")) {
            t = DT_keyword;
        }
        else if (atom->string() == ATOM_FORMATTING_UICONTROL)
            t = DT_uicontrol;
        else if (atom->string() == ATOM_FORMATTING_UNDERLINE)
            t = DT_u;
        else if (atom->string() == ATOM_FORMATTING_INDEX)
            t = DT_comment;
        else if (atom->string() == ATOM_FORMATTING_SUBSCRIPT)
            t = DT_sub;
        else if (atom->string() == ATOM_FORMATTING_SUPERSCRIPT)
            t = DT_sup;
        else
            qDebug() << "DT_LAST";
        writeStartTag(t);
        if (atom->string() == ATOM_FORMATTING_PARAMETER) {
            if (atom->next() != 0 && atom->next()->type() == Atom::String) {
                QRegExp subscriptRegExp("([a-z]+)_([0-9n])");
                if (subscriptRegExp.exactMatch(atom->next()->string())) {
                    xmlWriter().writeCharacters(subscriptRegExp.cap(1));
                    writeStartTag(DT_sub);
                    xmlWriter().writeCharacters(subscriptRegExp.cap(2));
                    writeEndTag(); // </sub>
                    skipAhead = 1;
                }
            }
        }
        else if (t == DT_keyword) {
            QString attr = atom->string().mid(5);
            if (!attr.isEmpty()) {
                if (attr.contains('=')) {
                    int index = 0;
                    int from = 0;
                    QString values;
                    while (index >= 0) {
                        index = attr.indexOf('"',from);
                        if (index >= 0) {
                            ++index;
                            from = index;
                            index = attr.indexOf('"',from);
                            if (index > from) {
                                if (!values.isEmpty())
                                    values.append(' ');
                                values += attr.mid(from,index-from);
                                from = index+1;
                            }
                        }
                    }
                    attr = values;
                }
            }
            xmlWriter().writeAttribute("outputclass", attr);
        }
    }
        break;
    case Atom::FormattingRight:
        if (atom->string() == ATOM_FORMATTING_LINK) {
            endLink();
        }
        else {
            writeEndTag(); // ?
        }
        break;
    case Atom::AnnotatedList:
    {
        QList<Node*> values = tree_->groups().values(atom->string());
        NodeMap nodeMap;
        for (int i = 0; i < values.size(); ++i) {
            const Node* n = values.at(i);
            if ((n->status() != Node::Internal) && (n->access() != Node::Private)) {
                nodeMap.insert(n->nameForLists(),n);
            }
        }
        generateAnnotatedList(relative, marker, nodeMap);
    }
        break;
    case Atom::GeneratedList:
        if (atom->string() == "annotatedclasses") {
            generateAnnotatedList(relative, marker, nonCompatClasses);
        }
        else if (atom->string() == "classes") {
            generateCompactList(relative, marker, nonCompatClasses, true);
        }
        else if (atom->string() == "qmlclasses") {
            generateCompactList(relative, marker, qmlClasses, true);
        }
        else if (atom->string().contains("classesbymodule")) {
            QString arg = atom->string().trimmed();
            QString moduleName = atom->string().mid(atom->string().indexOf(
                                                        "classesbymodule") + 15).trimmed();
            if (moduleClassMap.contains(moduleName))
                generateAnnotatedList(relative, marker, moduleClassMap[moduleName]);
        }
        else if (atom->string().contains("classesbyedition")) {

            QString arg = atom->string().trimmed();
            QString editionName = atom->string().mid(atom->string().indexOf(
                                                         "classesbyedition") + 16).trimmed();

            if (editionModuleMap.contains(editionName)) {

                // Add all classes in the modules listed for that edition.
                NodeMap editionClasses;
                foreach (const QString &moduleName, editionModuleMap[editionName]) {
                    if (moduleClassMap.contains(moduleName))
                        editionClasses.unite(moduleClassMap[moduleName]);
                }

                // Add additional groups and remove groups of classes that
                // should be excluded from the edition.

                QMultiMap <QString, Node *> groups = tree_->groups();
                foreach (const QString &groupName, editionGroupMap[editionName]) {
                    QList<Node *> groupClasses;
                    if (groupName.startsWith(QLatin1Char('-'))) {
                        groupClasses = groups.values(groupName.mid(1));
                        foreach (const Node *node, groupClasses)
                            editionClasses.remove(node->name());
                    }
                    else {
                        groupClasses = groups.values(groupName);
                        foreach (const Node *node, groupClasses)
                            editionClasses.insert(node->name(), node);
                    }
                }
                generateAnnotatedList(relative, marker, editionClasses);
            }
        }
        else if (atom->string() == "classhierarchy") {
            generateClassHierarchy(relative, marker, nonCompatClasses);
        }
        else if (atom->string() == "compatclasses") {
            generateCompactList(relative, marker, compatClasses, false);
        }
        else if (atom->string() == "obsoleteclasses") {
            generateCompactList(relative, marker, obsoleteClasses, false);
        }
        else if (atom->string() == "functionindex") {
            generateFunctionIndex(relative, marker);
        }
        else if (atom->string() == "legalese") {
            generateLegaleseList(relative, marker);
        }
        else if (atom->string() == "mainclasses") {
            generateCompactList(relative, marker, mainClasses, true);
        }
        else if (atom->string() == "services") {
            generateCompactList(relative, marker, serviceClasses, false);
        }
        else if (atom->string() == "overviews") {
            generateOverviewList(relative, marker);
        }
        else if (atom->string() == "namespaces") {
            generateAnnotatedList(relative, marker, namespaceIndex);
        }
        else if (atom->string() == "related") {
            const FakeNode *fake = static_cast<const FakeNode *>(relative);
            if (fake && !fake->groupMembers().isEmpty()) {
                NodeMap groupMembersMap;
                foreach (const Node *node, fake->groupMembers()) {
                    if (node->type() == Node::Fake)
                        groupMembersMap[fullName(node, relative, marker)] = node;
                }
                generateAnnotatedList(fake, marker, groupMembersMap);
            }
        }
        break;
    case Atom::SinceList:
    {
        NewSinceMaps::const_iterator nsmap;
        nsmap = newSinceMaps.find(atom->string());
        NewClassMaps::const_iterator ncmap;
        ncmap = newClassMaps.find(atom->string());
        NewClassMaps::const_iterator nqcmap;
        nqcmap = newQmlClassMaps.find(atom->string());
        if ((nsmap != newSinceMaps.constEnd()) && !nsmap.value().isEmpty()) {
            QList<Section> sections;
            QList<Section>::ConstIterator s;
            for (int i=0; i<LastSinceType; ++i)
                sections.append(Section(sinceTitle(i),QString(),QString(),QString()));

            NodeMultiMap::const_iterator n = nsmap.value().constBegin();
            while (n != nsmap.value().constEnd()) {
                const Node* node = n.value();
                switch (node->type()) {
                case Node::Fake:
                    if (node->subType() == Node::QmlClass) {
                        sections[QmlClass].appendMember((Node*)node);
                    }
                    break;
                case Node::Namespace:
                    sections[Namespace].appendMember((Node*)node);
                    break;
                case Node::Class:
                    sections[Class].appendMember((Node*)node);
                    break;
                case Node::Enum:
                    sections[Enum].appendMember((Node*)node);
                    break;
                case Node::Typedef:
                    sections[Typedef].appendMember((Node*)node);
                    break;
                case Node::Function: {
                    const FunctionNode* fn = static_cast<const FunctionNode*>(node);
                    if (fn->isMacro())
                        sections[Macro].appendMember((Node*)node);
                    else {
                        Node* p = fn->parent();
                        if (p) {
                            if (p->type() == Node::Class)
                                sections[MemberFunction].appendMember((Node*)node);
                            else if (p->type() == Node::Namespace) {
                                if (p->name().isEmpty())
                                    sections[GlobalFunction].appendMember((Node*)node);
                                else
                                    sections[NamespaceFunction].appendMember((Node*)node);
                            }
                            else
                                sections[GlobalFunction].appendMember((Node*)node);
                        }
                        else
                            sections[GlobalFunction].appendMember((Node*)node);
                    }
                    break;
                }
                case Node::Property:
                    sections[Property].appendMember((Node*)node);
                    break;
                case Node::Variable:
                    sections[Variable].appendMember((Node*)node);
                    break;
                case Node::QmlProperty:
                    sections[QmlProperty].appendMember((Node*)node);
                    break;
                case Node::QmlSignal:
                    sections[QmlSignal].appendMember((Node*)node);
                    break;
                case Node::QmlSignalHandler:
                    sections[QmlSignalHandler].appendMember((Node*)node);
                    break;
                case Node::QmlMethod:
                    sections[QmlMethod].appendMember((Node*)node);
                    break;
                default:
                    break;
                }
                ++n;
            }

            /*
                  First generate the table of contents.
                 */
            writeStartTag(DT_ul);
            s = sections.constBegin();
            while (s != sections.constEnd()) {
                if (!(*s).members.isEmpty()) {
                    QString li = outFileName() + QLatin1Char('#') + Doc::canonicalTitle((*s).name);
                    writeXrefListItem(li, (*s).name);
                }
                ++s;
            }
            writeEndTag(); // </ul>

            int idx = 0;
            s = sections.constBegin();
            while (s != sections.constEnd()) {
                if (!(*s).members.isEmpty()) {
                    writeStartTag(DT_p);
                    writeGuidAttribute(Doc::canonicalTitle((*s).name));
                    xmlWriter().writeAttribute("outputclass","h3");
                    writeCharacters(protectEnc((*s).name));
                    writeEndTag(); // </p>
                    if (idx == Class)
                        generateCompactList(0, marker, ncmap.value(), false, QString("Q"));
                    else if (idx == QmlClass)
                        generateCompactList(0, marker, nqcmap.value(), false, QString("Q"));
                    else if (idx == MemberFunction) {
                        ParentMaps parentmaps;
                        ParentMaps::iterator pmap;
                        NodeList::const_iterator i = s->members.constBegin();
                        while (i != s->members.constEnd()) {
                            Node* p = (*i)->parent();
                            pmap = parentmaps.find(p);
                            if (pmap == parentmaps.end())
                                pmap = parentmaps.insert(p,NodeMultiMap());
                            pmap->insert((*i)->name(),(*i));
                            ++i;
                        }
                        pmap = parentmaps.begin();
                        while (pmap != parentmaps.end()) {
                            NodeList nlist = pmap->values();
                            writeStartTag(DT_p);
                            xmlWriter().writeCharacters("Class ");
                            writeStartTag(DT_xref);
                            // formathtml
                            xmlWriter().writeAttribute("href",linkForNode(pmap.key(), 0));
                            QStringList pieces = fullName(pmap.key(), 0, marker).split("::");
                            writeCharacters(protectEnc(pieces.last()));
                            writeEndTag(); // </xref>
                            xmlWriter().writeCharacters(":");
                            writeEndTag(); // </p>

                            generateSection(nlist, 0, marker, CodeMarker::Summary);
                            ++pmap;
                        }
                    }
                    else {
                        generateSection(s->members, 0, marker, CodeMarker::Summary);
                    }
                }
                ++idx;
                ++s;
            }
        }
    }
        break;
    case Atom::Image:
    case Atom::InlineImage:
    {
        QString fileName = imageFileName(relative, atom->string());
        QString text;
        if (atom->next() != 0)
            text = atom->next()->string();
        if (fileName.isEmpty()) {
            QString images = "images";
            if (!baseDir().isEmpty())
                images.prepend("../");
            if (atom->string()[0] != '/')
                images.append(QLatin1Char('/'));
            fileName = images + atom->string();
        }
        if (relative && (relative->type() == Node::Fake) &&
                (relative->subType() == Node::Example)) {
            const ExampleNode* cen = static_cast<const ExampleNode*>(relative);
            if (cen->imageFileName().isEmpty()) {
                ExampleNode* en = const_cast<ExampleNode*>(cen);
                en->setImageFileName(fileName);
            }
        }

        if (currentTag() != DT_xref)
            writeStartTag(DT_fig);
        writeStartTag(DT_image);
        writeHrefAttribute(protectEnc(fileName));
        if (atom->type() == Atom::InlineImage)
            xmlWriter().writeAttribute("placement","inline");
        else {
            xmlWriter().writeAttribute("placement","break");
            xmlWriter().writeAttribute("align","center");
        }
        if (!text.isEmpty()) {
            writeStartTag(DT_alt);
            writeCharacters(protectEnc(text));
            writeEndTag(); // </alt>
        }
        writeEndTag(); // </image>
        if (currentTag() != DT_xref)
            writeEndTag(); // </fig>
    }
        break;
    case Atom::ImageText:
        // nothing
        break;
    case Atom::ImportantLeft:
        writeStartTag(DT_note);
        xmlWriter().writeAttribute("type","important");
        break;
    case Atom::ImportantRight:
        writeEndTag(); // </note>
        break;
    case Atom::NoteLeft:
        writeStartTag(DT_note);
        xmlWriter().writeAttribute("type","note");
        break;
    case Atom::NoteRight:
        writeEndTag(); // </note>
        break;
    case Atom::LegaleseLeft:
        inLegaleseText = true;
        break;
    case Atom::LegaleseRight:
        inLegaleseText = false;
        break;
    case Atom::LineBreak:
        //xmlWriter().writeEmptyElement("br");
        break;
    case Atom::Link:
    {
        const Node *node = 0;
        QString myLink = getLink(atom, relative, marker, &node);
        if (myLink.isEmpty()) {
            relative->doc().location().warning(tr("Can't link to '%1' in %2")
                                               .arg(atom->string())
                                               .arg(marker->plainFullName(relative)));
        }
        else if (!inSectionHeading) {
            beginLink(myLink);
        }
#if 0
        else {
            //xmlWriter().writeCharacters(atom->string());
            //qDebug() << "MYLINK:" << myLink << outFileName() << atom->string();
        }
#endif
        skipAhead = 1;
    }
        break;
    case Atom::GuidLink:
    {
        beginLink(atom->string());
        skipAhead = 1;
    }
        break;
    case Atom::LinkNode:
    {
        const Node* node = CodeMarker::nodeForString(atom->string());
        beginLink(linkForNode(node, relative));
        skipAhead = 1;
    }
        break;
    case Atom::ListLeft:
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        if (atom->string() == ATOM_LIST_BULLET) {
            writeStartTag(DT_ul);
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            writeStartTag(DT_dl);
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            threeColumnEnumValueTable = isThreeColumnEnumValueTable(atom);
            if (threeColumnEnumValueTable) {
                writeStartTag(DT_simpletable);
                xmlWriter().writeAttribute("outputclass","valuelist");
                writeStartTag(DT_sthead);
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Constant");
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Value");
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Description");
                writeEndTag(); // </stentry>
                writeEndTag(); // </sthead>
            }
            else {
                writeStartTag(DT_simpletable);
                xmlWriter().writeAttribute("outputclass","valuelist");
                writeStartTag(DT_sthead);
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Constant");
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
                xmlWriter().writeCharacters("Value");
                writeEndTag(); // </stentry>
                writeEndTag(); // </sthead>
            }
        }
        else {
            writeStartTag(DT_ol);
            if (atom->string() == ATOM_LIST_UPPERALPHA)
                xmlWriter().writeAttribute("outputclass","upperalpha");
            else if (atom->string() == ATOM_LIST_LOWERALPHA)
                xmlWriter().writeAttribute("outputclass","loweralpha");
            else if (atom->string() == ATOM_LIST_UPPERROMAN)
                xmlWriter().writeAttribute("outputclass","upperroman");
            else if (atom->string() == ATOM_LIST_LOWERROMAN)
                xmlWriter().writeAttribute("outputclass","lowerroman");
            else // (atom->string() == ATOM_LIST_NUMERIC)
                xmlWriter().writeAttribute("outputclass","numeric");
            if (atom->next() != 0 && atom->next()->string().toInt() != 1) {
                // I don't think this attribute is supported.
                xmlWriter().writeAttribute("start",atom->next()->string());
            }
        }
        break;
    case Atom::ListItemNumber:
        // nothing
        break;
    case Atom::ListTagLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            writeStartTag(DT_dt);
        }
        else { // (atom->string() == ATOM_LIST_VALUE)
            writeStartTag(DT_strow);
            writeStartTag(DT_stentry);
            writeStartTag(DT_tt);
            writeCharacters(protectEnc(plainCode(marker->markedUpEnumValue(atom->next()->string(),
                                                                           relative))));
            writeEndTag(); // </tt>
            writeEndTag(); // </stentry>
            writeStartTag(DT_stentry);

            QString itemValue;
            if (relative->type() == Node::Enum) {
                const EnumNode *enume = static_cast<const EnumNode *>(relative);
                itemValue = enume->itemValue(atom->next()->string());
            }

            if (itemValue.isEmpty())
                xmlWriter().writeCharacters("?");
            else {
                writeStartTag(DT_tt);
                writeCharacters(protectEnc(itemValue));
                writeEndTag(); // </tt>
            }
            skipAhead = 1;
        }
        break;
    case Atom::ListTagRight:
        if (atom->string() == ATOM_LIST_TAG)
            writeEndTag(); // </dt>
        break;
    case Atom::ListItemLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            writeStartTag(DT_dd);
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            if (threeColumnEnumValueTable) {
                writeEndTag(); // </stentry>
                writeStartTag(DT_stentry);
            }
        }
        else {
            writeStartTag(DT_li);
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::ListItemRight:
        if (atom->string() == ATOM_LIST_TAG) {
            writeEndTag(); // </dd>
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            writeEndTag(); // </stentry>
            writeEndTag(); // </strow>
        }
        else {
            writeEndTag(); // </li>
        }
        break;
    case Atom::ListRight:
        if (atom->string() == ATOM_LIST_BULLET) {
            writeEndTag(); // </ul>
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            writeEndTag(); // </dl>
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            writeEndTag(); // </simpletable>
        }
        else {
            writeEndTag(); // </ol>
        }
        break;
    case Atom::Nop:
        // nothing
        break;
    case Atom::ParaLeft:
        writeStartTag(DT_p);
        if (inLegaleseText)
            xmlWriter().writeAttribute("outputclass","legalese");
        in_para = true;
        break;
    case Atom::ParaRight:
        endLink();
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        break;
    case Atom::QuotationLeft:
        writeStartTag(DT_lq);
        break;
    case Atom::QuotationRight:
        writeEndTag(); // </lq>
        break;
    case Atom::RawString:
        if (atom->string() == " ")
            break;
        if (atom->string().startsWith(QLatin1Char('&')))
            writeCharacters(atom->string());
        else if (atom->string() == "<sup>*</sup>") {
            writeStartTag(DT_sup);
            writeCharacters("*");
            writeEndTag(); // </sup>
        }
        else if (atom->string() == "<sup>&reg;</sup>") {
            writeStartTag(DT_tm);
            xmlWriter().writeAttribute("tmtype","reg");
            writeEndTag(); // </tm>
        }
        else {
            writeStartTag(DT_pre);
            xmlWriter().writeAttribute("outputclass","raw-html");
            writeCharacters(atom->string());
            writeEndTag(); // </pre>
        }
        break;
    case Atom::SectionLeft:
#if 0
        if (inApiDesc) {
            writeEndTag(); // </apiDesc>
            inApiDesc = false;
        }
#endif
        enterSection("details",QString());
        //writeGuidAttribute(Doc::canonicalTitle(Text::sectionHeading(atom).toString()));
        break;
    case Atom::SectionRight:
        leaveSection();
        break;
    case Atom::SectionHeadingLeft:
    {
        writeStartTag(DT_p);
        QString id = Text::sectionHeading(atom).toString();
        id = stripMarkup(id);
        id = Doc::canonicalTitle(id);
        writeGuidAttribute(id);
        hx = QLatin1Char('h') + QString::number(atom->string().toInt() + hOffset(relative));
        xmlWriter().writeAttribute("outputclass",hx);
        inSectionHeading = true;
    }
        break;
    case Atom::SectionHeadingRight:
        writeEndTag(); // </title> (see case Atom::SectionHeadingLeft)
        inSectionHeading = false;
        break;
    case Atom::SidebarLeft:
        // nothing
        break;
    case Atom::SidebarRight:
        // nothing
        break;
    case Atom::String:
        if (inLink && !inContents && !inSectionHeading) {
            generateLink(atom, relative, marker);
        }
        else {
            writeCharacters(atom->string());
        }
        break;
    case Atom::TableLeft:
    {
        QString attr;
        if ((atom->count() > 0) && (atom->string(0) == "borderless"))
            attr = "borderless";
        else if ((atom->count() > 1) && (atom->string(1) == "borderless"))
            attr = "borderless";
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        writeStartTag(DT_table);
        if (!attr.isEmpty())
            xmlWriter().writeAttribute("outputclass",attr);
        numTableRows = 0;
        if (tableColumnCount != 0) {
            qDebug() << "ERROR: Nested tables!";
            tableColumnCount = 0;
        }
        tableColumnCount = countTableColumns(atom->next());
        writeStartTag(DT_tgroup);
        xmlWriter().writeAttribute("cols",QString::number(tableColumnCount));
        inTableHeader = false;
        inTableBody = false;
    }
        break;
    case Atom::TableRight:
        writeEndTag(); // </tbody>
        writeEndTag(); // </tgroup>
        writeEndTag(); // </table>
        inTableHeader = false;
        inTableBody = false;
        tableColumnCount = 0;
        break;
    case Atom::TableHeaderLeft:
        if (inTableBody) {
            writeEndTag(); // </tbody>
            writeEndTag(); // </tgroup>
            writeEndTag(); // </table>
            inTableHeader = false;
            inTableBody = false;
            tableColumnCount = 0;
            writeStartTag(DT_table);
            numTableRows = 0;
            tableColumnCount = countTableColumns(atom);
            writeStartTag(DT_tgroup);
            xmlWriter().writeAttribute("cols",QString::number(tableColumnCount));
        }
        writeStartTag(DT_thead);
        xmlWriter().writeAttribute("valign","top");
        writeStartTag(DT_row);
        xmlWriter().writeAttribute("valign","top");
        inTableHeader = true;
        inTableBody = false;
        break;
    case Atom::TableHeaderRight:
        writeEndTag(); // </row>
        if (matchAhead(atom, Atom::TableHeaderLeft)) {
            skipAhead = 1;
            writeStartTag(DT_row);
            xmlWriter().writeAttribute("valign","top");
        }
        else {
            writeEndTag(); // </thead>
            inTableHeader = false;
            inTableBody = true;
            writeStartTag(DT_tbody);
        }
        break;
    case Atom::TableRowLeft:
        if (!inTableHeader && !inTableBody) {
            inTableBody = true;
            writeStartTag(DT_tbody);
        }
        writeStartTag(DT_row);
        attr = atom->string();
        if (!attr.isEmpty()) {
            if (attr.contains('=')) {
                int index = 0;
                int from = 0;
                QString values;
                while (index >= 0) {
                    index = attr.indexOf('"',from);
                    if (index >= 0) {
                        ++index;
                        from = index;
                        index = attr.indexOf('"',from);
                        if (index > from) {
                            if (!values.isEmpty())
                                values.append(' ');
                            values += attr.mid(from,index-from);
                            from = index+1;
                        }
                    }
                }
                attr = values;
            }
            xmlWriter().writeAttribute("outputclass", attr);
        }
        xmlWriter().writeAttribute("valign","top");
        break;
    case Atom::TableRowRight:
        writeEndTag(); // </row>
        break;
    case Atom::TableItemLeft:
    {
        QString values;
        writeStartTag(DT_entry);
        for (int i=0; i<atom->count(); ++i) {
            attr = atom->string(i);
            if (attr.contains('=')) {
                int index = 0;
                int from = 0;
                while (index >= 0) {
                    index = attr.indexOf('"',from);
                    if (index >= 0) {
                        ++index;
                        from = index;
                        index = attr.indexOf('"',from);
                        if (index > from) {
                            if (!values.isEmpty())
                                values.append(' ');
                            values += attr.mid(from,index-from);
                            from = index+1;
                        }
                    }
                }
            }
            else {
                QStringList spans = attr.split(QLatin1Char(','));
                if (spans.size() == 2) {
                    if ((spans[0].toInt()>1) || (spans[1].toInt()>1)) {
                        values += "span(" + spans[0] + QLatin1Char(',') + spans[1] + QLatin1Char(')');
                    }
                }
            }
        }
        if (!values.isEmpty())
            xmlWriter().writeAttribute("outputclass",values);
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
    }
        break;
    case Atom::TableItemRight:
        if (inTableHeader)
            writeEndTag(); // </entry>
        else {
            writeEndTag(); // </entry>
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::TableOfContents:
    {
        int numColumns = 1;
        const Node* node = relative;

        Doc::Sections sectionUnit = Doc::Section4;
        QStringList params = atom->string().split(QLatin1Char(','));
        QString columnText = params.at(0);
        QStringList pieces = columnText.split(QLatin1Char(' '), QString::SkipEmptyParts);
        if (pieces.size() >= 2) {
            columnText = pieces.at(0);
            pieces.pop_front();
            QString path = pieces.join(" ").trimmed();
            node = findNodeForTarget(path, relative, marker, atom);
        }

        if (params.size() == 2) {
            numColumns = qMax(columnText.toInt(), numColumns);
            sectionUnit = (Doc::Sections)params.at(1).toInt();
        }

        if (node)
            generateTableOfContents(node,
                                    marker,
                                    sectionUnit,
                                    numColumns,
                                    relative);
    }
        break;
    case Atom::Target:
        if (in_para) {
            writeEndTag(); // </p>
            in_para = false;
        }
        writeStartTag(DT_p);
        writeGuidAttribute(Doc::canonicalTitle(atom->string()));
        xmlWriter().writeAttribute("outputclass","target");
        //xmlWriter().writeCharacters(protectEnc(atom->string()));
        writeEndTag(); // </p>
        break;
    case Atom::UnhandledFormat:
        writeStartTag(DT_b);
        xmlWriter().writeAttribute("outputclass","error");
        xmlWriter().writeCharacters("<Missing DITAXML>");
        writeEndTag(); // </b>
        break;
    case Atom::UnknownCommand:
        writeStartTag(DT_b);
        xmlWriter().writeAttribute("outputclass","error unknown-command");
        writeCharacters(protectEnc(atom->string()));
        writeEndTag(); // </b>
        break;
    case Atom::QmlText:
    case Atom::EndQmlText:
        // don't do anything with these. They are just tags.
        break;
    default:
        //        unknownAtom(atom);
        break;
    }
    return skipAhead;
}

/*!
  Generate a <cxxClass> element (and all the stuff inside it)
  for the C++ class represented by \a innerNode. \a marker is
  for marking up the code. I don't know what that means exactly.
 */
void
DitaXmlGenerator::generateClassLikeNode(const InnerNode* inner, CodeMarker* marker)
{
    QList<Section>::ConstIterator s;

    QString title;
    QString rawTitle;
    QString fullTitle;
    if (inner->type() == Node::Namespace) {
        const NamespaceNode* nsn = const_cast<NamespaceNode*>(static_cast<const NamespaceNode*>(inner));
        rawTitle = marker->plainName(inner);
        fullTitle = marker->plainFullName(inner);
        title = rawTitle + " Namespace";

        /*
          Note: Because the C++ specialization we are using
          has no <cxxNamespace> element, we are using the
          <cxxClass> element with an outputclass attribute
          set to "namespace" .
         */
        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        writeStartTag(DT_cxxClassDefinition);
        writeLocation(nsn);
        writeEndTag(); // <cxxClassDefinition>

        enterApiDesc(QString(),title);
#if 0
        // To be removed, if really not needed.
        Text brief = nsn->doc().briefText(); // zzz
        if (!brief.isEmpty()) {
            writeStartTag(DT_p);
            generateText(brief, nsn, marker);
            writeEndTag(); // </p>
        }
#endif
        generateStatus(nsn, marker);
        generateThreadSafeness(nsn, marker);
        generateSince(nsn, marker);

        enterSection("h2","Detailed Description");
        generateBody(nsn, marker);
        generateAlsoList(nsn, marker);
        leaveSection();
        leaveSection(); // </apiDesc>

        bool needOtherSection = false;
        QList<Section> summarySections;
        summarySections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.begin();
            while (s != summarySections.end()) {
                if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
                    if (!s->inherited.isEmpty())
                        needOtherSection = true;
                }
                else {
                    QString attr;
                    if (!s->members.isEmpty()) {
                        writeStartTag(DT_p);
                        attr  = cleanRef((*s).name).toLower() + " h2";
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc((*s).name));
                        writeEndTag(); // </title>
                        generateSection(s->members, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner, marker);
                    }
                    if (!s->reimpMembers.isEmpty()) {
                        QString name = QString("Reimplemented ") + (*s).name;
                        attr = cleanRef(name).toLower() + " h2";
                        writeStartTag(DT_p);
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc(name));
                        writeEndTag(); // </title>
                        generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner, marker);
                    }
                }
                ++s;
            }
            if (needOtherSection) {
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","h3");
                xmlWriter().writeCharacters("Additional Inherited Members");
                writeEndTag(); // </title>
                s = summarySections.begin();
                while (s != summarySections.end()) {
                    if (s->members.isEmpty())
                        generateSectionInheritedList(*s, inner, marker);
                    ++s;
                }
            }
            leaveSection();
        }

        writeEndTag(); // </cxxClassDetail>

        // not included: <related-links>
        // not included: <cxxClassNested>

        QList<Section> detailSections;
        detailSections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
        s = detailSections.begin();
        while (s != detailSections.end()) {
            if ((*s).name == "Classes") {
                writeNestedClasses((*s),nsn);
                break;
            }
            ++s;
        }

        s = detailSections.begin();
        while (s != detailSections.end()) {
            if ((*s).name == "Function Documentation") {
                writeFunctions((*s),nsn,marker);
            }
            else if ((*s).name == "Type Documentation") {
                writeEnumerations((*s),marker);
                writeTypedefs((*s),marker);
            }
            else if ((*s).name == "Namespaces") {
                qDebug() << "Nested namespaces" << outFileName();
            }
            else if ((*s).name == "Macro Documentation") {
                //writeMacros((*s),marker);
            }
            ++s;
        }

        generateLowStatusMembers(inner,marker,CodeMarker::Obsolete);
        generateLowStatusMembers(inner,marker,CodeMarker::Compat);
        writeEndTag(); // </cxxClass>
    }
    else if (inner->type() == Node::Class) {
        const ClassNode* cn = const_cast<ClassNode*>(static_cast<const ClassNode*>(inner));
        rawTitle = marker->plainName(inner);
        fullTitle = marker->plainFullName(inner);
        title = rawTitle + " Class";

        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        writeStartTag(DT_cxxClassDefinition);
        writeStartTag(DT_cxxClassAccessSpecifier);
        xmlWriter().writeAttribute("value",inner->accessString());
        writeEndTag(); // <cxxClassAccessSpecifier>
        if (cn->isAbstract()) {
            writeStartTag(DT_cxxClassAbstract);
            xmlWriter().writeAttribute("name","abstract");
            xmlWriter().writeAttribute("value","abstract");
            writeEndTag(); // </cxxClassAbstract>
        }
        writeDerivations(cn, marker); // <cxxClassDerivations>

        // not included: <cxxClassTemplateParameters>

        writeLocation(cn);
        writeEndTag(); // <cxxClassDefinition>

        enterApiDesc(QString(),title);
#if 0
        // To be removed, if really not needed.
        Text brief = cn->doc().briefText(); // zzz
        if (!brief.isEmpty()) {
            writeStartTag(DT_p);
            generateText(brief, cn, marker);
            writeEndTag(); // </p>
        }
#endif
        generateStatus(cn, marker);
        generateInherits(cn, marker);
        generateInheritedBy(cn, marker);
        generateThreadSafeness(cn, marker);
        generateSince(cn, marker);
        enterSection("h2","Detailed Description");
        generateBody(cn, marker);
        generateAlsoList(cn, marker);
        leaveSection();
        leaveSection(); // </apiDesc>

        bool needOtherSection = false;
        QList<Section> summarySections;
        summarySections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.begin();
            while (s != summarySections.end()) {
                if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
                    if (!s->inherited.isEmpty())
                        needOtherSection = true;
                }
                else {
                    QString attr;
                    if (!s->members.isEmpty()) {
                        writeStartTag(DT_p);
                        attr = cleanRef((*s).name).toLower() + " h2";
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc((*s).name));
                        writeEndTag(); // </p>
                        generateSection(s->members, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner, marker);
                    }
                    if (!s->reimpMembers.isEmpty()) {
                        QString name = QString("Reimplemented ") + (*s).name;
                        attr = cleanRef(name).toLower() + " h2";
                        writeStartTag(DT_p);
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc(name));
                        writeEndTag(); // </p>
                        generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner, marker);
                    }
                }
                ++s;
            }
            if (needOtherSection) {
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","h3");
                xmlWriter().writeCharacters("Additional Inherited Members");
                writeEndTag(); // </p>
                s = summarySections.begin();
                while (s != summarySections.end()) {
                    if (s->members.isEmpty())
                        generateSectionInheritedList(*s, inner, marker);
                    ++s;
                }
            }
            leaveSection();
        }

        // not included: <example> or <apiImpl>

        writeEndTag(); // </cxxClassDetail>

        // not included: <related-links>
        // not included: <cxxClassNested>

        QList<Section> detailSections;
        detailSections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
        s = detailSections.begin();
        while (s != detailSections.end()) {
            if ((*s).name == "Member Function Documentation") {
                writeFunctions((*s),cn,marker);
            }
            else if ((*s).name == "Member Type Documentation") {
                writeEnumerations((*s),marker);
                writeTypedefs((*s),marker);
            }
            else if ((*s).name == "Member Variable Documentation") {
                writeDataMembers((*s),marker);
            }
            else if ((*s).name == "Property Documentation") {
                writeProperties((*s),marker);
            }
            else if ((*s).name == "Macro Documentation") {
                //writeMacros((*s),marker);
            }
            else if ((*s).name == "Related Non-Members") {
                QString attribute("related-non-member");
                writeFunctions((*s),cn,marker,attribute);
            }
            ++s;
        }

        generateLowStatusMembers(inner,marker,CodeMarker::Obsolete);
        generateLowStatusMembers(inner,marker,CodeMarker::Compat);
        writeEndTag(); // </cxxClass>
    }
    else if ((inner->type() == Node::Fake) && (inner->subType() == Node::HeaderFile)) {
        const FakeNode* fn = const_cast<FakeNode*>(static_cast<const FakeNode*>(inner));
        rawTitle = marker->plainName(inner);
        fullTitle = marker->plainFullName(inner);
        title = rawTitle;

        /*
          Note: Because the C++ specialization we are using
          has no <cxxHeaderFile> element, we are using the
          <cxxClass> element with an outputclass attribute
          set to "headerfile" .
         */
        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        enterApiDesc(QString(),title);
#if 0
        // To be removed, if really not needed.
        Text brief = fn->doc().briefText(); // zzz
        if (!brief.isEmpty()) {
            writeStartTag(DT_p);
            generateText(brief, fn, marker);
            writeEndTag(); // </p>
        }
#endif
        generateStatus(fn, marker);
        generateThreadSafeness(fn, marker);
        generateSince(fn, marker);
        generateSince(fn, marker);
        enterSection("h2","Detailed Description");
        generateBody(fn, marker);
        generateAlsoList(fn, marker);
        leaveSection();
        leaveSection(); // </apiDesc>

        bool needOtherSection = false;
        QList<Section> summarySections;
        summarySections = marker->sections(inner, CodeMarker::Summary, CodeMarker::Okay);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.begin();
            while (s != summarySections.end()) {
                if (s->members.isEmpty() && s->reimpMembers.isEmpty()) {
                    if (!s->inherited.isEmpty())
                        needOtherSection = true;
                }
                else {
                    QString attr;
                    if (!s->members.isEmpty()) {
                        writeStartTag(DT_p);
                        attr = cleanRef((*s).name).toLower() + " h2";
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc((*s).name));
                        writeEndTag(); // </p>
                        generateSection(s->members, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner, marker);
                    }
                    if (!s->reimpMembers.isEmpty()) {
                        QString name = QString("Reimplemented ") + (*s).name;
                        attr = cleanRef(name).toLower() + " h2";
                        writeStartTag(DT_p);
                        xmlWriter().writeAttribute("outputclass",attr);
                        writeCharacters(protectEnc(name));
                        writeEndTag(); // </p>
                        generateSection(s->reimpMembers, inner, marker, CodeMarker::Summary);
                        generateSectionInheritedList(*s, inner, marker);
                    }
                }
                ++s;
            }
            if (needOtherSection) {
                enterSection("additional-inherited-members redundant",QString());
                writeStartTag(DT_p);
                xmlWriter().writeAttribute("outputclass","h3");
                xmlWriter().writeCharacters("Additional Inherited Members");
                writeEndTag(); // </p>
                s = summarySections.begin();
                while (s != summarySections.end()) {
                    if (s->members.isEmpty())
                        generateSectionInheritedList(*s, inner, marker);
                    ++s;
                }
            }
            leaveSection();
        }

        writeEndTag(); // </cxxClassDetail>

        // not included: <related-links>
        // not included: <cxxClassNested>

        QList<Section> detailSections;
        detailSections = marker->sections(inner, CodeMarker::Detailed, CodeMarker::Okay);
        s = detailSections.begin();
        while (s != detailSections.end()) {
            if ((*s).name == "Classes") {
                writeNestedClasses((*s),fn);
                break;
            }
            ++s;
        }

        s = detailSections.begin();
        while (s != detailSections.end()) {
            if ((*s).name == "Function Documentation") {
                writeFunctions((*s),fn,marker);
            }
            else if ((*s).name == "Type Documentation") {
                writeEnumerations((*s),marker);
                writeTypedefs((*s),marker);
            }
            else if ((*s).name == "Namespaces") {
                qDebug() << "Nested namespaces" << outFileName();
            }
            else if ((*s).name == "Macro Documentation") {
                //writeMacros((*s),marker);
            }
            ++s;
        }
        generateLowStatusMembers(inner,marker,CodeMarker::Obsolete);
        generateLowStatusMembers(inner,marker,CodeMarker::Compat);
        writeEndTag(); // </cxxClass>
    }
    else if ((inner->type() == Node::Fake) && (inner->subType() == Node::QmlClass)) {
        const QmlClassNode* qcn = const_cast<QmlClassNode*>(static_cast<const QmlClassNode*>(inner));
        const ClassNode* cn = qcn->classNode();
        rawTitle = marker->plainName(inner);
        fullTitle = marker->plainFullName(inner);
        title = rawTitle + " Element";
        //QString fullTitle = fake->fullTitle();
        //QString htmlTitle = fullTitle;

        generateHeader(inner, fullTitle);
        generateBrief(inner, marker); // <shortdesc>
        writeProlog(inner);

        writeStartTag(DT_cxxClassDetail);
        enterApiDesc(QString(),title);
#if 0
        // To be removed, if really not needed.
        Text brief = qcn->doc().briefText(); // zzz
        if (!brief.isEmpty()) {
            writeStartTag(DT_p);
            generateText(brief, qcn, marker);
            writeEndTag(); // </p>
        }
#endif
        generateQmlInstantiates(qcn, marker);
        generateQmlInherits(qcn, marker);
        generateQmlInheritedBy(qcn, marker);
        generateSince(qcn, marker);
        enterSection("h2","Detailed Description");
        generateBody(qcn, marker);
        if (cn) {
            generateQmlText(cn->doc().body(), cn, marker, qcn->name());
            generateAlsoList(cn, marker);
        }
        leaveSection();
        leaveSection(); // </apiDesc>

        QList<Section> summarySections;
        summarySections = marker->qmlSections(qcn,CodeMarker::Summary);
        if (!summarySections.isEmpty()) {
            enterSection("redundant",QString());
            s = summarySections.begin();
            while (s != summarySections.end()) {
                QString attr;
                if (!s->members.isEmpty()) {
                    writeStartTag(DT_p);
                    attr = cleanRef((*s).name).toLower() + " h2";
                    xmlWriter().writeAttribute("outputclass",attr);
                    writeCharacters(protectEnc((*s).name));
                    writeEndTag(); // </p>
                    generateQmlSummary(*s,qcn,marker);
                    //generateSection(s->members, inner, marker, CodeMarker::Summary);
                    //generateSectionInheritedList(*s, inner, marker);
                }
                ++s;
            }
            leaveSection();
        }

        QList<Section> detailSections;
        detailSections = marker->qmlSections(qcn,CodeMarker::Detailed);
        if (!detailSections.isEmpty()) {
            enterSection("details",QString());
            s = detailSections.begin();
            while (s != detailSections.end()) {
                if (!s->members.isEmpty()) {
                    QString attr;
                    writeStartTag(DT_p);
                    attr = cleanRef((*s).name).toLower() + " h2";
                    xmlWriter().writeAttribute("outputclass",attr);
                    writeCharacters(protectEnc((*s).name));
                    writeEndTag(); // </p>
                    NodeList::ConstIterator m = (*s).members.begin();
                    while (m != (*s).members.end()) {
                        generateDetailedQmlMember(*m, qcn, marker);
                        ++m;
                    }
                }
                ++s;
            }
            leaveSection();
        }
        writeEndTag(); // </cxxClassDetail>
        writeEndTag(); // </cxxClass>
    }
}


/*!
  Write a list item for a \a link with the given \a text.
 */
void DitaXmlGenerator::writeXrefListItem(const QString& link, const QString& text)
{
    writeStartTag(DT_li);
    writeStartTag(DT_xref);
    // formathtml
    writeHrefAttribute(link);
    writeCharacters(text);
    writeEndTag(); // </xref>
    writeEndTag(); // </li>
}

/*!
  Generate the DITA page for a qdoc file that doesn't map
  to an underlying c++ file.
 */
void DitaXmlGenerator::generateFakeNode(const FakeNode* fake, CodeMarker* marker)
{
    /*
      If the fake node is a page node, and if the page type
      is DITA map page, write the node's contents as a dita
      map and return without doing anything else.
     */
    if (fake->subType() == Node::Page && fake->pageType() == Node::DitaMapPage) {
        const DitaMapNode* dmn = static_cast<const DitaMapNode*>(fake);
        writeDitaMap(dmn);
        return;
    }

    QList<Section> sections;
    QList<Section>::const_iterator s;
    QString fullTitle = fake->fullTitle();

    if (fake->subType() == Node::QmlBasicType) {
        fullTitle = "QML Basic Type: " + fullTitle;
    }
    else if (fake->subType() == Node::Collision) {
        fullTitle = "Name Collision: " + fullTitle;
    }

    generateHeader(fake, fullTitle);
    generateBrief(fake, marker); // <shortdesc>
    writeProlog(fake);

    writeStartTag(DT_body);
    enterSection(QString(),QString());
    if (fake->subType() == Node::Module) {
        generateStatus(fake, marker);
        if (moduleNamespaceMap.contains(fake->name())) {
            enterSection("h2","Namespaces");
            generateAnnotatedList(fake, marker, moduleNamespaceMap[fake->name()]);
            leaveSection();
        }
        if (moduleClassMap.contains(fake->name())) {
            enterSection("h2","Classes");
            generateAnnotatedList(fake, marker, moduleClassMap[fake->name()]);
            leaveSection();
        }
    }

    if (fake->doc().isEmpty()) {
        if (fake->subType() == Node::File) {
            Text text;
            Quoter quoter;
            writeStartTag(DT_p);
            xmlWriter().writeAttribute("outputclass", "small-subtitle");
            text << fake->subTitle();
            generateText(text, fake, marker);
            writeEndTag(); // </p>
            Doc::quoteFromFile(fake->doc().location(), quoter, fake->name());
            QString code = quoter.quoteTo(fake->location(), "", "");
            text.clear();
            text << Atom(Atom::Code, code);
            generateText(text, fake, marker);
        }
    }
    else {
        if (fake->subType() == Node::Module) {
            enterSection("h2","Detailed Description");
            generateBody(fake, marker);
            leaveSection();
        }
        else {
            generateBody(fake, marker);
        }
        generateAlsoList(fake, marker);

        if ((fake->subType() == Node::QmlModule) && !fake->qmlModuleMembers().isEmpty()) {
            NodeMap qmlModuleMembersMap;
            foreach (const Node* node, fake->qmlModuleMembers()) {
                if (node->type() == Node::Fake && node->subType() == Node::QmlClass)
                    qmlModuleMembersMap[node->name()] = node;
            }
            generateAnnotatedList(fake, marker, qmlModuleMembersMap);
        }
        else if (!fake->groupMembers().isEmpty()) {
            NodeMap groupMembersMap;
            foreach (const Node *node, fake->groupMembers()) {
                if (node->type() == Node::Class || node->type() == Node::Namespace)
                    groupMembersMap[node->name()] = node;
            }
            generateAnnotatedList(fake, marker, groupMembersMap);
        }
    }
    leaveSection(); // </section>
    if (!writeEndTag()) { // </body>
        fake->doc().location().warning(tr("Pop of empty XML tag stack; generating DITA for '%1'").arg(fake->name()));
        return;
    }
    writeRelatedLinks(fake, marker);
    writeEndTag(); // </topic>
}

/*!
  This function writes a \e{<link>} element inside a
  \e{<related-links>} element.

  \sa writeRelatedLinks()
 */
void DitaXmlGenerator::writeLink(const Node* node,
                                 const QString& text,
                                 const QString& role)
{
    if (node) {
        QString link = fileName(node) + QLatin1Char('#') + node->guid();
        if (link.endsWith("#"))
            qDebug() << "LINK ENDS WITH #:" << link << outFileName();
        writeStartTag(DT_link);
        writeHrefAttribute(link);
        xmlWriter().writeAttribute("role", role);
        writeStartTag(DT_linktext);
        writeCharacters(text);
        writeEndTag(); // </linktext>
        writeEndTag(); // </link>
    }
}

/*!
  This function writes a \e{<related-links>} element, which
  contains the \c{next}, \c{previous}, and \c{start}
  links for topic pages that have them. Note that the
  value of the \e role attribute is \c{parent} for the
  \c{start} link.
 */
void DitaXmlGenerator::writeRelatedLinks(const FakeNode* node, CodeMarker* marker)
{
    const Node* linkNode = 0;
    QPair<QString,QString> linkPair;
    if (node && !node->links().empty()) {
        writeStartTag(DT_relatedLinks);
        if (node->links().contains(Node::PreviousLink)) {
            linkPair = node->links()[Node::PreviousLink];
            linkNode = findNodeForTarget(linkPair.first, node, marker);
            writeLink(linkNode, linkPair.second, "previous");
        }
        if (node->links().contains(Node::NextLink)) {
            linkPair = node->links()[Node::NextLink];
            linkNode = findNodeForTarget(linkPair.first, node, marker);
            writeLink(linkNode, linkPair.second, "next");
        }
        if (node->links().contains(Node::StartLink)) {
            linkPair = node->links()[Node::StartLink];
            linkNode = findNodeForTarget(linkPair.first, node, marker);
            writeLink(linkNode, linkPair.second, "parent");
        }
        writeEndTag(); // </related-links>
    }
}

/*!
  Returns "dita" for this subclass of class Generator.
 */
QString DitaXmlGenerator::fileExtension(const Node * /* node */) const
{
    return "dita";
}

/*!
  Writes an XML file header to the current XML stream. This
  depends on which kind of DITA XML file is being generated,
  which is determined by the \a node type and subtype and the
  \a subpage flag. If the \subpage flag is true, a \c{<topic>}
  header is written, regardless of the type of \a node.
 */
void DitaXmlGenerator::generateHeader(const Node* node,
                                      const QString& name,
                                      bool subpage)
{
    if (!node)
        return;

    DitaTag mainTag = DT_cxxClass;
    DitaTag nameTag = DT_apiName;
    QString doctype;
    QString dtd;
    QString base;
    QString version;
    QString outputclass;

    if (node->type() == Node::Class) {
        mainTag = DT_cxxClass;
        nameTag = DT_apiName;
        dtd = "dtd/cxxClass.dtd";
        version = "0.7.0";
        doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                version + "//EN\" \"" + dtd + "\">";
    }
    else if (node->type() == Node::Namespace) {
        mainTag = DT_cxxClass;
        nameTag = DT_apiName;
        dtd = "dtd/cxxClass.dtd";
        version = "0.7.0";
        doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                version + "//EN\" \"" + dtd + "\">";
        outputclass = "namespace";
    }
    else if (node->type() == Node::Fake || subpage) {
        if (node->subType() == Node::HeaderFile) {
            mainTag = DT_cxxClass;
            nameTag = DT_apiName;
            dtd = "dtd/cxxClass.dtd";
            version = "0.7.0";
            doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                    " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                    version + "//EN\" \"" + dtd + "\">";
            outputclass = "headerfile";
        }
        else if (node->subType() == Node::QmlClass) {
            mainTag = DT_cxxClass;
            nameTag = DT_apiName;
            dtd = "dtd/cxxClass.dtd";
            version = "0.7.0";
            doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                    " PUBLIC \"-//NOKIA//DTD DITA C++ API Class Reference Type v" +
                    version + "//EN\" \"" + dtd + "\">";
            outputclass = "QML-class";
        }
        else {
            mainTag = DT_topic;
            nameTag = DT_title;
            dtd = "dtd/topic.dtd";
            doctype = "<!DOCTYPE " + ditaTags[mainTag] +
                    " PUBLIC \"-//OASIS//DTD DITA Topic//EN\" \"" + dtd + "\">";
            switch (node->subType()) {
            case Node::Page:
                outputclass = node->pageTypeString();
                break;
            case Node::Group:
                outputclass = "group";
                break;
            case Node::Example:
                outputclass = "example";
                break;
            case Node::File:
                outputclass = "file";
                break;
            case Node::Image:  // not used
                outputclass = "image";
                break;
            case Node::Module:
                outputclass = "module";
                break;
            case Node::ExternalPage: // not used
                outputclass = "externalpage";
                break;
            default:
                outputclass = "page";
            }
        }
    }

    xmlWriter().writeDTD(doctype);
    xmlWriter().writeComment(node->doc().location().fileName());
    writeStartTag(mainTag);
    QString id = node->guid();
    xmlWriter().writeAttribute("id",id);
    if (!outputclass.isEmpty())
        xmlWriter().writeAttribute("outputclass",outputclass);
    writeStartTag(nameTag); // <title> or <apiName>
    writeCharacters(name);
    writeEndTag(); // </title> or </apiName>
}

/*!
  Outputs the \e brief command as a <shortdesc> element.
 */
void DitaXmlGenerator::generateBrief(const Node* node, CodeMarker* marker)
{
    Text brief = node->doc().briefText(true); // zzz
    if (!brief.isEmpty()) {
        generateText(brief, node, marker);
    }
}

/*!
  zzz
  Generates a table of contents beginning at \a node.
  Currently just returns without writing anything.
 */
void DitaXmlGenerator::generateTableOfContents(const Node* node,
                                               CodeMarker* marker,
                                               Doc::Sections sectionUnit,
                                               int numColumns,
                                               const Node* relative)

{
    return;
    if (!node->doc().hasTableOfContents())
        return;
    QList<Atom *> toc = node->doc().tableOfContents();
    if (toc.isEmpty())
        return;

    QString nodeName = "";
    if (node != relative)
        nodeName = node->name();

    QStringList sectionNumber;
    int columnSize = 0;

    QString tdTag;
    if (numColumns > 1) {
        tdTag = "<td>"; /* width=\"" + QString::number((100 + numColumns - 1) / numColumns) + "%\">";*/
        out() << "<table class=\"toc\">\n<tr class=\"topAlign\">"
              << tdTag << '\n';
    }

    // disable nested links in table of contents
    inContents = true;
    inLink = true;

    for (int i = 0; i < toc.size(); ++i) {
        Atom *atom = toc.at(i);

        int nextLevel = atom->string().toInt();
        if (nextLevel > (int)sectionUnit)
            continue;

        if (sectionNumber.size() < nextLevel) {
            do {
                out() << "<ul>";
                sectionNumber.append("1");
            } while (sectionNumber.size() < nextLevel);
        }
        else {
            while (sectionNumber.size() > nextLevel) {
                out() << "</ul>\n";
                sectionNumber.removeLast();
            }
            sectionNumber.last() = QString::number(sectionNumber.last().toInt() + 1);
        }
        int numAtoms;
        Text headingText = Text::sectionHeading(atom);

        if (sectionNumber.size() == 1 && columnSize > toc.size() / numColumns) {
            out() << "</ul></td>" << tdTag << "<ul>\n";
            columnSize = 0;
        }
        out() << "<li>";
        out() << "<xref href=\""
              << nodeName
              << "#"
              << Doc::canonicalTitle(headingText.toString())
              << "\">";
        generateAtomList(headingText.firstAtom(), node, marker, true, numAtoms);
        out() << "</xref></li>\n";

        ++columnSize;
    }
    while (!sectionNumber.isEmpty()) {
        out() << "</ul>\n";
        sectionNumber.removeLast();
    }

    if (numColumns > 1)
        out() << "</td></tr></table>\n";

    inContents = false;
    inLink = false;
}

/*!
  zzz
  Revised for the new doc format.
  Generates a table of contents beginning at \a node.
 */
void DitaXmlGenerator::generateTableOfContents(const Node* node,
                                               CodeMarker* marker,
                                               QList<Section>* sections)
{
    QList<Atom*> toc;
    if (node->doc().hasTableOfContents())
        toc = node->doc().tableOfContents();
    if (toc.isEmpty() && !sections && (node->subType() != Node::Module))
        return;

    QStringList sectionNumber;
    int detailsBase = 0;

    // disable nested links in table of contents
    inContents = true;
    inLink = true;

    out() << "<div class=\"toc\">\n";
    out() << "<h3>Contents</h3>\n";
    sectionNumber.append("1");
    out() << "<ul>\n";

    if (node->subType() == Node::Module) {
        if (moduleNamespaceMap.contains(node->name())) {
            out() << "<li class=\"level"
                  << sectionNumber.size()
                  << "\"><xref href=\"#"
                  << registerRef("namespaces")
                  << "\">Namespaces</xref></li>\n";
        }
        if (moduleClassMap.contains(node->name())) {
            out() << "<li class=\"level"
                  << sectionNumber.size()
                  << "\"><xref href=\"#"
                  << registerRef("classes")
                  << "\">Classes</xref></li>\n";
        }
        out() << "<li class=\"level"
              << sectionNumber.size()
              << "\"><xref href=\"#"
              << registerRef("details")
              << "\">Detailed Description</xref></li>\n";
        for (int i = 0; i < toc.size(); ++i) {
            if (toc.at(i)->string().toInt() == 1) {
                detailsBase = 1;
                break;
            }
        }
    }
    else if (sections && (node->type() == Node::Class)) {
        QList<Section>::ConstIterator s = sections->begin();
        while (s != sections->end()) {
            if (!s->members.isEmpty() || !s->reimpMembers.isEmpty()) {
                out() << "<li class=\"level"
                      << sectionNumber.size()
                      << "\"><xref href=\"#"
                      << registerRef((*s).pluralMember)
                      << "\">" << (*s).name
                      << "</xref></li>\n";
            }
            ++s;
        }
        out() << "<li class=\"level"
              << sectionNumber.size()
              << "\"><xref href=\"#"
              << registerRef("details")
              << "\">Detailed Description</xref></li>\n";
        for (int i = 0; i < toc.size(); ++i) {
            if (toc.at(i)->string().toInt() == 1) {
                detailsBase = 1;
                break;
            }
        }
    }

    for (int i = 0; i < toc.size(); ++i) {
        Atom *atom = toc.at(i);
        int nextLevel = atom->string().toInt() + detailsBase;
        if (sectionNumber.size() < nextLevel) {
            do {
                sectionNumber.append("1");
            } while (sectionNumber.size() < nextLevel);
        }
        else {
            while (sectionNumber.size() > nextLevel) {
                sectionNumber.removeLast();
            }
            sectionNumber.last() = QString::number(sectionNumber.last().toInt() + 1);
        }
        int numAtoms;
        Text headingText = Text::sectionHeading(atom);
        QString s = headingText.toString();
        out() << "<li class=\"level"
              << sectionNumber.size()
              << "\">";
        out() << "<xref href=\""
              << "#"
              << Doc::canonicalTitle(s)
              << "\">";
        generateAtomList(headingText.firstAtom(), node, marker, true, numAtoms);
        out() << "</xref></li>\n";
    }
    while (!sectionNumber.isEmpty()) {
        sectionNumber.removeLast();
    }
    out() << "</ul>\n";
    out() << "</div>\n";
    inContents = false;
    inLink = false;
}

void DitaXmlGenerator::generateLowStatusMembers(const InnerNode* inner,
                                                CodeMarker* marker,
                                                CodeMarker::Status status)
{
    QString attribute;
    if (status == CodeMarker::Compat)
        attribute = "Qt3-support";
    else if (status == CodeMarker::Obsolete)
        attribute = "obsolete";
    else
        return;

    QList<Section> sections = marker->sections(inner, CodeMarker::Detailed, status);
    QMutableListIterator<Section> j(sections);
    while (j.hasNext()) {
        if (j.next().members.size() == 0)
            j.remove();
    }
    if (sections.isEmpty())
        return;

    QList<Section>::ConstIterator s = sections.begin();
    while (s != sections.end()) {
        if ((*s).name == "Member Function Documentation") {
            writeFunctions((*s),inner,marker,attribute);
        }
        else if ((*s).name == "Member Type Documentation") {
            writeEnumerations((*s),marker,attribute);
            writeTypedefs((*s),marker,attribute);
        }
        else if ((*s).name == "Member Variable Documentation") {
            writeDataMembers((*s),marker,attribute);
        }
        else if ((*s).name == "Property Documentation") {
            writeProperties((*s),marker,attribute);
        }
        else if ((*s).name == "Macro Documentation") {
            //writeMacros((*s),marker,attribute);
        }
        ++s;
    }
}

/*!
  Write the XML for the class hierarchy to the current XML stream.
 */
void DitaXmlGenerator::generateClassHierarchy(const Node* relative,
                                              CodeMarker* marker,
                                              const QMap<QString,const Node*>& classMap)
{
    if (classMap.isEmpty())
        return;

    NodeMap topLevel;
    NodeMap::ConstIterator c = classMap.begin();
    while (c != classMap.end()) {
        const ClassNode* classe = static_cast<const ClassNode*>(*c);
        if (classe->baseClasses().isEmpty())
            topLevel.insert(classe->name(), classe);
        ++c;
    }

    QStack<NodeMap > stack;
    stack.push(topLevel);

    writeStartTag(DT_ul);
    while (!stack.isEmpty()) {
        if (stack.top().isEmpty()) {
            stack.pop();
            writeEndTag(); // </ul>
            if (!stack.isEmpty())
                writeEndTag(); // </li>
        }
        else {
            const ClassNode *child =
                    static_cast<const ClassNode *>(*stack.top().begin());
            writeStartTag(DT_li);
            generateFullName(child, relative, marker);
            writeEndTag(); // </li>
            stack.top().erase(stack.top().begin());

            NodeMap newTop;
            foreach (const RelatedClass &d, child->derivedClasses()) {
                if (d.access != Node::Private && !d.node->doc().isEmpty())
                    newTop.insert(d.node->name(), d.node);
            }
            if (!newTop.isEmpty()) {
                stack.push(newTop);
                writeStartTag(DT_li);
                writeStartTag(DT_ul);
            }
        }
    }
}

/*!
  Write XML for the contents of the \a nodeMap to the current
  XML stream.
 */
void DitaXmlGenerator::generateAnnotatedList(const Node* relative,
                                             CodeMarker* marker,
                                             const NodeMap& nodeMap)
{
    if (nodeMap.isEmpty())
        return;
    writeStartTag(DT_table);
    xmlWriter().writeAttribute("outputclass","annotated");
    writeStartTag(DT_tgroup);
    xmlWriter().writeAttribute("cols","2");
    writeStartTag(DT_tbody);

    foreach (const QString& name, nodeMap.keys()) {
        const Node* node = nodeMap[name];

        if (node->status() == Node::Obsolete)
            continue;

        writeStartTag(DT_row);
        writeStartTag(DT_entry);
        writeStartTag(DT_p);
        generateFullName(node, relative, marker);
        writeEndTag(); // </p>
        writeEndTag(); // <entry>

        if (!(node->type() == Node::Fake)) {
            Text brief = node->doc().trimmedBriefText(name);
            if (!brief.isEmpty()) {
                writeStartTag(DT_entry);
                writeStartTag(DT_p);
                generateText(brief, node, marker);
                writeEndTag(); // </p>
                writeEndTag(); // <entry>
            }
        }
        else {
            writeStartTag(DT_entry);
            writeStartTag(DT_p);
            writeCharacters(protectEnc(node->doc().briefText().toString())); // zzz
            writeEndTag(); // </p>
            writeEndTag(); // <entry>
        }
        writeEndTag(); // </row>
    }
    writeEndTag(); // </tbody>
    writeEndTag(); // </tgroup>
    writeEndTag(); // </table>
}

/*!
  This function finds the common prefix of the names of all
  the classes in \a classMap and then generates a compact
  list of the class names alphabetized on the part of the
  name not including the common prefix. You can tell the
  function to use \a comonPrefix as the common prefix, but
  normally you let it figure it out itself by looking at
  the name of the first and last classes in \a classMap.
 */
void DitaXmlGenerator::generateCompactList(const Node* relative,
                                           CodeMarker* marker,
                                           const NodeMap& classMap,
                                           bool includeAlphabet,
                                           QString commonPrefix)
{
    const int NumParagraphs = 37; // '0' to '9', 'A' to 'Z', '_'

    if (classMap.isEmpty())
        return;

    /*
      If commonPrefix is not empty, then the caller knows what
      the common prefix is and has passed it in, so just use that
      one. But if the commonPrefix is empty (it normally is), then
      compute a common prefix using this simple algorithm. Note we
      assume the prefix length is 1, i.e. we will have a single
      character as the common prefix.
     */
    int commonPrefixLen = commonPrefix.length();
    if (commonPrefixLen == 0) {
        QVector<int> count(26);
        for (int i=0; i<26; ++i)
            count[i] = 0;

        NodeMap::const_iterator iter = classMap.begin();
        while (iter != classMap.end()) {
            if (!iter.key().contains("::")) {
                QChar c = iter.key()[0];
                if ((c >= 'A') && (c <= 'Z')) {
                    int idx = c.unicode() - QChar('A').unicode();
                    ++count[idx];
                }
            }
            ++iter;
        }
        int highest = 0;
        int idx = -1;
        for (int i=0; i<26; ++i) {
            if (count[i] > highest) {
                highest = count[i];
                idx = i;
            }
        }
        idx += QChar('A').unicode();
        QChar common(idx);
        commonPrefix = common;
        commonPrefixLen = 1;

#if 0
        /*
          The algorithm below eventually failed, so it was replaced
          with the simple (perhaps too simple) algorithm above.

          The caller didn't pass in a common prefix, so get the common
          prefix by looking at the class names of the first and last
          classes in the class map. Discard any namespace names and
          just use the bare class names. For Qt, the prefix is "Q".

          Note that the algorithm used here to derive the common prefix
          from the first and last classes in alphabetical order (QAccel
          and QXtWidget in Qt 2.1), fails if either class name does not
          begin with Q.
        */
        QString first;
        QString last;
        NodeMap::const_iterator iter = classMap.begin();
        while (iter != classMap.end()) {
            if (!iter.key().contains("::")) {
                first = iter.key();
                break;
            }
            ++iter;
        }

        if (first.isEmpty())
            first = classMap.begin().key();

        iter = classMap.end();
        while (iter != classMap.begin()) {
            --iter;
            if (!iter.key().contains("::")) {
                last = iter.key();
                break;
            }
        }

        if (last.isEmpty())
            last = classMap.begin().key();

        if (classMap.size() > 1) {
            while (commonPrefixLen < first.length() + 1 &&
                   commonPrefixLen < last.length() + 1 &&
                   first[commonPrefixLen] == last[commonPrefixLen])
                ++commonPrefixLen;
        }

        commonPrefix = first.left(commonPrefixLen);
#endif
    }

    /*
      Divide the data into 37 paragraphs: 0, ..., 9, A, ..., Z,
      underscore (_). QAccel will fall in paragraph 10 (A) and
      QXtWidget in paragraph 33 (X). This is the only place where we
      assume that NumParagraphs is 37. Each paragraph is a NodeMap.
    */
    NodeMap paragraph[NumParagraphs+1];
    QString paragraphName[NumParagraphs+1];
    QSet<char> usedParagraphNames;

    NodeMap::ConstIterator c = classMap.begin();
    while (c != classMap.end()) {
        QStringList pieces = c.key().split("::");
        QString key;
        int idx = commonPrefixLen;
        if (!pieces.last().startsWith(commonPrefix))
            idx = 0;
        if (pieces.size() == 1)
            key = pieces.last().mid(idx).toLower();
        else
            key = pieces.last().toLower();

        int paragraphNr = NumParagraphs - 1;

        if (key[0].digitValue() != -1) {
            paragraphNr = key[0].digitValue();
        }
        else if (key[0] >= QLatin1Char('a') && key[0] <= QLatin1Char('z')) {
            paragraphNr = 10 + key[0].unicode() - 'a';
        }

        paragraphName[paragraphNr] = key[0].toUpper();
        usedParagraphNames.insert(key[0].toLower().cell());
        paragraph[paragraphNr].insert(key, c.value());
        ++c;
    }

    /*
      Each paragraph j has a size: paragraph[j].count(). In the
      discussion, we will assume paragraphs 0 to 5 will have sizes
      3, 1, 4, 1, 5, 9.

      We now want to compute the paragraph offset. Paragraphs 0 to 6
      start at offsets 0, 3, 4, 8, 9, 14, 23.
    */
    int paragraphOffset[NumParagraphs + 1];     // 37 + 1
    paragraphOffset[0] = 0;
    for (int i=0; i<NumParagraphs; i++)         // i = 0..36
        paragraphOffset[i+1] = paragraphOffset[i] + paragraph[i].count();

    int curParNr = 0;
    int curParOffset = 0;
    QMap<QChar,QString> cmap;

    /*
      Output the alphabet as a row of links.
     */
    if (includeAlphabet) {
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","alphabet");
        for (int i = 0; i < 26; i++) {
            QChar ch('a' + i);
            if (usedParagraphNames.contains(char('a' + i))) {
                writeStartTag(DT_xref);
                // formathtml
                QString guid = lookupGuid(outFileName(),QString(ch));
                QString attr = outFileName() + QString("#%1").arg(guid);
                xmlWriter().writeAttribute("href", attr);
                xmlWriter().writeCharacters(QString(ch.toUpper()));
                writeEndTag(); // </xref>
            }
        }
        writeEndTag(); // </p>
    }

    /*
      Output a <p> element to contain all the <dl> elements.
     */
    writeStartTag(DT_p);
    xmlWriter().writeAttribute("outputclass","compactlist");

    for (int i=0; i<classMap.count()-1; i++) {
        while ((curParNr < NumParagraphs) &&
               (curParOffset == paragraph[curParNr].count())) {
            ++curParNr;
            curParOffset = 0;
        }

        /*
          Starting a new paragraph means starting a new <dl>.
        */
        if (curParOffset == 0) {
            if (i > 0) {
                writeEndTag(); // </dlentry>
                writeEndTag(); // </dl>
            }
            writeStartTag(DT_dl);
            writeStartTag(DT_dlentry);
            writeStartTag(DT_dt);
            if (includeAlphabet) {
                QChar c = paragraphName[curParNr][0].toLower();
                writeGuidAttribute(QString(c));
            }
            xmlWriter().writeAttribute("outputclass","sublist-header");
            xmlWriter().writeCharacters(paragraphName[curParNr]);
            writeEndTag(); // </dt>
        }

        /*
          Output a <dd> for the current offset in the current paragraph.
         */
        writeStartTag(DT_dd);
        if ((curParNr < NumParagraphs) &&
                !paragraphName[curParNr].isEmpty()) {
            NodeMap::Iterator it;
            it = paragraph[curParNr].begin();
            for (int i=0; i<curParOffset; i++)
                ++it;

            /*
              Previously, we used generateFullName() for this, but we
              require some special formatting.
            */
            writeStartTag(DT_xref);
            // formathtml
            writeHrefAttribute(linkForNode(it.value(), relative));

            QStringList pieces;
            if (it.value()->subType() == Node::QmlClass)
                pieces << it.value()->name();
            else
                pieces = fullName(it.value(), relative, marker).split("::");
            xmlWriter().writeCharacters(protectEnc(pieces.last()));
            writeEndTag(); // </xref>
            if (pieces.size() > 1) {
                xmlWriter().writeCharacters(" (");
                generateFullName(it.value()->parent(),relative,marker);
                xmlWriter().writeCharacters(")");
            }
        }
        writeEndTag(); // </dd>
        curParOffset++;
    }
    writeEndTag(); // </dlentry>
    writeEndTag(); // </dl>
    writeEndTag(); // </p>
}

/*!
  Write XML for a function index to the current XML stream.
 */
void DitaXmlGenerator::generateFunctionIndex(const Node* relative,
                                             CodeMarker* marker)
{
    writeStartTag(DT_p);
    xmlWriter().writeAttribute("outputclass","alphabet");
    for (int i = 0; i < 26; i++) {
        QChar ch('a' + i);
        writeStartTag(DT_xref);
        // formathtml
        QString guid = lookupGuid(outFileName(),QString(ch));
        QString attr = outFileName() + QString("#%1").arg(guid);
        xmlWriter().writeAttribute("href", attr);
        xmlWriter().writeCharacters(QString(ch.toUpper()));
        writeEndTag(); // </xref>

    }
    writeEndTag(); // </p>

    char nextLetter = 'a';
    char currentLetter;

    writeStartTag(DT_ul);
    QMap<QString, NodeMap >::ConstIterator f = funcIndex.begin();
    while (f != funcIndex.end()) {
        writeStartTag(DT_li);
        currentLetter = f.key()[0].unicode();
        while (islower(currentLetter) && currentLetter >= nextLetter) {
            writeStartTag(DT_p);
            writeGuidAttribute(QString(nextLetter));
            xmlWriter().writeAttribute("outputclass","target");
            xmlWriter().writeCharacters(QString(nextLetter));
            writeEndTag(); // </p>
            nextLetter++;
        }
        xmlWriter().writeCharacters(protectEnc(f.key()));
        xmlWriter().writeCharacters(":");

        NodeMap::ConstIterator s = (*f).begin();
        while (s != (*f).end()) {
            generateFullName((*s)->parent(), relative, marker, *s);
            ++s;
        }
        writeEndTag(); // </li>
        ++f;
    }
    writeEndTag(); // </ul>
}

/*!
  Write the legalese texts as XML to the current XML stream.
 */
void DitaXmlGenerator::generateLegaleseList(const Node* relative,
                                            CodeMarker* marker)
{
    QMap<Text, const Node*>::ConstIterator it = legaleseTexts.begin();
    while (it != legaleseTexts.end()) {
        Text text = it.key();
        generateText(text, relative, marker);
        writeStartTag(DT_ul);
        do {
            writeStartTag(DT_li);
            generateFullName(it.value(), relative, marker);
            writeEndTag(); // </li>
            ++it;
        } while (it != legaleseTexts.end() && it.key() == text);
        writeEndTag(); //</ul>
    }
}

/*!
  Generate the text for the QML item described by \a node
  and write it to the current XML stream.
 */
void DitaXmlGenerator::generateQmlItem(const Node* node,
                                       const Node* relative,
                                       CodeMarker* marker,
                                       bool summary)
{
    QString marked = marker->markedUpQmlItem(node,summary);
    QRegExp tag("(<[^@>]*>)");
    if (marked.indexOf(tag) != -1) {
        QString tmp = protectEnc(marked.mid(tag.pos(1), tag.cap(1).length()));
        marked.replace(tag.pos(1), tag.cap(1).length(), tmp);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i>\\1<sub>\\2</sub></i>");
#if 0
    marked.replace("<@param>", "<i>");
    marked.replace("</@param>", "</i>");

    marked.replace("<@extra>", "<tt>");
    marked.replace("</@extra>", "</tt>");
#endif
    if (summary) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }
    writeText(marked, marker, relative);
}

/*!
  Write the XML for the overview list to the current XML stream.
 */
void DitaXmlGenerator::generateOverviewList(const Node* relative, CodeMarker* /* marker */)
{
    QMap<const FakeNode*, QMap<QString, FakeNode*> > fakeNodeMap;
    QMap<QString, const FakeNode*> groupTitlesMap;
    QMap<QString, FakeNode*> uncategorizedNodeMap;
    QRegExp singleDigit("\\b([0-9])\\b");

    const NodeList children = tree_->root()->childNodes();
    foreach (Node* child, children) {
        if (child->type() == Node::Fake && child != relative) {
            FakeNode* fakeNode = static_cast<FakeNode*>(child);

            // Check whether the page is part of a group or is the group
            // definition page.
            QString group;
            bool isGroupPage = false;
            if (fakeNode->doc().metaCommandsUsed().contains("group")) {
                group = fakeNode->doc().metaCommandArgs("group")[0];
                isGroupPage = true;
            }

            // there are too many examples; they would clutter the list
            if (fakeNode->subType() == Node::Example)
                continue;

            // not interested either in individual (Qt Designer etc.) manual chapters
            if (fakeNode->links().contains(Node::ContentsLink))
                continue;

            // Discard external nodes.
            if (fakeNode->subType() == Node::ExternalPage)
                continue;

            QString sortKey = fakeNode->fullTitle().toLower();
            if (sortKey.startsWith("the "))
                sortKey.remove(0, 4);
            sortKey.replace(singleDigit, "0\\1");

            if (!group.isEmpty()) {
                if (isGroupPage) {
                    // If we encounter a group definition page, we add all
                    // the pages in that group to the list for that group.
                    foreach (Node* member, fakeNode->groupMembers()) {
                        if (member->type() != Node::Fake)
                            continue;
                        FakeNode* page = static_cast<FakeNode*>(member);
                        if (page) {
                            QString sortKey = page->fullTitle().toLower();
                            if (sortKey.startsWith("the "))
                                sortKey.remove(0, 4);
                            sortKey.replace(singleDigit, "0\\1");
                            fakeNodeMap[const_cast<const FakeNode*>(fakeNode)].insert(sortKey, page);
                            groupTitlesMap[fakeNode->fullTitle()] = const_cast<const FakeNode*>(fakeNode);
                        }
                    }
                }
                else if (!isGroupPage) {
                    // If we encounter a page that belongs to a group then
                    // we add that page to the list for that group.
                    const FakeNode* groupNode =
                            static_cast<const FakeNode*>(tree_->root()->findNode(group, Node::Fake));
                    if (groupNode)
                        fakeNodeMap[groupNode].insert(sortKey, fakeNode);
                    //else
                    //    uncategorizedNodeMap.insert(sortKey, fakeNode);
                }// else
                //    uncategorizedNodeMap.insert(sortKey, fakeNode);
            }// else
            //    uncategorizedNodeMap.insert(sortKey, fakeNode);
        }
    }

    // We now list all the pages found that belong to groups.
    // If only certain pages were found for a group, but the definition page
    // for that group wasn't listed, the list of pages will be intentionally
    // incomplete. However, if the group definition page was listed, all the
    // pages in that group are listed for completeness.

    if (!fakeNodeMap.isEmpty()) {
        foreach (const QString& groupTitle, groupTitlesMap.keys()) {
            const FakeNode* groupNode = groupTitlesMap[groupTitle];
            writeStartTag(DT_p);
            xmlWriter().writeAttribute("outputclass","h3");
            writeStartTag(DT_xref);
            // formathtml
            xmlWriter().writeAttribute("href",linkForNode(groupNode, relative));
            writeCharacters(protectEnc(groupNode->fullTitle()));
            writeEndTag(); // </xref>
            writeEndTag(); // </p>
            if (fakeNodeMap[groupNode].count() == 0)
                continue;

            writeStartTag(DT_ul);
            foreach (const FakeNode* fakeNode, fakeNodeMap[groupNode]) {
                QString title = fakeNode->fullTitle();
                if (title.startsWith("The "))
                    title.remove(0, 4);
                writeStartTag(DT_li);
                writeStartTag(DT_xref);
                // formathtml
                xmlWriter().writeAttribute("href",linkForNode(fakeNode, relative));
                writeCharacters(protectEnc(title));
                writeEndTag(); // </xref>
                writeEndTag(); // </li>
            }
            writeEndTag(); // </ul>
        }
    }

    if (!uncategorizedNodeMap.isEmpty()) {
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","h3");
        xmlWriter().writeCharacters("Miscellaneous");
        writeEndTag(); // </p>
        writeStartTag(DT_ul);
        foreach (const FakeNode *fakeNode, uncategorizedNodeMap) {
            QString title = fakeNode->fullTitle();
            if (title.startsWith("The "))
                title.remove(0, 4);
            writeStartTag(DT_li);
            writeStartTag(DT_xref);
            // formathtml
            xmlWriter().writeAttribute("href",linkForNode(fakeNode, relative));
            writeCharacters(protectEnc(title));
            writeEndTag(); // </xref>
            writeEndTag(); // </li>
        }
        writeEndTag(); // </ul>
    }
}

/*!
  Write the XML for a standard section of a page, e.g.
  "Public Functions" or "Protected Slots." The section
  is written too the current XML stream as a table.
 */
void DitaXmlGenerator::generateSection(const NodeList& nl,
                                       const Node* relative,
                                       CodeMarker* marker,
                                       CodeMarker::SynopsisStyle style)
{
    if (!nl.isEmpty()) {
        writeStartTag(DT_ul);
        NodeList::ConstIterator m = nl.begin();
        while (m != nl.end()) {
            if ((*m)->access() != Node::Private) {
                writeStartTag(DT_li);
                QString marked = getMarkedUpSynopsis(*m, relative, marker, style);
                writeText(marked, marker, relative);
                writeEndTag(); // </li>
            }
            ++m;
        }
        writeEndTag(); // </ul>
    }
}

/*!
  Writes the "inherited from" list to the current XML stream.
 */
void DitaXmlGenerator::generateSectionInheritedList(const Section& section,
                                                    const Node* relative,
                                                    CodeMarker* marker)
{
    if (section.inherited.isEmpty())
        return;
    writeStartTag(DT_ul);
    QList<QPair<InnerNode*,int> >::ConstIterator p = section.inherited.begin();
    while (p != section.inherited.end()) {
        writeStartTag(DT_li);
        QString text;
        text.setNum((*p).second);
        text += QLatin1Char(' ');
        if ((*p).second == 1)
            text += section.singularMember;
        else
            text += section.pluralMember;
        text += " inherited from ";
        writeCharacters(text);
        writeStartTag(DT_xref);
        // formathtml
        // zzz
        text = fileName((*p).first) + QLatin1Char('#');
        text += DitaXmlGenerator::cleanRef(section.name.toLower());
        xmlWriter().writeAttribute("href",text);
        text = protectEnc(marker->plainFullName((*p).first, relative));
        writeCharacters(text);
        writeEndTag(); // </xref>
        writeEndTag(); // </li>
        ++p;
    }
    writeEndTag(); // </ul>
}

/*!
  Get the synopsis from the \a node using the \a relative
  node if needed, and mark up the synopsis using \a marker.
  Use the style to decide which kind of sysnopsis to build,
  normally \c Summary or \c Detailed. Return the marked up
  string.
 */
QString DitaXmlGenerator::getMarkedUpSynopsis(const Node* node,
                                              const Node* relative,
                                              CodeMarker* marker,
                                              CodeMarker::SynopsisStyle style)
{
    QString marked = marker->markedUpSynopsis(node, relative, style);
    QRegExp tag("(<[^@>]*>)");
    if (marked.indexOf(tag) != -1) {
        QString tmp = protectEnc(marked.mid(tag.pos(1), tag.cap(1).length()));
        marked.replace(tag.pos(1), tag.cap(1).length(), tmp);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i> \\1<sub>\\2</sub></i>");
#if 0
    marked.replace("<@param>","<i>");
    marked.replace("</@param>","</i>");
#endif
    if (style == CodeMarker::Summary) {
        marked.remove("<@name>");   // was "<b>"
        marked.remove("</@name>");  // was "</b>"
    }

    if (style == CodeMarker::Subpage) {
        QRegExp extraRegExp("<@extra>.*</@extra>");
        extraRegExp.setMinimal(true);
        marked.remove(extraRegExp);
    }
#if 0
    else {
        marked.replace("<@extra>","<tt>");
        marked.replace("</@extra>","</tt>");
    }
#endif

    if (style != CodeMarker::Detailed) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }
    return marked;
}

/*!
  Renamed from highlightedCode() in the html generator. Gets the text
  from \a markedCode , and then the text is written to the current XML
  stream.
 */
void DitaXmlGenerator::writeText(const QString& markedCode,
                                 CodeMarker* marker,
                                 const Node* relative)
{
    QString src = markedCode;
    QString text;
    QStringRef arg;
    QStringRef par1;

    const QChar charLangle = '<';
    const QChar charAt = '@';

    /*
      First strip out all the extraneous markup. The table
      below contains the markup we want to keep. Everything
      else that begins with "<@" or "</@" is stripped out.
     */
    static const QString spanTags[] = {
        "<@link ",         "<@link ",
        "<@type>",         "<@type>",
        "<@headerfile>",   "<@headerfile>",
        "<@func>",         "<@func>",
        "<@func ",         "<@func ",
        "<@param>",        "<@param>",
        "<@extra>",        "<@extra>",
        "</@link>",        "</@link>",
        "</@type>",        "</@type>",
        "</@headerfile>",  "</@headerfile>",
        "</@func>",        "</@func>",
        "</@param>",        "</@param>",
        "</@extra>",        "</@extra>"
    };
    for (int i = 0, n = src.size(); i < n;) {
        if (src.at(i) == charLangle) {
            bool handled = false;
            for (int k = 0; k != 13; ++k) {
                const QString & tag = spanTags[2 * k];
                if (tag == QStringRef(&src, i, tag.length())) {
                    text += spanTags[2 * k + 1];
                    i += tag.length();
                    handled = true;
                    break;
                }
            }
            if (!handled) {
                ++i;
                if (src.at(i) == charAt ||
                        (src.at(i) == QLatin1Char('/') && src.at(i + 1) == charAt)) {
                    // drop 'our' unknown tags (the ones still containing '@')
                    while (i < n && src.at(i) != QLatin1Char('>'))
                        ++i;
                    ++i;
                }
                else {
                    // retain all others
                    text += charLangle;
                }
            }
        }
        else {
            text += src.at(i);
            ++i;
        }
    }

    // replace all <@link> tags: "(<@link node=\"([^\"]+)\">).*(</@link>)"
    // replace all "(<@(type|headerfile|func)(?: +[^>]*)?>)(.*)(</@\\2>)" tags
    src = text;
    text = QString();
    static const QString markTags[] = {
        // 0       1         2           3       4        5
        "link", "type", "headerfile", "func", "param", "extra"
    };

    for (int i = 0, n = src.size(); i < n;) {
        if (src.at(i) == charLangle && src.at(i + 1) == charAt) {
            i += 2;
            for (int k = 0; k != 6; ++k) {
                if (parseArg(src, markTags[k], &i, n, &arg, &par1)) {
                    const Node* n = 0;
                    if (k == 0) { // <@link>
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        n = CodeMarker::nodeForString(par1.toString());
                        QString link = linkForNode(n, relative);
                        addLink(link, arg);
                    }
                    else if (k == 4) { // <@param>
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        writeStartTag(DT_i);
                        //writeCharacters(" " + arg.toString());
                        writeCharacters(arg.toString());
                        writeEndTag(); // </i>
                    }
                    else if (k == 5) { // <@extra>
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        writeStartTag(DT_tt);
                        writeCharacters(arg.toString());
                        writeEndTag(); // </tt>
                    }
                    else {
                        if (!text.isEmpty()) {
                            writeCharacters(text);
                            text.clear();
                        }
                        par1 = QStringRef();
                        QString link;
                        n = marker->resolveTarget(arg.toString(), tree_, relative);
                        if (n && n->subType() == Node::QmlBasicType) {
                            if (relative && relative->subType() == Node::QmlClass) {
                                link = linkForNode(n,relative);
                                addLink(link, arg);
                            }
                            else {
                                writeCharacters(arg.toString());
                            }
                        }
                        else {
                            // (zzz) Is this correct for all cases?
                            link = linkForNode(n,relative);
                            addLink(link, arg);
                        }
                    }
                    break;
                }
            }
        }
        else {
            text += src.at(i++);
        }
    }
    if (!text.isEmpty()) {
        writeCharacters(text);
    }
}

void DitaXmlGenerator::generateLink(const Atom* atom,
                                    const Node* /* relative */,
                                    CodeMarker* marker)
{
    static QRegExp camelCase("[A-Z][A-Z][a-z]|[a-z][A-Z0-9]|_");

    if (funcLeftParen.indexIn(atom->string()) != -1 && marker->recognizeLanguage("Cpp")) {
        // hack for C++: move () outside of link
        int k = funcLeftParen.pos(1);
        writeCharacters(protectEnc(atom->string().left(k)));
        if (link.isEmpty()) {
            if (showBrokenLinks)
                writeEndTag(); // </i>
        }
        else
            writeEndTag(); // </xref>
        inLink = false;
        writeCharacters(protectEnc(atom->string().mid(k)));
    }
    else if (marker->recognizeLanguage("Java")) {
        // hack for Java: remove () and use <tt> when appropriate
        bool func = atom->string().endsWith("()");
        bool tt = (func || atom->string().contains(camelCase));
        if (tt)
            writeStartTag(DT_tt);
        if (func)
            writeCharacters(protectEnc(atom->string().left(atom->string().length() - 2)));
        else
            writeCharacters(protectEnc(atom->string()));
        writeEndTag(); // </tt>
    }
    else
        writeCharacters(protectEnc(atom->string()));
}

QString DitaXmlGenerator::cleanRef(const QString& ref)
{
    QString clean;

    if (ref.isEmpty())
        return clean;

    clean.reserve(ref.size() + 20);
    const QChar c = ref[0];
    const uint u = c.unicode();

    if ((u >= 'a' && u <= 'z') ||
            (u >= 'A' && u <= 'Z') ||
            (u >= '0' && u <= '9')) {
        clean += c;
    }
    else if (u == '~') {
        clean += "dtor.";
    }
    else if (u == '_') {
        clean += "underscore.";
    }
    else {
        clean += QLatin1Char('A');
    }

    for (int i = 1; i < (int) ref.length(); i++) {
        const QChar c = ref[i];
        const uint u = c.unicode();
        if ((u >= 'a' && u <= 'z') ||
                (u >= 'A' && u <= 'Z') ||
                (u >= '0' && u <= '9') || u == '-' ||
                u == '_' || u == ':' || u == '.') {
            clean += c;
        }
        else if (c.isSpace()) {
            clean += QLatin1Char('-');
        }
        else if (u == '!') {
            clean += "-not";
        }
        else if (u == '&') {
            clean += "-and";
        }
        else if (u == '<') {
            clean += "-lt";
        }
        else if (u == '=') {
            clean += "-eq";
        }
        else if (u == '>') {
            clean += "-gt";
        }
        else if (u == '#') {
            clean += QLatin1Char('#');
        }
        else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

QString DitaXmlGenerator::registerRef(const QString& ref)
{
    QString clean = DitaXmlGenerator::cleanRef(ref);

    for (;;) {
        QString& prevRef = refMap[clean.toLower()];
        if (prevRef.isEmpty()) {
            prevRef = ref;
            break;
        }
        else if (prevRef == ref)
            break;
        clean += QLatin1Char('x');
    }
    return clean;
}

/*!
  Calls protect() with the \a string. Returns the result.
 */
QString DitaXmlGenerator::protectEnc(const QString& string)
{
    return protect(string, outputEncoding);
}

QString DitaXmlGenerator::protect(const QString& string, const QString& ) //outputEncoding)
{
#define APPEND(x) \
    if (xml.isEmpty()) { \
    xml = string; \
    xml.truncate(i); \
} \
    xml += (x);

    QString xml;
    int n = string.length();

    for (int i = 0; i < n; ++i) {
        QChar ch = string.at(i);

        if (ch == QLatin1Char('&')) {
            APPEND("&amp;");
        }
        else if (ch == QLatin1Char('<')) {
            APPEND("&lt;");
        }
        else if (ch == QLatin1Char('>')) {
            APPEND("&gt;");
        }
        else if (ch == QLatin1Char('"')) {
            APPEND("&quot;");
        }
#if 0
        else if ((outputEncoding == "ISO-8859-1" && ch.unicode() > 0x007F) ||
                 (ch == QLatin1Char('*') && i + 1 < n && string.at(i) == QLatin1Char('/')) ||
                 (ch == QLatin1Char('.') && i > 2 && string.at(i - 2) == QLatin1Char('.'))) {
            // we escape '*/' and the last dot in 'e.g.' and 'i.e.' for the Javadoc generator
            APPEND("&#x");
            xml += QString::number(ch.unicode(), 16);
            xml += QLatin1Char(';');
        }
#endif
        else {
            if (!xml.isEmpty())
                xml += ch;
        }
    }

    if (!xml.isEmpty())
        return xml;
    return string;

#undef APPEND
}

/*!
  Constructs a file name appropriate for the \a node
  and returns the file name.
 */
QString DitaXmlGenerator::fileBase(const Node* node) const
{
    QString result;
    result = Generator::fileBase(node);
#if 0
    if (!node->isInnerNode()) {
        switch (node->status()) {
        case Node::Compat:
            result += "-qt3";
            break;
        case Node::Obsolete:
            result += "-obsolete";
            break;
        default:
            ;
        }
    }
#endif
    return result;
}

QString DitaXmlGenerator::guidForNode(const Node* node)
{
    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
    default:
        break;
    case Node::Enum:
        return node->guid();
    case Node::Typedef:
    {
        const TypedefNode* tdn = static_cast<const TypedefNode*>(node);
        if (tdn->associatedEnum())
            return guidForNode(tdn->associatedEnum());
    }
        return node->guid();
    case Node::Function:
    {
        const FunctionNode* fn = static_cast<const FunctionNode*>(node);
        if (fn->associatedProperty()) {
            return guidForNode(fn->associatedProperty());
        }
        else {
            QString ref = fn->name();
            if (fn->overloadNumber() != 1) {
                ref += QLatin1Char('-') + QString::number(fn->overloadNumber());
            }
        }
        return fn->guid();
    }
    case Node::Fake:
        if (node->subType() != Node::QmlPropertyGroup)
            break;
    case Node::QmlProperty:
    case Node::Property:
        return node->guid();
    case Node::QmlSignal:
        return node->guid();
    case Node::QmlSignalHandler:
        return node->guid();
    case Node::QmlMethod:
        return node->guid();
    case Node::Variable:
        return node->guid();
    case Node::Target:
        return node->guid();
    }
    return QString();
}

/*!
  Constructs a file name appropriate for the \a node and returns
  it. If the \a node is not a fake node, or if it is a fake node but
  it is neither an external page node nor an image node or a ditamap,
  call the PageGenerator::fileName() function.
 */
QString DitaXmlGenerator::fileName(const Node* node)
{
    if (node->type() == Node::Fake) {
        if (static_cast<const FakeNode*>(node)->pageType() == Node::DitaMapPage)
            return node->name();
        if (static_cast<const FakeNode*>(node)->subType() == Node::ExternalPage)
            return node->name();
        if (static_cast<const FakeNode*>(node)->subType() == Node::Image)
            return node->name();
    }
    return Generator::fileName(node);
}

QString DitaXmlGenerator::linkForNode(const Node* node, const Node* relative)
{
    if (node == 0 || node == relative)
        return QString();
    if (!node->url().isEmpty())
        return node->url();
    if (fileBase(node).isEmpty())
        return QString();
    if (node->access() == Node::Private)
        return QString();

    QString fn = fileName(node);
    if (node && relative && node->parent() != relative) {
        if (node->parent()->subType() == Node::QmlClass && relative->subType() == Node::QmlClass) {
            if (node->parent()->isAbstract()) {
                /*
                  This is a bit of a hack. What we discover with
                  the three 'if' statements immediately above,
                  is that node's parent is marked \qmlabstract
                  but the link appears in a qdoc comment for a
                  subclass of the node's parent. This means the
                  link should refer to the file for the relative
                  node, not the file for node.
                 */
                fn = fileName(relative);
#if DEBUG_ABSTRACT
                qDebug() << "ABSTRACT:" << node->parent()->name()
                         << node->name() << relative->name()
                         << node->parent()->type() << node->parent()->subType()
                         << relative->type() << relative->subType() << outFileName();
#endif
            }
        }
    }
    QString link = fn;

    if (!node->isInnerNode() || node->subType() == Node::QmlPropertyGroup) {
        QString guid = guidForNode(node);
        if (relative && fn == fileName(relative) && guid == guidForNode(relative)) {
            return QString();
        }
        link += QLatin1Char('#');
        link += guid;
    }
    /*
      If the output is going to subdirectories, then if the
      two nodes will be output to different directories, then
      the link must go up to the parent directory and then
      back down into the other subdirectory.
     */
    if (node && relative && (node != relative)) {
        if (node->outputSubdirectory() != relative->outputSubdirectory())
            link.prepend(QString("../" + node->outputSubdirectory() + QLatin1Char('/')));
    }
    return link;
}

QString DitaXmlGenerator::refForAtom(Atom* atom, const Node* /* node */)
{
    if (atom->type() == Atom::SectionLeft)
        return Doc::canonicalTitle(Text::sectionHeading(atom).toString());
    if (atom->type() == Atom::Target)
        return Doc::canonicalTitle(atom->string());
    return QString();
}

void DitaXmlGenerator::generateFullName(const Node* apparentNode,
                                        const Node* relative,
                                        CodeMarker* marker,
                                        const Node* actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    writeStartTag(DT_xref);
    // formathtml
    QString href = linkForNode(actualNode, relative);
    writeHrefAttribute(href);
    writeCharacters(protectEnc(fullName(apparentNode, relative, marker)));
    writeEndTag(); // </xref>
}

void DitaXmlGenerator::findAllClasses(const InnerNode* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private && (*c)->url().isEmpty()) {
            if ((*c)->type() == Node::Class && !(*c)->doc().isEmpty()) {
                QString className = (*c)->name();
                if ((*c)->parent() &&
                        (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                    className = (*c)->parent()->name()+"::"+className;

                if (!(static_cast<const ClassNode *>(*c))->hideFromMainList()) {
                    if ((*c)->status() == Node::Compat) {
                        compatClasses.insert(className, *c);
                    }
                    else if ((*c)->status() == Node::Obsolete) {
                        obsoleteClasses.insert(className, *c);
                    }
                    else {
                        nonCompatClasses.insert(className, *c);
                        if ((*c)->status() == Node::Main)
                            mainClasses.insert(className, *c);
                    }
                }

                QString moduleName = (*c)->moduleName();
                if (moduleName == "Qt3SupportLight") {
                    moduleClassMap[moduleName].insert((*c)->name(), *c);
                    moduleName = "Qt3Support";
                }
                if (!moduleName.isEmpty())
                    moduleClassMap[moduleName].insert((*c)->name(), *c);

                QString serviceName =
                        (static_cast<const ClassNode *>(*c))->serviceName();
                if (!serviceName.isEmpty())
                    serviceClasses.insert(serviceName, *c);
            }
            else if ((*c)->type() == Node::Fake &&
                     (*c)->subType() == Node::QmlClass &&
                     !(*c)->doc().isEmpty()) {
                QString qmlClassName = (*c)->name();
                qmlClasses.insert(qmlClassName,*c);
            }
            else if ((*c)->isInnerNode()) {
                findAllClasses(static_cast<InnerNode *>(*c));
            }
        }
        ++c;
    }
}

void DitaXmlGenerator::findAllFunctions(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().begin();
    while (c != node->childNodes().end()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode() && (*c)->url().isEmpty()) {
                findAllFunctions(static_cast<const InnerNode*>(*c));
            }
            else if ((*c)->type() == Node::Function) {
                const FunctionNode* func = static_cast<const FunctionNode*>(*c);
                if ((func->status() > Node::Obsolete) &&
                        !func->isInternal() &&
                        (func->metaness() != FunctionNode::Ctor) &&
                        (func->metaness() != FunctionNode::Dtor)) {
                    funcIndex[(*c)->name()].insert((*c)->parent()->fullDocumentName(), *c);
                }
            }
        }
        ++c;
    }
}

void DitaXmlGenerator::findAllLegaleseTexts(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().begin();
    while (c != node->childNodes().end()) {
        if ((*c)->access() != Node::Private) {
            if (!(*c)->doc().legaleseText().isEmpty())
                legaleseTexts.insertMulti((*c)->doc().legaleseText(), *c);
            if ((*c)->isInnerNode())
                findAllLegaleseTexts(static_cast<const InnerNode *>(*c));
        }
        ++c;
    }
}

void DitaXmlGenerator::findAllNamespaces(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().begin();
    while (c != node->childNodes().end()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode() && (*c)->url().isEmpty()) {
                findAllNamespaces(static_cast<const InnerNode *>(*c));
                if ((*c)->type() == Node::Namespace) {
                    const NamespaceNode *nspace = static_cast<const NamespaceNode *>(*c);
                    // Ensure that the namespace's name is not empty (the root
                    // namespace has no name).
                    if (!nspace->name().isEmpty()) {
                        namespaceIndex.insert(nspace->name(), *c);
                        QString moduleName = (*c)->moduleName();
                        if (moduleName == "Qt3SupportLight") {
                            moduleNamespaceMap[moduleName].insert((*c)->name(), *c);
                            moduleName = "Qt3Support";
                        }
                        if (!moduleName.isEmpty())
                            moduleNamespaceMap[moduleName].insert((*c)->name(), *c);
                    }
                }
            }
        }
        ++c;
    }
}

/*!
  We're writing an attribute that indicates that the text
  data is a heading, hence, h1, h2, h3... etc, and we must
  decide which number to use.
 */
int DitaXmlGenerator::hOffset(const Node* node)
{
    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
        return 2;
    case Node::Fake:
        return 1;
    case Node::Enum:
    case Node::Typedef:
    case Node::Function:
    case Node::Property:
    default:
        return 3;
    }
}

bool DitaXmlGenerator::isThreeColumnEnumValueTable(const Atom* atom)
{
    while (atom != 0 && !(atom->type() == Atom::ListRight && atom->string() == ATOM_LIST_VALUE)) {
        if (atom->type() == Atom::ListItemLeft && !matchAhead(atom, Atom::ListItemRight))
            return true;
        atom = atom->next();
    }
    return false;
}

const Node* DitaXmlGenerator::findNodeForTarget(const QString& target,
                                                const Node* relative,
                                                CodeMarker* marker,
                                                const Atom* atom)
{
    const Node* node = 0;

    if (target.isEmpty()) {
        node = relative;
    }
    else if (target.endsWith(".html")) {
        node = tree_->root()->findNode(target, Node::Fake);
    }
    else if (marker) {
        node = marker->resolveTarget(target, tree_, relative);
        if (!node)
            node = tree_->findFakeNodeByTitle(target, relative);
        if (!node && atom) {
            node = tree_->findUnambiguousTarget(target, *const_cast<Atom**>(&atom), relative);
        }
    }

    if (!node)
        relative->doc().location().warning(tr("Cannot link to '%1'").arg(target));

    return node;
}

const QPair<QString,QString> DitaXmlGenerator::anchorForNode(const Node* node)
{
    QPair<QString,QString> anchorPair;
    anchorPair.first = Generator::fileName(node);
    if (node->type() == Node::Fake) {
        const FakeNode *fakeNode = static_cast<const FakeNode*>(node);
        anchorPair.second = fakeNode->title();
    }

    return anchorPair;
}

QString DitaXmlGenerator::getLink(const Atom* atom,
                                  const Node* relative,
                                  CodeMarker* marker,
                                  const Node** node)
{
    QString link;
    *node = 0;
    inObsoleteLink = false;

    if (atom->string().contains(QLatin1Char(':')) &&
            (atom->string().startsWith("file:")
             || atom->string().startsWith("http:")
             || atom->string().startsWith("https:")
             || atom->string().startsWith("ftp:")
             || atom->string().startsWith("mailto:"))) {

        link = atom->string();
    }
    else {
        QStringList path;
        if (atom->string().contains('#'))
            path = atom->string().split('#');
        else
            path.append(atom->string());

        Atom* targetAtom = 0;
        QString first = path.first().trimmed();

        if (first.isEmpty()) {
            *node = relative;
        }
        else if (first.endsWith(".html")) {
            *node = tree_->root()->findNode(first, Node::Fake);
        }
        else {
            *node = marker->resolveTarget(first, tree_, relative);
            if (!*node) {
                *node = tree_->findFakeNodeByTitle(first, relative);
            }
            if (!*node) {
                *node = tree_->findUnambiguousTarget(first, targetAtom, relative);
            }
        }

        if (*node) {
            if (!(*node)->url().isEmpty()) {
                return (*node)->url();
            }
            else {
                path.removeFirst();
            }
        }
        else {
            *node = relative;
        }

        if (*node && (*node)->status() == Node::Obsolete) {
            if (relative && (relative->parent() != *node) &&
                    (relative->status() != Node::Obsolete)) {
                bool porting = false;
                if (relative->type() == Node::Fake) {
                    const FakeNode* fake = static_cast<const FakeNode*>(relative);
                    if (fake->title().startsWith("Porting"))
                        porting = true;
                }
                QString name = marker->plainFullName(relative);
                if (!porting && !name.startsWith("Q3")) {
                    if (obsoleteLinks) {
                        relative->doc().location().warning(tr("Link to obsolete item '%1' in %2")
                                                           .arg(atom->string())
                                                           .arg(name));
                    }
                    inObsoleteLink = true;
                }
            }
        }

        while (!path.isEmpty()) {
            targetAtom = tree_->findTarget(path.first(), *node);
            if (targetAtom == 0)
                break;
            path.removeFirst();
        }

        if (path.isEmpty()) {
            link = linkForNode(*node, relative);
            if (*node && (*node)->subType() == Node::Image)
                link = "images/used-in-examples/" + link;
            if (targetAtom) {
                if (link.isEmpty())
                    link = outFileName();
                QString guid = lookupGuid(link,refForAtom(targetAtom,*node));
                link += QLatin1Char('#') + guid;
            }
            else if (!link.isEmpty() && *node &&
                     (link.endsWith(".xml") || link.endsWith(".dita"))) {
                link += QLatin1Char('#') + (*node)->guid();
            }
        }
        /*
          If the output is going to subdirectories, then if the
          two nodes will be output to different directories, then
          the link must go up to the parent directory and then
          back down into the other subdirectory.
        */
        if (link.startsWith("images/")) {
            link.prepend(QString("../"));
        }
        else if (*node && relative && (*node != relative)) {
            if ((*node)->outputSubdirectory() != relative->outputSubdirectory()) {
                link.prepend(QString("../" + (*node)->outputSubdirectory() + QLatin1Char('/')));
            }
        }
    }
    if (!link.isEmpty() && link[0] == '#') {
        link.prepend(outFileName());
    }
    return link;
}

/*!
  This function can be called if getLink() returns an empty
  string.
 */
QString DitaXmlGenerator::getDisambiguationLink(const Atom *, CodeMarker *)
{
    qDebug() << "Unimplemented function called: "
             << "QString DitaXmlGenerator::getDisambiguationLink()";
    return QString();
}

void DitaXmlGenerator::generateIndex(const QString& fileBase,
                                     const QString& url,
                                     const QString& title)
{
    tree_->generateIndex(outputDir() + QLatin1Char('/') + fileBase + ".index", url, title);
}

void DitaXmlGenerator::generateStatus(const Node* node, CodeMarker* marker)
{
    Text text;

    switch (node->status()) {
    case Node::Obsolete:
        if (node->isInnerNode())
            Generator::generateStatus(node, marker);
        break;
    case Node::Compat:
        if (node->isInnerNode()) {
            text << Atom::ParaLeft
                 << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
                 << "This "
                 << typeString(node)
                 << " is part of the Qt 3 support library."
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
                 << " It is provided to keep old source code working. "
                 << "We strongly advise against "
                 << "using it in new code. See ";

            const FakeNode *fakeNode = tree_->findFakeNodeByTitle("Porting To Qt 4");
            Atom *targetAtom = 0;
            if (fakeNode && node->type() == Node::Class) {
                QString oldName(node->name());
                oldName.remove(QLatin1Char('3'));
                targetAtom = tree_->findTarget(oldName,fakeNode);
            }

            if (targetAtom) {
                QString fn = fileName(fakeNode);
                QString guid = lookupGuid(fn,refForAtom(targetAtom,fakeNode));
                text << Atom(Atom::GuidLink, fn + QLatin1Char('#') + guid);
            }
            else
                text << Atom(Atom::Link, "Porting to Qt 4");

            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << Atom(Atom::String, "Porting to Qt 4")
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                 << " for more information."
                 << Atom::ParaRight;
        }
        generateText(text, node, marker);
        break;
    default:
        Generator::generateStatus(node, marker);
    }
}

void DitaXmlGenerator::beginLink(const QString& link)
{
    this->link = link;
    if (link.isEmpty())
        return;
    writeStartTag(DT_xref);
    // formathtml
    writeHrefAttribute(link);
    inLink = true;
}

void DitaXmlGenerator::endLink()
{
    if (inLink) {
        if (link.isEmpty()) {
            if (showBrokenLinks)
                writeEndTag(); // </i>
        }
        else {
            if (inObsoleteLink) {
                writeStartTag(DT_sup);
                xmlWriter().writeCharacters("(obsolete)");
                writeEndTag(); // </sup>
            }
            writeEndTag(); // </xref>
        }
    }
    inLink = false;
    inObsoleteLink = false;
}

/*!
  Generates the summary for the \a section. Only used for
  sections of QML element documentation.

  Currently handles only the QML property group.
 */
void DitaXmlGenerator::generateQmlSummary(const Section& section,
                                          const Node* relative,
                                          CodeMarker* marker)
{
    if (!section.members.isEmpty()) {
        writeStartTag(DT_ul);
        NodeList::ConstIterator m;
        m = section.members.begin();
        while (m != section.members.end()) {
            writeStartTag(DT_li);
            generateQmlItem(*m,relative,marker,true);
            writeEndTag(); // </li>
            ++m;
        }
        writeEndTag(); // </ul>
    }
}

/*!
  Outputs the DITA detailed documentation for a section
  on a QML element reference page.
 */
void DitaXmlGenerator::generateDetailedQmlMember(const Node* node,
                                                 const InnerNode* relative,
                                                 CodeMarker* marker)
{
    QString marked;
    const QmlPropertyNode* qpn = 0;
    if (node->subType() == Node::QmlPropertyGroup) {
        const QmlPropGroupNode* qpgn = static_cast<const QmlPropGroupNode*>(node);
        NodeList::ConstIterator p = qpgn->childNodes().begin();
        writeStartTag(DT_ul);
        while (p != qpgn->childNodes().end()) {
            if ((*p)->type() == Node::QmlProperty) {
                qpn = static_cast<const QmlPropertyNode*>(*p);
                writeStartTag(DT_li);
                writeGuidAttribute((Node*)qpn);
                QString attr;
                int ro = qpn->getReadOnly();
                if (ro < 0) {
                    if (!qpn->isWritable(tree_))
                        attr = "read-only";
                }
                else if (ro > 0)
                    attr = "read-only";
                if (qpgn->isDefault()) {
                    if (!attr.isEmpty())
                        attr += QLatin1Char(' ');
                    attr += "default";
                }
                if (!attr.isEmpty())
                    xmlWriter().writeAttribute("outputclass",attr);
                generateQmlItem(qpn, relative, marker, false);
                writeEndTag(); // </li>
            }
            ++p;
        }
        writeEndTag(); // </ul>
    }
    else if (node->type() == Node::QmlProperty) {
        qpn = static_cast<const QmlPropertyNode*>(node);
        /*
          If the QML property node has a single subproperty,
          override, replace qpn with that override node and
          proceed as normal.
         */
        if (qpn->qmlPropNodes().size() == 1) {
            Node* n = qpn->qmlPropNodes().at(0);
            if (n->type() == Node::QmlProperty)
                qpn = static_cast<const QmlPropertyNode*>(n);
        }
        /*
          Now qpn either has no overrides, or it has more
          than 1. If it has none, proceed to output as nortmal.
         */
        if (qpn->qmlPropNodes().isEmpty()) {
            writeStartTag(DT_ul);
            writeStartTag(DT_li);
            writeGuidAttribute((Node*)qpn);
            QString attr;
            int ro = qpn->getReadOnly();
            if (ro < 0) {
                const ClassNode* cn = qpn->declarativeCppNode();
                if (cn && !qpn->isWritable(tree_))
                    attr = "read-only";
            }
            else if (ro > 0)
                attr = "read-only";
            if (qpn->isDefault()) {
                if (!attr.isEmpty())
                    attr += QLatin1Char(' ');
                attr += "default";
            }
            if (!attr.isEmpty())
                xmlWriter().writeAttribute("outputclass",attr);
            generateQmlItem(qpn, relative, marker, false);
            writeEndTag(); // </li>
            writeEndTag(); // </ul>
        }
        else {
            /*
              The QML property node has multiple override nodes.
              Process the whole list as we would for a QML property
              group.
             */
            NodeList::ConstIterator p = qpn->qmlPropNodes().begin();
            writeStartTag(DT_ul);
            while (p != qpn->qmlPropNodes().end()) {
                if ((*p)->type() == Node::QmlProperty) {
                    QmlPropertyNode* q = static_cast<QmlPropertyNode*>(*p);
                    writeStartTag(DT_li);
                    writeGuidAttribute((Node*)q);
                    QString attr;
                    int ro = qpn->getReadOnly();
                    if (ro < 0) {
                        if (!qpn->isWritable(tree_))
                            attr = "read-only";
                    }
                    else if (ro > 0)
                        attr = "read-only";
                    if (qpn->isDefault()) {
                        if (!attr.isEmpty())
                            attr += QLatin1Char(' ');
                        attr += "default";
                    }
                    if (!attr.isEmpty())
                        xmlWriter().writeAttribute("outputclass",attr);
                    generateQmlItem(q, relative, marker, false);
                    writeEndTag(); // </li>
                }
                ++p;
            }
            writeEndTag(); // </ul>
        }
    }
    else if (node->type() == Node::QmlSignal) {
        Node* n = const_cast<Node*>(node);
        writeStartTag(DT_ul);
        writeStartTag(DT_li);
        writeGuidAttribute(n);
        marked = getMarkedUpSynopsis(n, relative, marker, CodeMarker::Detailed);
        writeText(marked, marker, relative);
        writeEndTag(); // </li>
        writeEndTag(); // </ul>
    }
    else if (node->type() == Node::QmlSignalHandler) {
        Node* n = const_cast<Node*>(node);
        writeStartTag(DT_ul);
        writeStartTag(DT_li);
        writeGuidAttribute(n);
        marked = getMarkedUpSynopsis(n, relative, marker, CodeMarker::Detailed);
        writeText(marked, marker, relative);
        writeEndTag(); // </li>
        writeEndTag(); // </ul>
    }
    else if (node->type() == Node::QmlMethod) {
        Node* n = const_cast<Node*>(node);
        writeStartTag(DT_ul);
        writeStartTag(DT_li);
        writeGuidAttribute(n);
        marked = getMarkedUpSynopsis(n, relative, marker, CodeMarker::Detailed);
        writeText(marked, marker, relative);
        writeEndTag(); // </li>
        writeEndTag(); // </ul>
    }
    generateStatus(node, marker);
    generateBody(node, marker);
    generateThreadSafeness(node, marker);
    generateSince(node, marker);
    generateAlsoList(node, marker);
}

/*!
  Output the "Inherits" line for the QML element,
  if there should be one.
 */
void DitaXmlGenerator::generateQmlInherits(const QmlClassNode* qcn, CodeMarker* marker)
{
    if (!qcn)
        return;
    const FakeNode* base = qcn->qmlBase();
    if (base) {
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","inherits");
        Text text;
        text << "[Inherits ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(base));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, base->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << "]";
        generateText(text, qcn, marker);
        writeEndTag(); // </p>
    }
}

/*!
  Output the "[Xxx instantiates the C++ class QmlGraphicsXxx]"
  line for the QML element, if there should be one.

  If there is no class node, or if the class node status
  is set to Node::Internal, do nothing.
 */
void DitaXmlGenerator::generateQmlInstantiates(const QmlClassNode* qcn,
                                               CodeMarker* marker)
{
    const ClassNode* cn = qcn->classNode();
    if (cn && (cn->status() != Node::Internal)) {
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","instantiates");
        Text text;
        text << "[";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, qcn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << " instantiates the C++ class ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << "]";
        generateText(text, qcn, marker);
        writeEndTag(); // </p>
    }
}

/*!
  Output the "[QmlGraphicsXxx is instantiated by QML element Xxx]"
  line for the class, if there should be one.

  If there is no QML element, or if the class node status
  is set to Node::Internal, do nothing.
 */
void DitaXmlGenerator::generateInstantiatedBy(const ClassNode* cn,
                                              CodeMarker* marker)
{
    if (cn &&  cn->status() != Node::Internal && cn->qmlElement() != 0) {
        const QmlClassNode* qcn = cn->qmlElement();
        writeStartTag(DT_p);
        xmlWriter().writeAttribute("outputclass","instantiated-by");
        Text text;
        text << "[";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << " is instantiated by QML element ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, qcn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << "]";
        generateText(text, cn, marker);
        writeEndTag(); // </p>
    }
}

/*!
  Return the full qualification of the node \a n, but without
  the name of \a n itself. e.g. A::B::C
 */
QString DitaXmlGenerator::fullQualification(const Node* n)
{
    QString fq;
    InnerNode* in = n->parent();
    while (in) {
        if ((in->type() == Node::Class) ||
                (in->type() == Node::Namespace)) {
            if (in->name().isEmpty())
                break;
            if (fq.isEmpty())
                fq = in->name();
            else
                fq = in->name() + "::" + fq;
        }
        else
            break;
        in = in->parent();
    }
    return fq;
}

/*!
  Outputs the <cxxClassDerivations> element.
  \code
 <cxxClassDerivations>
  <cxxClassDerivation>
   ...
  </cxxClassDerivation>
  ...
 </cxxClassDerivations>
  \endcode

  The <cxxClassDerivation> element is:

  \code
 <cxxClassDerivation>
  <cxxClassDerivationAccessSpecifier value="public"/>
  <cxxClassBaseClass href="class_base">Base</cxxClassBaseClass>
 </cxxClassDerivation>
  \endcode
 */
void DitaXmlGenerator::writeDerivations(const ClassNode* cn, CodeMarker* marker)
{
    QList<RelatedClass>::ConstIterator r;

    if (!cn->baseClasses().isEmpty()) {
        writeStartTag(DT_cxxClassDerivations);
        r = cn->baseClasses().begin();
        while (r != cn->baseClasses().end()) {
            writeStartTag(DT_cxxClassDerivation);
            writeStartTag(DT_cxxClassDerivationAccessSpecifier);
            xmlWriter().writeAttribute("value",(*r).accessString());
            writeEndTag(); // </cxxClassDerivationAccessSpecifier>

            // not included: <cxxClassDerivationVirtual>

            writeStartTag(DT_cxxClassBaseClass);
            QString attr = fileName((*r).node) + QLatin1Char('#') + (*r).node->guid();
            xmlWriter().writeAttribute("href",attr);
            writeCharacters(marker->plainFullName((*r).node));
            writeEndTag(); // </cxxClassBaseClass>

            // not included: <ClassBaseStruct> or <cxxClassBaseUnion>

            writeEndTag(); // </cxxClassDerivation>

            // not included: <cxxStructDerivation>

            ++r;
        }
        writeEndTag(); // </cxxClassDerivations>
    }
}

/*!
  Writes a <cxxXXXAPIItemLocation> element, depending on the
  type of the node \a n, which can be a class, function, enum,
  typedef, or property.
 */
void DitaXmlGenerator::writeLocation(const Node* n)
{
    DitaTag s1, s2, s3a, s3b;
    s1 = DT_cxxClassAPIItemLocation;
    s2 = DT_cxxClassDeclarationFile;
    s3a = DT_cxxClassDeclarationFileLineStart;
    s3b = DT_cxxClassDeclarationFileLineEnd;
    if (n->type() == Node::Class || n->type() == Node::Namespace) {
        s1 = DT_cxxClassAPIItemLocation;
        s2 = DT_cxxClassDeclarationFile;
        s3a = DT_cxxClassDeclarationFileLineStart;
        s3b = DT_cxxClassDeclarationFileLineEnd;
    }
    else if (n->type() == Node::Function) {
        FunctionNode* fn = const_cast<FunctionNode*>(static_cast<const FunctionNode*>(n));
        if (fn->isMacro()) {
            s1 = DT_cxxDefineAPIItemLocation;
            s2 = DT_cxxDefineDeclarationFile;
            s3a = DT_cxxDefineDeclarationFileLine;
            s3b = DT_NONE;
        }
        else {
            s1 = DT_cxxFunctionAPIItemLocation;
            s2 = DT_cxxFunctionDeclarationFile;
            s3a = DT_cxxFunctionDeclarationFileLine;
            s3b = DT_NONE;
        }
    }
    else if (n->type() == Node::Enum) {
        s1 = DT_cxxEnumerationAPIItemLocation;
        s2 = DT_cxxEnumerationDeclarationFile;
        s3a = DT_cxxEnumerationDeclarationFileLineStart;
        s3b = DT_cxxEnumerationDeclarationFileLineEnd;
    }
    else if (n->type() == Node::Typedef) {
        s1 = DT_cxxTypedefAPIItemLocation;
        s2 = DT_cxxTypedefDeclarationFile;
        s3a = DT_cxxTypedefDeclarationFileLine;
        s3b = DT_NONE;
    }
    else if ((n->type() == Node::Property) ||
             (n->type() == Node::Variable)) {
        s1 = DT_cxxVariableAPIItemLocation;
        s2 = DT_cxxVariableDeclarationFile;
        s3a = DT_cxxVariableDeclarationFileLine;
        s3b = DT_NONE;
    }
    writeStartTag(s1);
    writeStartTag(s2);
    xmlWriter().writeAttribute("name","filePath");
    xmlWriter().writeAttribute("value",n->location().filePath());
    writeEndTag(); // <s2>
    writeStartTag(s3a);
    xmlWriter().writeAttribute("name","lineNumber");
    QString lineNr;
    xmlWriter().writeAttribute("value",lineNr.setNum(n->location().lineNo()));
    writeEndTag(); // </s3a>
    if (s3b != DT_NONE) {
        writeStartTag(s3b);
        xmlWriter().writeAttribute("name","lineNumber");
        QString lineNr;
        xmlWriter().writeAttribute("value",lineNr.setNum(n->location().lineNo()));
        writeEndTag(); // </s3b>
    }
    writeEndTag(); // </cxx<s1>ApiItemLocation>
}

/*!
  Write the <cxxFunction> elements.
 */
void DitaXmlGenerator::writeFunctions(const Section& s,
                                      const InnerNode* parent,
                                      CodeMarker* marker,
                                      const QString& attribute)
{
    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Function) {
            FunctionNode* fn = const_cast<FunctionNode*>(static_cast<const FunctionNode*>(*m));
            writeStartTag(DT_cxxFunction);
            xmlWriter().writeAttribute("id",fn->guid());
            if (fn->metaness() == FunctionNode::Signal)
                xmlWriter().writeAttribute("otherprops","signal");
            else if (fn->metaness() == FunctionNode::Slot)
                xmlWriter().writeAttribute("otherprops","slot");
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(fn->name());
            writeEndTag(); // </apiName>
            generateBrief(fn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxFunctionDetail);
            writeStartTag(DT_cxxFunctionDefinition);
            writeStartTag(DT_cxxFunctionAccessSpecifier);
            xmlWriter().writeAttribute("value",fn->accessString());
            writeEndTag(); // <cxxFunctionAccessSpecifier>

            // not included: <cxxFunctionStorageClassSpecifierExtern>

            if (fn->isStatic()) {
                writeStartTag(DT_cxxFunctionStorageClassSpecifierStatic);
                xmlWriter().writeAttribute("name","static");
                xmlWriter().writeAttribute("value","static");
                writeEndTag(); // <cxxFunctionStorageClassSpecifierStatic>
            }

            // not included: <cxxFunctionStorageClassSpecifierMutable>,

            if (fn->isConst()) {
                writeStartTag(DT_cxxFunctionConst);
                xmlWriter().writeAttribute("name","const");
                xmlWriter().writeAttribute("value","const");
                writeEndTag(); // <cxxFunctionConst>
            }

            // not included: <cxxFunctionExplicit>
            //               <cxxFunctionInline

            if (fn->virtualness() != FunctionNode::NonVirtual) {
                writeStartTag(DT_cxxFunctionVirtual);
                xmlWriter().writeAttribute("name","virtual");
                xmlWriter().writeAttribute("value","virtual");
                writeEndTag(); // <cxxFunctionVirtual>
                if (fn->virtualness() == FunctionNode::PureVirtual) {
                    writeStartTag(DT_cxxFunctionPureVirtual);
                    xmlWriter().writeAttribute("name","pure virtual");
                    xmlWriter().writeAttribute("value","pure virtual");
                    writeEndTag(); // <cxxFunctionPureVirtual>
                }
            }

            if (fn->name() == parent->name()) {
                writeStartTag(DT_cxxFunctionConstructor);
                xmlWriter().writeAttribute("name","constructor");
                xmlWriter().writeAttribute("value","constructor");
                writeEndTag(); // <cxxFunctionConstructor>
            }
            else if (fn->name()[0] == QChar('~')) {
                writeStartTag(DT_cxxFunctionDestructor);
                xmlWriter().writeAttribute("name","destructor");
                xmlWriter().writeAttribute("value","destructor");
                writeEndTag(); // <cxxFunctionDestructor>
            }
            else {
                writeStartTag(DT_cxxFunctionDeclaredType);
                QString src = marker->typified(fn->returnType());
                replaceTypesWithLinks(fn,parent,marker,src);
                writeEndTag(); // <cxxFunctionDeclaredType>
            }

            // not included: <cxxFunctionReturnType>

            QString fq = fullQualification(fn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxFunctionScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxFunctionScopedName>
            }
            writeStartTag(DT_cxxFunctionPrototype);
            writeCharacters(fn->signature(true));
            writeEndTag(); // <cxxFunctionPrototype>

            QString fnl = fn->signature(false);
            int idx = fnl.indexOf(' ');
            if (idx < 0)
                idx = 0;
            else
                ++idx;
            fnl = fn->parent()->name() + "::" + fnl.mid(idx);
            writeStartTag(DT_cxxFunctionNameLookup);
            writeCharacters(fnl);
            writeEndTag(); // <cxxFunctionNameLookup>

            if (!fn->isInternal() && fn->isReimp() && fn->reimplementedFrom() != 0) {
                FunctionNode* rfn = (FunctionNode*)fn->reimplementedFrom();
                if (rfn && !rfn->isInternal()) {
                    writeStartTag(DT_cxxFunctionReimplemented);
                    xmlWriter().writeAttribute("href",rfn->ditaXmlHref());
                    writeCharacters(marker->plainFullName(rfn));
                    writeEndTag(); // </cxxFunctionReimplemented>
                }
            }
            writeParameters(fn,parent,marker);
            writeLocation(fn);
            writeEndTag(); // <cxxFunctionDefinition>

            writeApiDesc(fn, marker, QString());
            // generateAlsoList(inner, marker);

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxFunctionDetail>
            writeEndTag(); // </cxxFunction>

            if (fn->metaness() == FunctionNode::Ctor ||
                    fn->metaness() == FunctionNode::Dtor ||
                    fn->overloadNumber() != 1) {
            }
        }
        ++m;
    }
}

static const QString typeTag("type");
static const QChar charLangle = '<';
static const QChar charAt = '@';

/*!
  This function replaces class and enum names with <apiRelation>
  elements, i.e. links.
 */
void DitaXmlGenerator::replaceTypesWithLinks(const Node* n,
                                             const InnerNode* parent,
                                             CodeMarker* marker,
                                             QString& src)
{
    QStringRef arg;
    QStringRef par1;
    int srcSize = src.size();
    QString text;
    for (int i=0; i<srcSize;) {
        if (src.at(i) == charLangle && src.at(i+1) == charAt) {
            if (!text.isEmpty()) {
                writeCharacters(text);
                text.clear();
            }
            i += 2;
            if (parseArg(src, typeTag, &i, srcSize, &arg, &par1)) {
                const Node* tn = marker->resolveTarget(arg.toString(), tree_, parent, n);
                if (tn) {
                    //Do not generate a link from a C++ function to a QML Basic Type (such as int)
                    if (n->type() == Node::Function && tn->subType() == Node::QmlBasicType)
                        writeCharacters(arg.toString());
                    else
                        addLink(linkForNode(tn,parent),arg,DT_apiRelation);
                }
                else {
                    // Write simple arguments, like void and bool,
                    // which do not have a Qt defined target.
                    writeCharacters(arg.toString());
                }
            }
        }
        else {
            text += src.at(i++);
        }
    }
    if (!text.isEmpty()) {
        writeCharacters(text);
        text.clear();
    }
}

/*!
  This function writes the <cxxFunctionParameters> element.
 */
void DitaXmlGenerator::writeParameters(const FunctionNode* fn,
                                       const InnerNode* parent,
                                       CodeMarker* marker)
{
    const QList<Parameter>& parameters = fn->parameters();
    if (!parameters.isEmpty()) {
        writeStartTag(DT_cxxFunctionParameters);
        QList<Parameter>::ConstIterator p = parameters.begin();
        while (p != parameters.end()) {
            writeStartTag(DT_cxxFunctionParameter);
            writeStartTag(DT_cxxFunctionParameterDeclaredType);
            QString src = marker->typified((*p).leftType());
            replaceTypesWithLinks(fn,parent,marker,src);
            //writeCharacters((*p).leftType());
            if (!(*p).rightType().isEmpty())
                writeCharacters((*p).rightType());
            writeEndTag(); // <cxxFunctionParameterDeclaredType>
            writeStartTag(DT_cxxFunctionParameterDeclarationName);
            writeCharacters((*p).name());
            writeEndTag(); // <cxxFunctionParameterDeclarationName>

            // not included: <cxxFunctionParameterDefinitionName>

            if (!(*p).defaultValue().isEmpty()) {
                writeStartTag(DT_cxxFunctionParameterDefaultValue);
                writeCharacters((*p).defaultValue());
                writeEndTag(); // <cxxFunctionParameterDefaultValue>
            }

            // not included: <apiDefNote>

            writeEndTag(); // <cxxFunctionParameter>
            ++p;
        }
        writeEndTag(); // <cxxFunctionParameters>
    }
}

/*!
  This function writes the enum types.
 */
void DitaXmlGenerator::writeEnumerations(const Section& s,
                                         CodeMarker* marker,
                                         const QString& attribute)
{
    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Enum) {
            const EnumNode* en = static_cast<const EnumNode*>(*m);
            writeStartTag(DT_cxxEnumeration);
            xmlWriter().writeAttribute("id",en->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(en->name());
            writeEndTag(); // </apiName>
            generateBrief(en,marker);

            // not included <prolog>

            writeStartTag(DT_cxxEnumerationDetail);
            writeStartTag(DT_cxxEnumerationDefinition);
            writeStartTag(DT_cxxEnumerationAccessSpecifier);
            xmlWriter().writeAttribute("value",en->accessString());
            writeEndTag(); // <cxxEnumerationAccessSpecifier>

            QString fq = fullQualification(en);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxEnumerationScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxEnumerationScopedName>
            }
            const QList<EnumItem>& items = en->items();
            if (!items.isEmpty()) {
                writeStartTag(DT_cxxEnumerationPrototype);
                writeCharacters(en->name());
                xmlWriter().writeCharacters(" = { ");
                QList<EnumItem>::ConstIterator i = items.begin();
                while (i != items.end()) {
                    writeCharacters((*i).name());
                    if (!(*i).value().isEmpty()) {
                        xmlWriter().writeCharacters(" = ");
                        writeCharacters((*i).value());
                    }
                    ++i;
                    if (i != items.end())
                        xmlWriter().writeCharacters(", ");
                }
                xmlWriter().writeCharacters(" }");
                writeEndTag(); // <cxxEnumerationPrototype>
            }

            writeStartTag(DT_cxxEnumerationNameLookup);
            writeCharacters(en->parent()->name() + "::" + en->name());
            writeEndTag(); // <cxxEnumerationNameLookup>

            // not included: <cxxEnumerationReimplemented>

            if (!items.isEmpty()) {
                writeStartTag(DT_cxxEnumerators);
                QList<EnumItem>::ConstIterator i = items.begin();
                while (i != items.end()) {
                    writeStartTag(DT_cxxEnumerator);
                    writeStartTag(DT_apiName);
                    writeCharacters((*i).name());
                    writeEndTag(); // </apiName>

                    QString fq = fullQualification(en->parent());
                    if (!fq.isEmpty()) {
                        writeStartTag(DT_cxxEnumeratorScopedName);
                        writeCharacters(fq + "::" + (*i).name());
                        writeEndTag(); // <cxxEnumeratorScopedName>
                    }
                    writeStartTag(DT_cxxEnumeratorPrototype);
                    writeCharacters((*i).name());
                    writeEndTag(); // <cxxEnumeratorPrototype>
                    writeStartTag(DT_cxxEnumeratorNameLookup);
                    writeCharacters(en->parent()->name() + "::" + (*i).name());
                    writeEndTag(); // <cxxEnumeratorNameLookup>

                    if (!(*i).value().isEmpty()) {
                        writeStartTag(DT_cxxEnumeratorInitialiser);
                        if ((*i).value().toInt(0,16) == 0)
                            xmlWriter().writeAttribute("value", "0");
                        else
                            xmlWriter().writeAttribute("value", (*i).value());
                        writeEndTag(); // <cxxEnumeratorInitialiser>
                    }

                    // not included: <cxxEnumeratorAPIItemLocation>

                    if (!(*i).text().isEmpty()) {
                        writeStartTag(DT_apiDesc);
                        generateText((*i).text(), en, marker);
                        writeEndTag(); // </apiDesc>
                    }
                    writeEndTag(); // <cxxEnumerator>
                    ++i;
                }
                writeEndTag(); // <cxxEnumerators>
            }

            writeLocation(en);
            writeEndTag(); // <cxxEnumerationDefinition>

            writeApiDesc(en, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxEnumerationDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxEnumeration>
        }
        ++m;
    }
}

/*!
  This function writes the output for the \typedef commands.
 */
void DitaXmlGenerator::writeTypedefs(const Section& s,
                                     CodeMarker* marker,
                                     const QString& attribute)

{
    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Typedef) {
            const TypedefNode* tn = static_cast<const TypedefNode*>(*m);
            writeStartTag(DT_cxxTypedef);
            xmlWriter().writeAttribute("id",tn->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(tn->name());
            writeEndTag(); // </apiName>
            generateBrief(tn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxTypedefDetail);
            writeStartTag(DT_cxxTypedefDefinition);
            writeStartTag(DT_cxxTypedefAccessSpecifier);
            xmlWriter().writeAttribute("value",tn->accessString());
            writeEndTag(); // <cxxTypedefAccessSpecifier>

            // not included: <cxxTypedefDeclaredType>

            QString fq = fullQualification(tn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxTypedefScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxTypedefScopedName>
            }

            // not included: <cxxTypedefPrototype>

            writeStartTag(DT_cxxTypedefNameLookup);
            writeCharacters(tn->parent()->name() + "::" + tn->name());
            writeEndTag(); // <cxxTypedefNameLookup>

            // not included: <cxxTypedefReimplemented>

            writeLocation(tn);
            writeEndTag(); // <cxxTypedefDefinition>

            writeApiDesc(tn, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxTypedefDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxTypedef>
        }
        ++m;
    }
}

/*!
  This function writes the output for the \property commands.
  This is the Q_PROPERTYs.
 */
void DitaXmlGenerator::writeProperties(const Section& s,
                                       CodeMarker* marker,
                                       const QString& attribute)
{
    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Property) {
            const PropertyNode* pn = static_cast<const PropertyNode*>(*m);
            writeStartTag(DT_cxxVariable);
            xmlWriter().writeAttribute("id",pn->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(pn->name());
            writeEndTag(); // </apiName>
            generateBrief(pn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxVariableDetail);
            writeStartTag(DT_cxxVariableDefinition);
            writeStartTag(DT_cxxVariableAccessSpecifier);
            xmlWriter().writeAttribute("value",pn->accessString());
            writeEndTag(); // <cxxVariableAccessSpecifier>

            // not included: <cxxVariableStorageClassSpecifierExtern>,
            //               <cxxVariableStorageClassSpecifierStatic>,
            //               <cxxVariableStorageClassSpecifierMutable>,
            //               <cxxVariableConst>, <cxxVariableVolatile>

            if (!pn->qualifiedDataType().isEmpty()) {
                writeStartTag(DT_cxxVariableDeclaredType);
                writeCharacters(pn->qualifiedDataType());
                writeEndTag(); // <cxxVariableDeclaredType>
            }
            QString fq = fullQualification(pn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxVariableScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxVariableScopedName>
            }

            writeStartTag(DT_cxxVariablePrototype);
            xmlWriter().writeCharacters("Q_PROPERTY(");
            writeCharacters(pn->qualifiedDataType());
            xmlWriter().writeCharacters(" ");
            writeCharacters(pn->name());
            writePropertyParameter("READ",pn->getters());
            writePropertyParameter("WRITE",pn->setters());
            writePropertyParameter("RESET",pn->resetters());
            writePropertyParameter("NOTIFY",pn->notifiers());
            if (pn->isDesignable() != pn->designableDefault()) {
                xmlWriter().writeCharacters(" DESIGNABLE ");
                if (!pn->runtimeDesignabilityFunction().isEmpty())
                    writeCharacters(pn->runtimeDesignabilityFunction());
                else
                    xmlWriter().writeCharacters(pn->isDesignable() ? "true" : "false");
            }
            if (pn->isScriptable() != pn->scriptableDefault()) {
                xmlWriter().writeCharacters(" SCRIPTABLE ");
                if (!pn->runtimeScriptabilityFunction().isEmpty())
                    writeCharacters(pn->runtimeScriptabilityFunction());
                else
                    xmlWriter().writeCharacters(pn->isScriptable() ? "true" : "false");
            }
            if (pn->isWritable() != pn->writableDefault()) {
                xmlWriter().writeCharacters(" STORED ");
                xmlWriter().writeCharacters(pn->isStored() ? "true" : "false");
            }
            if (pn->isUser() != pn->userDefault()) {
                xmlWriter().writeCharacters(" USER ");
                xmlWriter().writeCharacters(pn->isUser() ? "true" : "false");
            }
            if (pn->isConstant())
                xmlWriter().writeCharacters(" CONSTANT");
            if (pn->isFinal())
                xmlWriter().writeCharacters(" FINAL");
            xmlWriter().writeCharacters(")");
            writeEndTag(); // <cxxVariablePrototype>

            writeStartTag(DT_cxxVariableNameLookup);
            writeCharacters(pn->parent()->name() + "::" + pn->name());
            writeEndTag(); // <cxxVariableNameLookup>

            if (pn->overriddenFrom() != 0) {
                PropertyNode* opn = (PropertyNode*)pn->overriddenFrom();
                writeStartTag(DT_cxxVariableReimplemented);
                xmlWriter().writeAttribute("href",opn->ditaXmlHref());
                writeCharacters(marker->plainFullName(opn));
                writeEndTag(); // </cxxVariableReimplemented>
            }

            writeLocation(pn);
            writeEndTag(); // <cxxVariableDefinition>

            writeApiDesc(pn, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxVariableDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxVariable>
        }
        ++m;
    }
}

/*!
  This function outputs the nodes resulting from \variable commands.
 */
void DitaXmlGenerator::writeDataMembers(const Section& s,
                                        CodeMarker* marker,
                                        const QString& attribute)
{
    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Variable) {
            const VariableNode* vn = static_cast<const VariableNode*>(*m);
            writeStartTag(DT_cxxVariable);
            xmlWriter().writeAttribute("id",vn->guid());
            if (!attribute.isEmpty())
                xmlWriter().writeAttribute("outputclass",attribute);
            writeStartTag(DT_apiName);
            writeCharacters(vn->name());
            writeEndTag(); // </apiName>
            generateBrief(vn,marker);

            // not included: <prolog>

            writeStartTag(DT_cxxVariableDetail);
            writeStartTag(DT_cxxVariableDefinition);
            writeStartTag(DT_cxxVariableAccessSpecifier);
            xmlWriter().writeAttribute("value",vn->accessString());
            writeEndTag(); // <cxxVariableAccessSpecifier>

            // not included: <cxxVAriableStorageClassSpecifierExtern>

            if (vn->isStatic()) {
                writeStartTag(DT_cxxVariableStorageClassSpecifierStatic);
                xmlWriter().writeAttribute("name","static");
                xmlWriter().writeAttribute("value","static");
                writeEndTag(); // <cxxVariableStorageClassSpecifierStatic>
            }

            // not included: <cxxVAriableStorageClassSpecifierMutable>,
            //               <cxxVariableConst>, <cxxVariableVolatile>

            writeStartTag(DT_cxxVariableDeclaredType);
            writeCharacters(vn->leftType());
            if (!vn->rightType().isEmpty())
                writeCharacters(vn->rightType());
            writeEndTag(); // <cxxVariableDeclaredType>

            QString fq = fullQualification(vn);
            if (!fq.isEmpty()) {
                writeStartTag(DT_cxxVariableScopedName);
                writeCharacters(fq);
                writeEndTag(); // <cxxVariableScopedName>
            }

            writeStartTag(DT_cxxVariablePrototype);
            writeCharacters(vn->leftType() + QLatin1Char(' '));
            //writeCharacters(vn->parent()->name() + "::" + vn->name());
            writeCharacters(vn->name());
            if (!vn->rightType().isEmpty())
                writeCharacters(vn->rightType());
            writeEndTag(); // <cxxVariablePrototype>

            writeStartTag(DT_cxxVariableNameLookup);
            writeCharacters(vn->parent()->name() + "::" + vn->name());
            writeEndTag(); // <cxxVariableNameLookup>

            // not included: <cxxVariableReimplemented>

            writeLocation(vn);
            writeEndTag(); // <cxxVariableDefinition>

            writeApiDesc(vn, marker, QString());

            // not included: <example> or <apiImpl>

            writeEndTag(); // </cxxVariableDetail>

            // not included: <related-links>

            writeEndTag(); // </cxxVariable>
        }
        ++m;
    }
}

/*!
  This function writes a \macro as a <cxxDefine>.
 */
void DitaXmlGenerator::writeMacros(const Section& s,
                                   CodeMarker* marker,
                                   const QString& attribute)
{
    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Function) {
            const FunctionNode* fn = static_cast<const FunctionNode*>(*m);
            if (fn->isMacro()) {
                writeStartTag(DT_cxxDefine);
                xmlWriter().writeAttribute("id",fn->guid());
                if (!attribute.isEmpty())
                    xmlWriter().writeAttribute("outputclass",attribute);
                writeStartTag(DT_apiName);
                writeCharacters(fn->name());
                writeEndTag(); // </apiName>
                generateBrief(fn,marker);

                // not included: <prolog>

                writeStartTag(DT_cxxDefineDetail);
                writeStartTag(DT_cxxDefineDefinition);
                writeStartTag(DT_cxxDefineAccessSpecifier);
                xmlWriter().writeAttribute("value",fn->accessString());
                writeEndTag(); // <cxxDefineAccessSpecifier>

                writeStartTag(DT_cxxDefinePrototype);
                xmlWriter().writeCharacters("#define ");
                writeCharacters(fn->name());
                if (fn->metaness() == FunctionNode::MacroWithParams) {
                    QStringList params = fn->parameterNames();
                    if (!params.isEmpty()) {
                        xmlWriter().writeCharacters("(");
                        for (int i = 0; i < params.size(); ++i) {
                            if (params[i].isEmpty())
                                xmlWriter().writeCharacters("...");
                            else
                                writeCharacters(params[i]);
                            if ((i+1) < params.size())
                                xmlWriter().writeCharacters(", ");
                        }
                        xmlWriter().writeCharacters(")");
                    }
                }
                writeEndTag(); // <cxxDefinePrototype>

                writeStartTag(DT_cxxDefineNameLookup);
                writeCharacters(fn->name());
                writeEndTag(); // <cxxDefineNameLookup>

                if (fn->reimplementedFrom() != 0) {
                    FunctionNode* rfn = (FunctionNode*)fn->reimplementedFrom();
                    writeStartTag(DT_cxxDefineReimplemented);
                    xmlWriter().writeAttribute("href",rfn->ditaXmlHref());
                    writeCharacters(marker->plainFullName(rfn));
                    writeEndTag(); // </cxxDefineReimplemented>
                }

                if (fn->metaness() == FunctionNode::MacroWithParams) {
                    QStringList params = fn->parameterNames();
                    if (!params.isEmpty()) {
                        writeStartTag(DT_cxxDefineParameters);
                        for (int i = 0; i < params.size(); ++i) {
                            writeStartTag(DT_cxxDefineParameter);
                            writeStartTag(DT_cxxDefineParameterDeclarationName);
                            writeCharacters(params[i]);
                            writeEndTag(); // <cxxDefineParameterDeclarationName>

                            // not included: <apiDefNote>

                            writeEndTag(); // <cxxDefineParameter>
                        }
                        writeEndTag(); // <cxxDefineParameters>
                    }
                }

                writeLocation(fn);
                writeEndTag(); // <cxxDefineDefinition>

                writeApiDesc(fn, marker, QString());

                // not included: <example> or <apiImpl>

                writeEndTag(); // </cxxDefineDetail>

                // not included: <related-links>

                writeEndTag(); // </cxxDefine>
            }
        }
        ++m;
    }
}

/*!
  This function writes one parameter of a Q_PROPERTY macro.
  The property is identified by \a tag ("READ" "WRIE" etc),
  and it is found in the 'a nlist.
 */
void DitaXmlGenerator::writePropertyParameter(const QString& tag, const NodeList& nlist)
{
    NodeList::const_iterator n = nlist.begin();
    while (n != nlist.end()) {
        xmlWriter().writeCharacters(" ");
        writeCharacters(tag);
        xmlWriter().writeCharacters(" ");
        writeCharacters((*n)->name());
        ++n;
    }
}

/*!
  Calls beginSubPage() in the base class to open the file.
  Then creates a new XML stream writer using the IO device
  from opened file and pushes the XML writer onto a stackj.
  Creates the file named \a fileName in the output directory.
  Attaches a QTextStream to the created file, which is written
  to all over the place using out(). Finally, it sets some
  parameters in the XML writer and calls writeStartDocument().

  It also ensures that a GUID map is created for the output file.
 */
void DitaXmlGenerator::beginSubPage(const InnerNode* node,
                                    const QString& fileName)
{
    Generator::beginSubPage(node,fileName);
    (void) lookupGuidMap(fileName);
    QXmlStreamWriter* writer = new QXmlStreamWriter(out().device());
    xmlWriterStack.push(writer);
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(4);
    writer->writeStartDocument();
    clearSectionNesting();
}

/*!
  Calls writeEndDocument() and then pops the XML stream writer
  off the stack and deletes it. Then it calls endSubPage() in
  the base class to close the device.
 */
void DitaXmlGenerator::endSubPage()
{
    if (inSection())
        qDebug() << "Missing </section> in" << outFileName() << sectionNestingLevel;
    xmlWriter().writeEndDocument();
    delete xmlWriterStack.pop();
    Generator::endSubPage();
}

/*!
  Returns a reference to the XML stream writer currently in use.
  There is one XML stream writer open for each XML file being
  written, and they are kept on a stack. The one on top of the
  stack is the one being written to at the moment.
 */
QXmlStreamWriter& DitaXmlGenerator::xmlWriter()
{
    return *xmlWriterStack.top();
}

/*!
  Writes the \e {<apiDesc>} element for \a node to the current XML
  stream using the code \a marker and the \a title.
 */
void DitaXmlGenerator::writeApiDesc(const Node* node,
                                    CodeMarker* marker,
                                    const QString& title)
{
    if (!node->doc().isEmpty()) {
        inDetailedDescription = true;
        enterApiDesc(QString(),title);
        generateBody(node, marker);
        generateAlsoList(node, marker);
        leaveSection();
    }
    inDetailedDescription = false;
}

/*!
  Write the nested class elements.
 */
void DitaXmlGenerator::writeNestedClasses(const Section& s,
                                          const Node* n)
{
    if (s.members.isEmpty())
        return;
    writeStartTag(DT_cxxClassNested);
    writeStartTag(DT_cxxClassNestedDetail);

    NodeList::ConstIterator m = s.members.begin();
    while (m != s.members.end()) {
        if ((*m)->type() == Node::Class) {
            writeStartTag(DT_cxxClassNestedClass);
            QString link = linkForNode((*m), n);
            xmlWriter().writeAttribute("href", link);
            QString name = n->name() + "::" + (*m)->name();
            writeCharacters(name);
            writeEndTag(); // <cxxClassNestedClass>
        }
        ++m;
    }
    writeEndTag(); // <cxxClassNestedDetail>
    writeEndTag(); // <cxxClassNested>
}

/*!
  Recursive writing of DITA XML files from the root \a node.
 */
void
DitaXmlGenerator::generateInnerNode(const InnerNode* node)
{
    if (!node->url().isNull())
        return;

    if (node->type() == Node::Fake) {
        const FakeNode *fakeNode = static_cast<const FakeNode *>(node);
        if (fakeNode->subType() == Node::ExternalPage)
            return;
        if (fakeNode->subType() == Node::Image)
            return;
        if (fakeNode->subType() == Node::QmlPropertyGroup)
            return;
        if (fakeNode->subType() == Node::Page) {
            if (node->count() > 0)
                qDebug("PAGE %s HAS CHILDREN", qPrintable(fakeNode->title()));
        }
    }

    /*
      Obtain a code marker for the source file.
     */
    CodeMarker *marker = CodeMarker::markerForFileName(node->location().filePath());

    if (node->parent() != 0) {
        if (!node->name().endsWith(".ditamap"))
            beginSubPage(node, fileName(node));
        if (node->type() == Node::Namespace || node->type() == Node::Class) {
            generateClassLikeNode(node, marker);
        }
        else if (node->type() == Node::Fake) {
            if (node->subType() == Node::HeaderFile)
                generateClassLikeNode(node, marker);
            else if (node->subType() == Node::QmlClass)
                generateClassLikeNode(node, marker);
            else
                generateFakeNode(static_cast<const FakeNode*>(node), marker);
        }
        if (!node->name().endsWith(".ditamap"))
            endSubPage();
    }

    NodeList::ConstIterator c = node->childNodes().begin();
    while (c != node->childNodes().end()) {
        if ((*c)->isInnerNode() && (*c)->access() != Node::Private)
            generateInnerNode((const InnerNode*) *c);
        ++c;
    }
}

/*!
  Returns true if \a format is "XML" or "HTML" .
 */
bool DitaXmlGenerator::canHandleFormat(const QString& format)
{
    return (format == "HTML") || (format == this->format());
}

/*!
  If the node multimap \a nmm contains nodes mapped to \a key,
  if any of the nodes mapped to \a key has the same href as the
  \a node, return true. Otherwise, return false.
 */
bool DitaXmlGenerator::isDuplicate(NodeMultiMap* nmm, const QString& key, Node* node)
{
    QList<Node*> matches = nmm->values(key);
    if (!matches.isEmpty()) {
        for (int i=0; i<matches.size(); ++i) {
            if (matches[i] == node)
                return true;
            if (fileName(node) == fileName(matches[i]))
                return true;
        }
    }
    return false;
}
/*!
  Collect all the nodes in the tree according to their type or subtype.

  If a node is found that is named index.html, return that node as the
  root page node.

  type: Class
  type: Namespace

  subtype: Example
  subtype: External page
  subtype: Group
  subtype: Header file
  subtype: Module
  subtype: Page
  subtype: QML basic type
  subtype: QML class
  subtype: QML module
 */
Node* DitaXmlGenerator::collectNodesByTypeAndSubtype(const InnerNode* parent)
{
    Node* rootPageNode = 0;
    const NodeList& children = parent->childNodes();
    if (children.size() == 0)
        return rootPageNode;

    QString message;
    for (int i=0; i<children.size(); ++i) {
        Node* child = children[i];
        if ((child->type() == Node::Fake) && (child->subType() == Node::Collision)) {
            const FakeNode* fake = static_cast<const FakeNode*>(child);
            Node* n = collectNodesByTypeAndSubtype(fake);
            if (n)
                rootPageNode = n;
            continue;
        }
        if (!child || child->isInternal() || child->doc().isEmpty())
            continue;

        if (child->name() == "index.html" || child->name() == "index") {
            rootPageNode = child;
        }

        switch (child->type()) {
        case Node::Namespace:
            if (!isDuplicate(nodeTypeMaps[Node::Namespace],child->name(),child))
                nodeTypeMaps[Node::Namespace]->insert(child->name(),child);
            break;
        case Node::Class:
            if (!isDuplicate(nodeTypeMaps[Node::Class],child->name(),child))
                nodeTypeMaps[Node::Class]->insert(child->name(),child);
            break;
        case Node::Fake:
            switch (child->subType()) {
            case Node::Example:
                if (!isDuplicate(nodeSubtypeMaps[Node::Example],child->title(),child))
                    nodeSubtypeMaps[Node::Example]->insert(child->title(),child);
                break;
            case Node::HeaderFile:
                if (!isDuplicate(nodeSubtypeMaps[Node::HeaderFile],child->title(),child))
                    nodeSubtypeMaps[Node::HeaderFile]->insert(child->title(),child);
                break;
            case Node::File:
                break;
            case Node::Image:
                break;
            case Node::Group:
                if (!isDuplicate(nodeSubtypeMaps[Node::Group],child->title(),child))
                    nodeSubtypeMaps[Node::Group]->insert(child->title(),child);
                break;
            case Node::Module:
                if (!isDuplicate(nodeSubtypeMaps[Node::Module],child->title(),child))
                    nodeSubtypeMaps[Node::Module]->insert(child->title(),child);
                break;
            case Node::Page:
                if (!isDuplicate(pageTypeMaps[child->pageType()],child->title(),child))
                    pageTypeMaps[child->pageType()]->insert(child->title(),child);
                break;
            case Node::ExternalPage:
                if (!isDuplicate(nodeSubtypeMaps[Node::ExternalPage],child->title(),child))
                    nodeSubtypeMaps[Node::ExternalPage]->insert(child->title(),child);
                break;
            case Node::QmlClass:
                if (!isDuplicate(nodeSubtypeMaps[Node::QmlClass],child->title(),child))
                    nodeSubtypeMaps[Node::QmlClass]->insert(child->title(),child);
                break;
            case Node::QmlPropertyGroup:
                break;
            case Node::QmlBasicType:
                if (!isDuplicate(nodeSubtypeMaps[Node::QmlBasicType],child->title(),child))
                    nodeSubtypeMaps[Node::QmlBasicType]->insert(child->title(),child);
                break;
            case Node::QmlModule:
                if (!isDuplicate(nodeSubtypeMaps[Node::QmlModule],child->title(),child))
                    nodeSubtypeMaps[Node::QmlModule]->insert(child->title(),child);
                break;
            case Node::Collision:
                qDebug() << "FAKE NODE: Collision";
                if (!isDuplicate(nodeSubtypeMaps[Node::Collision],child->title(),child))
                    nodeSubtypeMaps[Node::Collision]->insert(child->title(),child);
                break;
            default:
                break;
            }
            break;
        case Node::Enum:
            break;
        case Node::Typedef:
            break;
        case Node::Function:
            break;
        case Node::Property:
            break;
        case Node::Variable:
            break;
        case Node::Target:
            break;
        case Node::QmlProperty:
            break;
        case Node::QmlSignal:
            break;
        case Node::QmlSignalHandler:
            break;
        case Node::QmlMethod:
            break;
        default:
            break;
        }
    }
    return rootPageNode;
}

/*!
  Creates the DITA map for the qdoc run. The map is written
  to the file \e{qt.ditamap" in the DITA XML output directory.
 */
void DitaXmlGenerator::writeDitaMap(const Tree *tree)
{
    QString doctype;

/*
    Remove #if 0 to get a flat ditamap.
*/
#if 0
    beginSubPage(tree->root(),"qt.ditamap");
    doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";
    xmlWriter().writeDTD(doctype);
    writeStartTag(DT_map);
    writeStartTag(DT_topicmeta);
    writeStartTag(DT_shortdesc);
    xmlWriter().writeCharacters("The top level map for the Qt documentation");
    writeEndTag(); // </shortdesc>
    writeEndTag(); // </topicmeta>
    GuidMaps::iterator i = guidMaps.begin();
    while (i != guidMaps.end()) {
        writeStartTag(DT_topicref);
        if (i.key() != "qt.ditamap")
            xmlWriter().writeAttribute("href",i.key());
        writeEndTag(); // </topicref>
        ++i;
    }
    endSubPage();
#endif

    for (unsigned i=0; i<Node::LastType; ++i)
        nodeTypeMaps[i] = new NodeMultiMap;
    for (unsigned i=0; i<Node::LastSubtype; ++i)
        nodeSubtypeMaps[i] = new NodeMultiMap;
    for (unsigned i=0; i<Node::OnBeyondZebra; ++i)
        pageTypeMaps[i] = new NodeMultiMap;
    Node* rootPageNode = collectNodesByTypeAndSubtype(tree->root());

    beginSubPage(tree->root(),"qt.ditamap");

    doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";
    xmlWriter().writeDTD(doctype);
    writeStartTag(DT_map);
    writeStartTag(DT_topicmeta);
    writeStartTag(DT_shortdesc);
    xmlWriter().writeCharacters("The top level map for the Qt documentation");
    writeEndTag(); // </shortdesc>
    writeEndTag(); // </topicmeta>

    writeStartTag(DT_topicref);
    if (rootPageNode) {
        if (!rootPageNode->title().isEmpty())
            xmlWriter().writeAttribute("navtitle",rootPageNode->title());
        else
            xmlWriter().writeAttribute("navtitle",project);
        xmlWriter().writeAttribute("href",fileName(rootPageNode));
    }
    else
        xmlWriter().writeAttribute("navtitle",project);

    writeTopicrefs(pageTypeMaps[Node::OverviewPage], "overviews");
    writeTopicrefs(pageTypeMaps[Node::HowToPage], "howtos");
    writeTopicrefs(pageTypeMaps[Node::TutorialPage], "tutorials");
    writeTopicrefs(pageTypeMaps[Node::FAQPage], "faqs");
    writeTopicrefs(pageTypeMaps[Node::ArticlePage], "articles");
    writeTopicrefs(nodeSubtypeMaps[Node::Example], "examples");
    writeTopicrefs(nodeSubtypeMaps[Node::QmlClass], "QML types");
    writeTopicrefs(nodeTypeMaps[Node::Class], "C++ classes");
    writeTopicrefs(nodeTypeMaps[Node::Namespace], "C++ namespaces");
    writeTopicrefs(nodeSubtypeMaps[Node::HeaderFile], "header files");
    writeTopicrefs(nodeSubtypeMaps[Node::Module], "modules");
    writeTopicrefs(nodeSubtypeMaps[Node::Group], "groups");
    writeTopicrefs(nodeSubtypeMaps[Node::QmlModule], "QML modules");
    writeTopicrefs(nodeSubtypeMaps[Node::QmlBasicType], "QML basic types");

    writeEndTag(); // </topicref>
    endSubPage();

    for (unsigned i=0; i<Node::LastType; ++i)
        delete nodeTypeMaps[i];
    for (unsigned i=0; i<Node::LastSubtype; ++i)
        delete nodeSubtypeMaps[i];
    for (unsigned i=0; i<Node::OnBeyondZebra; ++i)
        delete pageTypeMaps[i];
}

/*!
  Creates the DITA map from the topicrefs in \a node,
  which is a DitaMapNode.
 */
void DitaXmlGenerator::writeDitaMap(const DitaMapNode* node)
{
    beginSubPage(node,node->name());

    QString doctype;
    doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";
    xmlWriter().writeDTD(doctype);
    writeStartTag(DT_map);
    writeStartTag(DT_topicmeta);
    writeStartTag(DT_shortdesc);
    xmlWriter().writeCharacters(node->title());
    writeEndTag(); // </shortdesc>
    writeEndTag(); // </topicmeta>
    const DitaRefList map = node->map();
    writeDitaRefs(map);
    endSubPage();
}

/*!
  Write the \a ditarefs to the current output file.
 */
void DitaXmlGenerator::writeDitaRefs(const DitaRefList& ditarefs)
{
    foreach (DitaRef* t, ditarefs) {
        if (t->isMapRef())
            writeStartTag(DT_mapref);
        else
            writeStartTag(DT_topicref);
        xmlWriter().writeAttribute("navtitle",t->navtitle());
        if (t->href().isEmpty()) {
            const FakeNode* fn = tree_->findFakeNodeByTitle(t->navtitle());
            if (fn)
                xmlWriter().writeAttribute("href",fileName(fn));
        }
        else
            xmlWriter().writeAttribute("href",t->href());
        if (t->subrefs() && !t->subrefs()->isEmpty())
            writeDitaRefs(*(t->subrefs()));
        writeEndTag(); // </topicref> or </mapref>
    }
}

void DitaXmlGenerator::writeTopicrefs(NodeMultiMap* nmm, const QString& navtitle)
{
    if (!nmm || nmm->isEmpty())
        return;
    writeStartTag(DT_topicref);
    xmlWriter().writeAttribute("navtitle",navtitle);
    NodeMultiMap::iterator i;
    NodeMultiMap *ditaMaps = pageTypeMaps[Node::DitaMapPage];

    /*!
       Put all pages that are already in a hand-written ditamap not in
       the qt.ditamap separately. It loops through all ditamaps recursively
       before deciding to write an article to qt.ditamap.
     */
    if ((navtitle == "articles" && ditaMaps && ditaMaps->size() > 0)) {
        NodeMultiMap::iterator mapIterator = ditaMaps->begin();
        while (mapIterator != ditaMaps->end()) {
            writeStartTag(DT_mapref);
            xmlWriter().writeAttribute("navtitle",mapIterator.key());
            xmlWriter().writeAttribute("href",fileName(mapIterator.value()));
            writeEndTag();
            ++mapIterator;
        }
        i = nmm->begin();
        while (i != nmm->end()) {
            // Hardcode not writing index.dita multiple times in the tree.
            // index.dita should only appear at the top of the ditamap.
            if (fileName(i.value()) == "index.dita") {
                i++;
                continue;
            }
            bool foundInDitaMap = false;
            mapIterator = ditaMaps->begin();
            while (mapIterator != ditaMaps->end()) {
                const DitaMapNode *dmNode = static_cast<const DitaMapNode *>(mapIterator.value());
                for (int count = 0; count < dmNode->map().count(); count++) {
                    if (dmNode->map().at(count)->navtitle() == i.key()) {
                        foundInDitaMap = true;
                    }
                    ++mapIterator;
                }
            }
            if (!foundInDitaMap) {
                writeStartTag(DT_topicref);
                xmlWriter().writeAttribute("navtitle",i.key());
                xmlWriter().writeAttribute("href",fileName(i.value()));
                writeEndTag(); // </topicref>
            }
            ++i;
        }
    }
    /*!
       Shortcut when there are no hand-written ditamaps or when we are
       not generating the articles list.
     */
    else {
        i = nmm->begin();
        while (i != nmm->end()) {
            // Hardcode not writing index.dita multiple times in the tree.
            // index.dita should only appear at the top of the ditamap.
            if (fileName(i.value()) == "index.dita") {
                i++;
                continue;
            }
            writeStartTag(DT_topicref);
            xmlWriter().writeAttribute("navtitle",i.key());
            xmlWriter().writeAttribute("href",fileName(i.value()));
            switch (i.value()->type()) {
            case Node::Class: {
                const NamespaceNode* nn = static_cast<const NamespaceNode*>(i.value());
                const NodeList& c = nn->childNodes();
                for (int j=0; j<c.size(); ++j) {
                    if (c[j]->isInternal() || c[j]->access() == Node::Private || c[j]->doc().isEmpty())
                        continue;
                    if (c[j]->type() == Node::Class) {
                        writeStartTag(DT_topicref);
                        xmlWriter().writeAttribute("navtitle",c[j]->name());
                        xmlWriter().writeAttribute("href",fileName(c[j]));
                        writeEndTag(); // </topicref>
                    }
                }
                break;
            }
            default:
                break;
            }
            writeEndTag(); // </topicref>
            ++i;
        }
    }
    writeEndTag(); // </topicref>
}


/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If a value
  for the tag is found, the element is written with the
  found value. Otherwise if \a force is set, an empty
  element is written using the tag.

  Returns true or false depending on whether it writes
  an element using the tag \a t.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
bool DitaXmlGenerator::writeMetadataElement(const InnerNode* inner,
                                            DitaXmlGenerator::DitaTag t,
                                            bool force)
{
    QString s = getMetadataElement(inner,t);
    if (s.isEmpty() && !force)
        return false;
    writeStartTag(t);
    if (!s.isEmpty())
        xmlWriter().writeCharacters(s);
    writeEndTag();
    return true;
}


/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If one or more
  value sfor the tag are found, the elements are written.
  Otherwise nothing is written.

  Returns true or false depending on whether it writes
  at least one element using the tag \a t.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
bool DitaXmlGenerator::writeMetadataElements(const InnerNode* inner,
                                             DitaXmlGenerator::DitaTag t)
{
    QStringList s = getMetadataElements(inner,t);
    if (s.isEmpty())
        return false;
    for (int i=0; i<s.size(); ++i) {
        writeStartTag(t);
        xmlWriter().writeCharacters(s[i]);
        writeEndTag();
    }
    return true;
}

/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If a value
  for the tag is found, the value is returned.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
QString DitaXmlGenerator::getMetadataElement(const InnerNode* inner, DitaXmlGenerator::DitaTag t)
{
    QString s = Generator::getMetadataElement(inner, ditaTags[t]);
    if (s.isEmpty())
        s = metadataDefault(t);
    return s;
}

/*!
  Looks up the tag name for \a t in the map of metadata
  values for the current topic in \a inner. If values
  for the tag are found, they are returned in a string
  list.

  \note If \a t is found in the metadata map, all the
  pairs having the key \a t are erased. i.e. Once you
  all this function for a particular \a t, you consume
  \a t.
 */
QStringList DitaXmlGenerator::getMetadataElements(const InnerNode* inner,
                                                  DitaXmlGenerator::DitaTag t)
{
    QStringList s = Generator::getMetadataElements(inner,ditaTags[t]);
    if (s.isEmpty())
        s.append(metadataDefault(t));
    return s;
}

/*!
  Returns the value of key \a t or an empty string
  if \a t is not found in the map.
 */
QString DitaXmlGenerator::metadataDefault(DitaTag t) const
{
    return metadataDefaults.value(ditaTags[t]);
}

/*!
  Writes the <prolog> element for the \a inner node
  using the \a marker. The <prolog> element contains
  the <metadata> element, plus some others. This
  function writes one or more of these elements:

  \list
    \o <audience> *
    \o <author> *
    \o <brand> not used
    \o <category> *
    \o <compomnent> *
    \o <copyrholder> *
    \o <copyright> *
    \o <created> not used
    \o <copyryear> *
    \o <critdates> not used
    \o <keyword> not used
    \o <keywords> not used
    \o <metadata> *
    \o <othermeta> *
    \o <permissions> *
    \o <platform> not used
    \o <prodinfo> *
    \o <prodname> *
    \o <prolog> *
    \o <publisher> *
    \o <resourceid> not used
    \o <revised> not used
    \o <source> not used
    \o <tm> not used
    \o <unknown> not used
    \o <vrm> *
    \o <vrmlist> *
  \endlist

  \node * means the tag has been used.

 */
void
DitaXmlGenerator::writeProlog(const InnerNode* inner)
{
    if (!inner)
        return;
    writeStartTag(DT_prolog);
    writeMetadataElements(inner,DT_author);
    writeMetadataElement(inner,DT_publisher);
    QString s = getMetadataElement(inner,DT_copyryear);
    QString t = getMetadataElement(inner,DT_copyrholder);
    writeStartTag(DT_copyright);
    writeStartTag(DT_copyryear);
    if (!s.isEmpty())
        xmlWriter().writeAttribute("year",s);
    writeEndTag(); // </copyryear>
    writeStartTag(DT_copyrholder);
    if (!s.isEmpty())
        xmlWriter().writeCharacters(t);
    writeEndTag(); // </copyrholder>
    writeEndTag(); // </copyright>
    s = getMetadataElement(inner,DT_permissions);
    writeStartTag(DT_permissions);
    xmlWriter().writeAttribute("view",s);
    writeEndTag(); // </permissions>
    writeStartTag(DT_metadata);
    QStringList sl = getMetadataElements(inner,DT_audience);
    if (!sl.isEmpty()) {
        for (int i=0; i<sl.size(); ++i) {
            writeStartTag(DT_audience);
            xmlWriter().writeAttribute("type",sl[i]);
            writeEndTag(); // </audience>
        }
    }
    if (!writeMetadataElement(inner,DT_category,false)) {
        writeStartTag(DT_category);
        QString category = "Page";
        if (inner->type() == Node::Class)
            category = "Class reference";
        else if (inner->type() == Node::Namespace)
            category = "Namespace";
        else if (inner->type() == Node::Fake) {
            if (inner->subType() == Node::QmlClass)
                category = "QML Reference";
            else if (inner->subType() == Node::QmlBasicType)
                category = "QML Basic Type";
            else if (inner->subType() == Node::HeaderFile)
                category = "Header File";
            else if (inner->subType() == Node::Module)
                category = "Module";
            else if (inner->subType() == Node::File)
                category = "Example Source File";
            else if (inner->subType() == Node::Example)
                category = "Example";
            else if (inner->subType() == Node::Image)
                category = "Image";
            else if (inner->subType() == Node::Group)
                category = "Group";
            else if (inner->subType() == Node::Page)
                category = "Page";
            else if (inner->subType() == Node::ExternalPage)
                category = "External Page"; // Is this necessary?
        }
        xmlWriter().writeCharacters(category);
        writeEndTag(); // </category>
    }
    if (vrm.size() > 0) {
        writeStartTag(DT_prodinfo);
        if (!writeMetadataElement(inner,DT_prodname,false)) {
            writeStartTag(DT_prodname);
            xmlWriter().writeCharacters(projectDescription);
            writeEndTag(); // </prodname>
        }
        writeStartTag(DT_vrmlist);
        writeStartTag(DT_vrm);
        if (vrm.size() > 0)
            xmlWriter().writeAttribute("version",vrm[0]);
        if (vrm.size() > 1)
            xmlWriter().writeAttribute("release",vrm[1]);
        if (vrm.size() > 2)
            xmlWriter().writeAttribute("modification",vrm[2]);
        writeEndTag(); // <vrm>
        writeEndTag(); // <vrmlist>
        if (!writeMetadataElement(inner,DT_component,false)) {
            QString component = inner->moduleName();
            if (!component.isEmpty()) {
                writeStartTag(DT_component);
                xmlWriter().writeCharacters(component);
                writeEndTag(); // </component>
            }
        }
        writeEndTag(); // </prodinfo>
    }
    const QStringMultiMap& metaTagMap = inner->doc().metaTagMap();
    QMapIterator<QString, QString> i(metaTagMap);
    while (i.hasNext()) {
        i.next();
        writeStartTag(DT_othermeta);
        xmlWriter().writeAttribute("name",i.key());
        xmlWriter().writeAttribute("content",i.value());
        writeEndTag(); // </othermeta>
    }
    if ((tagStack.first() == DT_cxxClass && !inner->includes().isEmpty()) ||
        (inner->type() == Node::Fake && inner->subType() == Node::HeaderFile)) {
        writeStartTag(DT_othermeta);
        xmlWriter().writeAttribute("name","includeFile");
        QString text;
        QStringList::ConstIterator i = inner->includes().begin();
        while (i != inner->includes().end()) {
            if ((*i).startsWith("<") && (*i).endsWith(">"))
                text += *i;
            else
                text += "<" + *i + ">";
            ++i;
            if (i != inner->includes().end())
                text += "\n";
        }
        xmlWriter().writeAttribute("content",text);
        writeEndTag(); // </othermeta>
    }
    writeEndTag(); // </metadata>
    writeEndTag(); // </prolog>
}

/*!
  This function should be called to write the \a href attribute
  if the href could be an \e http or \e ftp link. If \a href is
  one or the other, a \e scope attribute is also writen, with
  value \e external.
 */
void DitaXmlGenerator::writeHrefAttribute(const QString& href)
{
    xmlWriter().writeAttribute("href", href);
    if (href.startsWith("http:") || href.startsWith("ftp:") ||
            href.startsWith("https:") || href.startsWith("mailto:"))
        xmlWriter().writeAttribute("scope", "external");
}

/*!
  Strips the markup tags from \a src, when we are trying to
  create an \e{id} attribute. Returns the stripped text.
 */
QString DitaXmlGenerator::stripMarkup(const QString& src) const
{
    QString text;
    const QChar charAt = '@';
    const QChar charSlash = '/';
    const QChar charLangle = '<';
    const QChar charRangle = '>';

    int n = src.size();
    int i = 0;
    while (i < n) {
        if (src.at(i) == charLangle) {
            ++i;
            if (src.at(i) == charAt || (src.at(i) == charSlash && src.at(i+1) == charAt)) {
                while (i < n && src.at(i) != charRangle)
                    ++i;
                ++i;
            }
            else {
                text += charLangle;
            }
        }
        else
            text += src.at(i++);
    }
    return text;
}


QT_END_NAMESPACE
