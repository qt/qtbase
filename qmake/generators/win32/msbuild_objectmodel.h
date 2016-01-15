/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MSBUILD_OBJECTMODEL_H
#define MSBUILD_OBJECTMODEL_H

#include "project.h"
#include "xmloutput.h"
#include "msvc_objectmodel.h"
#include <qatomic.h>
#include <qlist.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

// Tree & Flat view of files --------------------------------------------------
class XNode
{
public:
    virtual ~XNode() { }
    void addElement(const VCFilterFile &file) {
        addElement(file.file, file);
    }
    virtual void addElement(const QString &filepath, const VCFilterFile &allInfo) = 0;
    virtual void removeElements()= 0;
    virtual void generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &tagName,
                             VCProject &tool, const QString &filter) = 0;
    virtual bool hasElements() = 0;
};

class XTreeNode : public XNode
{
    typedef QMap<QString, XTreeNode*> ChildrenMap;
    VCFilterFile info;
    ChildrenMap children;

public:
    virtual ~XTreeNode() { removeElements(); }

    int pathIndex(const QString &filepath) {
        int Windex = filepath.indexOf("\\");
        int Uindex = filepath.indexOf("/");
        if (Windex != -1 && Uindex != -1)
            return qMin(Windex, Uindex);
        else if (Windex != -1)
            return Windex;
        return Uindex;
    }

    void addElement(const QString &filepath, const VCFilterFile &allInfo){
        QString newNodeName(filepath);

        int index = pathIndex(filepath);
        if (index != -1)
            newNodeName = filepath.left(index);

        XTreeNode *n = children.value(newNodeName);
        if (!n) {
            n = new XTreeNode;
            n->info = allInfo;
            children.insert(newNodeName, n);
        }
        if (index != -1)
            n->addElement(filepath.mid(index+1), allInfo);
    }

    void removeElements() {
        ChildrenMap::ConstIterator it = children.constBegin();
        ChildrenMap::ConstIterator end = children.constEnd();
        for( ; it != end; it++) {
            (*it)->removeElements();
            delete it.value();
        }
        children.clear();
    }

    void generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &tagName, VCProject &tool,
                     const QString &filter);
    bool hasElements() {
        return children.size() != 0;
    }
};

class XFlatNode : public XNode
{
    typedef QMap<QString, VCFilterFile> ChildrenMapFlat;
    ChildrenMapFlat children;

public:
    virtual ~XFlatNode() { removeElements(); }

    int pathIndex(const QString &filepath) {
        int Windex = filepath.lastIndexOf("\\");
        int Uindex = filepath.lastIndexOf("/");
        if (Windex != -1 && Uindex != -1)
            return qMax(Windex, Uindex);
        else if (Windex != -1)
            return Windex;
        return Uindex;
    }

    void addElement(const QString &filepath, const VCFilterFile &allInfo){
        QString newKey(filepath);

        int index = pathIndex(filepath);
        if (index != -1)
            newKey = filepath.mid(index+1);

        // Key designed to sort files with same
        // name in different paths correctly
        children.insert(newKey + "\0" + allInfo.file, allInfo);
    }

    void removeElements() {
        children.clear();
    }

    void generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &tagName, VCProject &proj,
                     const QString &filter);
    bool hasElements() {
        return children.size() != 0;
    }
};

class VCXProjectWriter : public VCProjectWriter
{
public:
    void write(XmlOutput &, VCProjectSingleConfig &);
    void write(XmlOutput &, VCProject &);

    void write(XmlOutput &, const VCCLCompilerTool &);
    void write(XmlOutput &, const VCLinkerTool &);
    void write(XmlOutput &, const VCMIDLTool &);
    void write(XmlOutput &, const VCCustomBuildTool &);
    void write(XmlOutput &, const VCLibrarianTool &);
    void write(XmlOutput &, const VCResourceCompilerTool &);
    void write(XmlOutput &, const VCEventTool &);
    void write(XmlOutput &, const VCDeploymentTool &);
    void write(XmlOutput &, const VCWinDeployQtTool &);
    void write(XmlOutput &, const VCConfiguration &);
    void write(XmlOutput &, VCFilter &);

private:
    struct OutputFilterData
    {
        VCFilter filter;
        VCFilterFile info;
        bool inBuild;
    };

    static void addFilters(VCProject &project, XmlOutput &xmlFilter, const QString &filterName);
    static void outputFilter(VCProject &project, XmlOutput &xml, XmlOutput &xmlFilter, const QString &filtername);
    static void outputFileConfigs(VCProject &project, XmlOutput &xml, XmlOutput &xmlFilter,
                                  const VCFilterFile &info, const QString &filtername);
    static bool outputFileConfig(OutputFilterData *d, XmlOutput &xml, XmlOutput &xmlFilter,
                                 const QString &filename, const QString &fullFilterName,
                                 bool fileAdded, bool hasCustomBuildStep);
    static void outputFileConfig(XmlOutput &xml, XmlOutput &xmlFilter, const QString &fileName, const QString &filterName);
    static QString generateCondition(const VCConfiguration &config);

    friend class XTreeNode;
    friend class XFlatNode;
};

QT_END_NAMESPACE

#endif // MSVC_OBJECTMODEL_H
