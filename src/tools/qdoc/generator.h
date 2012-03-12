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
  generator.h
*/

#ifndef GENERATOR_H
#define GENERATOR_H

#include <qlist.h>
#include <qmap.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>

#include "node.h"
#include "text.h"

QT_BEGIN_NAMESPACE

typedef QMap<QString, const Node*> NodeMap;
typedef QMultiMap<QString, Node*> NodeMultiMap;
typedef QMap<QString, NodeMultiMap> NewSinceMaps;
typedef QMap<Node*, NodeMultiMap> ParentMaps;
typedef QMap<QString, NodeMap> NewClassMaps;

class ClassNode;
class Config;
class CodeMarker;
class FakeNode;
class FunctionNode;
class InnerNode;
class Location;
class NamespaceNode;
class Node;
class Tree;

class Generator
{
public:
    Generator();
    virtual ~Generator();

    virtual void initializeGenerator(const Config &config);
    virtual void terminateGenerator();
    virtual QString format() = 0;
    virtual bool canHandleFormat(const QString &format) { return format == this->format(); }
    virtual void generateTree(const Tree *tree) = 0;

    static void initialize(const Config& config);
    static void terminate();
    static Generator *generatorForFormat(const QString& format);
    static const QString& outputDir() { return outDir_; }
    static const QString& baseDir() { return baseDir_; }

protected:
    virtual void startText(const Node *relative, CodeMarker *marker);
    virtual void endText(const Node *relative, CodeMarker *marker);
    virtual int generateAtom(const Atom *atom,
                             const Node *relative,
                             CodeMarker *marker);
    virtual void generateClassLikeNode(const InnerNode *inner, CodeMarker *marker);
    virtual void generateFakeNode(const FakeNode *fake, CodeMarker *marker);

    virtual bool generateText(const Text& text,
                              const Node *relative,
                              CodeMarker *marker);
    virtual bool generateQmlText(const Text& text,
                                 const Node *relative,
                                 CodeMarker *marker,
                                 const QString& qmlName);
    virtual void generateQmlInherits(const QmlClassNode* qcn, CodeMarker* marker);
    virtual void generateBody(const Node *node, CodeMarker *marker);
    virtual void generateAlsoList(const Node *node, CodeMarker *marker);
    virtual void generateMaintainerList(const InnerNode* node, CodeMarker* marker);
    virtual void generateInherits(const ClassNode *classe,
                                  CodeMarker *marker);
    virtual void generateInheritedBy(const ClassNode *classe,
                                     CodeMarker *marker);

    void generateThreadSafeness(const Node *node, CodeMarker *marker);
    void generateSince(const Node *node, CodeMarker *marker);
    void generateStatus(const Node *node, CodeMarker *marker);
    const Atom* generateAtomList(const Atom *atom,
                                 const Node *relative,
                                 CodeMarker *marker,
                                 bool generate,
                                 int& numGeneratedAtoms);
    void generateFileList(const FakeNode* fake,
                          CodeMarker* marker,
                          Node::SubType subtype,
                          const QString& tag);
    void generateExampleFiles(const FakeNode *fake, CodeMarker *marker);

    virtual int skipAtoms(const Atom *atom, Atom::Type type) const;
    virtual QString fullName(const Node *node,
                             const Node *relative,
                             CodeMarker *marker) const;

    virtual QString outFileName() { return QString(); }

    QString indent(int level, const QString& markedCode);
    QString plainCode(const QString& markedCode);
    virtual QString typeString(const Node *node);
    virtual QString imageFileName(const Node *relative, const QString& fileBase);
    void setImageFileExtensions(const QStringList& extensions);
    void unknownAtom(const Atom *atom);
    QMap<QString, QString> &formattingLeftMap();
    QMap<QString, QString> &formattingRightMap();
    QMap<QString, QStringList> editionModuleMap;
    QMap<QString, QStringList> editionGroupMap;

    static QString trimmedTrailing(const QString &string);
    static bool matchAhead(const Atom *atom, Atom::Type expectedAtomType);
    static void supplementAlsoList(const Node *node, QList<Text> &alsoList);
    static QString outputPrefix(const QString &nodeType);

    QString getMetadataElement(const InnerNode* inner, const QString& t);
    QStringList getMetadataElements(const InnerNode* inner, const QString& t);
    void findAllSince(const InnerNode *node);

private:
    void generateReimplementedFrom(const FunctionNode *func,
                                   CodeMarker *marker);
    void appendFullName(Text& text,
                        const Node *apparentNode,
                        const Node *relative,
                        CodeMarker *marker,
                        const Node *actualNode = 0);
    void appendFullName(Text& text,
                        const Node *apparentNode,
                        const QString& fullName,
                        const Node *actualNode);
    void appendFullNames(Text& text,
                         const NodeList& nodes,
                         const Node* relative,
                         CodeMarker* marker);
    void appendSortedNames(Text& text,
                           const ClassNode *classe,
                           const QList<RelatedClass> &classes,
                           CodeMarker *marker);

protected:
    void appendSortedQmlNames(Text& text,
                              const Node* base,
                              const NodeList& subs,
                              CodeMarker *marker);

    static QString sinceTitles[];
    NewSinceMaps newSinceMaps;
    NewClassMaps newClassMaps;
    NewClassMaps newQmlClassMaps;

private:
    QString amp;
    QString lt;
    QString gt;
    QString quot;
    QRegExp tag;

    static QList<Generator *> generators;
    static QMap<QString, QMap<QString, QString> > fmtLeftMaps;
    static QMap<QString, QMap<QString, QString> > fmtRightMaps;
    static QMap<QString, QStringList> imgFileExts;
    static QSet<QString> outputFormats;
    static QStringList imageFiles;
    static QStringList imageDirs;
    static QStringList exampleDirs;
    static QStringList exampleImgExts;
    static QStringList scriptFiles;
    static QStringList scriptDirs;
    static QStringList styleFiles;
    static QStringList styleDirs;
    static QString outDir_;
    static QString baseDir_;
    static QString project;
    static QHash<QString, QString> outputPrefixes;
};

QT_END_NAMESPACE

#endif
