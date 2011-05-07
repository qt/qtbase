/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CUSTOMWIDGET_H
#define CUSTOMWIDGET_H

#include "extension.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QIcon>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QWidget;
class QDesignerFormEditorInterface;

class QDesignerCustomWidgetInterface
{
public:
    virtual ~QDesignerCustomWidgetInterface() {}

    virtual QString name() const = 0;
    virtual QString group() const = 0;
    virtual QString toolTip() const = 0;
    virtual QString whatsThis() const = 0;
    virtual QString includeFile() const = 0;
    virtual QIcon icon() const = 0;

    virtual bool isContainer() const = 0;

    virtual QWidget *createWidget(QWidget *parent) = 0;

    virtual bool isInitialized() const { return false; }
    virtual void initialize(QDesignerFormEditorInterface *core) { Q_UNUSED(core); }

    virtual QString domXml() const
    {
        return QString::fromUtf8("<widget class=\"%1\" name=\"%2\"/>")
            .arg(name()).arg(name().toLower());
    }

    virtual QString codeTemplate() const { return QString(); }
};
Q_DECLARE_INTERFACE(QDesignerCustomWidgetInterface, "com.trolltech.Qt.Designer.CustomWidget")


class QDesignerCustomWidgetCollectionInterface
{
public:
    virtual ~QDesignerCustomWidgetCollectionInterface() {}

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const = 0;
};
Q_DECLARE_INTERFACE(QDesignerCustomWidgetCollectionInterface,
    "com.trolltech.Qt.Designer.CustomWidgetCollection")

QT_END_NAMESPACE

QT_END_HEADER

#endif // CUSTOMWIDGET_H
