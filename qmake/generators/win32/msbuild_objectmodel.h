// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MSBUILD_OBJECTMODEL_H
#define MSBUILD_OBJECTMODEL_H

#include "project.h"
#include "xmloutput.h"
#include "msvc_objectmodel.h"
#include <qlist.h>
#include <qstring.h>
#include <qmap.h>

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

    void addElement(const QString &filepath, const VCFilterFile &allInfo) override {
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

    void removeElements() override {
        ChildrenMap::ConstIterator it = children.constBegin();
        ChildrenMap::ConstIterator end = children.constEnd();
        for( ; it != end; it++) {
            (*it)->removeElements();
            delete it.value();
        }
        children.clear();
    }

    void generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &tagName, VCProject &tool,
                     const QString &filter) override;
    bool hasElements() override {
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

    void addElement(const QString &filepath, const VCFilterFile &allInfo) override {
        QString newKey(filepath);

        int index = pathIndex(filepath);
        if (index != -1)
            newKey = filepath.mid(index+1);

        // Key designed to sort files with same
        // name in different paths correctly
        children.insert(newKey + "\0" + allInfo.file, allInfo);
    }

    void removeElements() override {
        children.clear();
    }

    void generateXML(XmlOutput &xml, XmlOutput &xmlFilter, const QString &tagName, VCProject &proj,
                     const QString &filter) override;
    bool hasElements() override {
        return children.size() != 0;
    }
};

class VCXProjectWriter : public VCProjectWriter
{
public:
    void write(XmlOutput &, VCProjectSingleConfig &) override;
    void write(XmlOutput &, VCProject &) override;

    void write(XmlOutput &, const VCCLCompilerTool &) override;
    void write(XmlOutput &, const VCLinkerTool &) override;
    void write(XmlOutput &, const VCMIDLTool &) override;
    void write(XmlOutput &, const VCCustomBuildTool &) override;
    void write(XmlOutput &, const VCLibrarianTool &) override;
    void write(XmlOutput &, const VCResourceCompilerTool &) override;
    void write(XmlOutput &, const VCEventTool &) override;
    void write(XmlOutput &, const VCDeploymentTool &) override;
    void write(XmlOutput &, const VCWinDeployQtTool &) override;
    void write(XmlOutput &, const VCConfiguration &) override;
    void write(XmlOutput &, VCFilter &) override;

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
    static XmlOutput::xml_output attrTagToolsVersion(const VCConfiguration &config);

    friend class XTreeNode;
    friend class XFlatNode;
};

QT_END_NAMESPACE

#endif // MSVC_OBJECTMODEL_H
