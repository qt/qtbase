/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef TREEWALKER_H
#define TREEWALKER_H

#include <qvector.h>

QT_BEGIN_NAMESPACE

class DomUI;
class DomLayoutDefault;
class DomLayoutFunction;
class DomTabStops;
class DomLayout;
class DomLayoutItem;
class DomWidget;
class DomSpacer;
class DomColor;
class DomColorGroup;
class DomPalette;
class DomFont;
class DomPoint;
class DomRect;
class DomSizePolicy;
class DomSize;
class DomDate;
class DomTime;
class DomDateTime;
class DomProperty;
class DomCustomWidgets;
class DomCustomWidget;
class DomAction;
class DomActionGroup;
class DomActionRef;
class DomImages;
class DomImage;
class DomItem;
class DomIncludes;
class DomInclude;
class DomString;
class DomResourcePixmap;
class DomResources;
class DomResource;
class DomConnections;
class DomConnection;
class DomConnectionHints;
class DomConnectionHint;
class DomScript;
class DomButtonGroups;
class DomButtonGroup;

struct TreeWalker
{
    Q_DISABLE_COPY_MOVE(TreeWalker)

    TreeWalker() = default;
    inline virtual ~TreeWalker() = default;

    virtual void acceptUI(DomUI *ui);
    virtual void acceptLayoutDefault(DomLayoutDefault *layoutDefault);
    virtual void acceptLayoutFunction(DomLayoutFunction *layoutFunction);
    virtual void acceptTabStops(DomTabStops *tabStops);
    virtual void acceptCustomWidgets(DomCustomWidgets *customWidgets);
    virtual void acceptCustomWidget(DomCustomWidget *customWidget);
    virtual void acceptLayout(DomLayout *layout);
    virtual void acceptLayoutItem(DomLayoutItem *layoutItem);
    virtual void acceptWidget(DomWidget *widget);
    virtual void acceptSpacer(DomSpacer *spacer);
    virtual void acceptColor(DomColor *color);
    virtual void acceptColorGroup(DomColorGroup *colorGroup);
    virtual void acceptPalette(DomPalette *palette);
    virtual void acceptFont(DomFont *font);
    virtual void acceptPoint(DomPoint *point);
    virtual void acceptRect(DomRect *rect);
    virtual void acceptSizePolicy(DomSizePolicy *sizePolicy);
    virtual void acceptSize(DomSize *size);
    virtual void acceptDate(DomDate *date);
    virtual void acceptTime(DomTime *time);
    virtual void acceptDateTime(DomDateTime *dateTime);
    virtual void acceptProperty(DomProperty *property);
    using DomWidgets = QVector<DomWidget *>;
    virtual void acceptIncludes(DomIncludes *includes);
    virtual void acceptInclude(DomInclude *incl);
    virtual void acceptAction(DomAction *action);
    virtual void acceptActionGroup(DomActionGroup *actionGroup);
    virtual void acceptActionRef(DomActionRef *actionRef);
    virtual void acceptConnections(DomConnections *connections);
    virtual void acceptConnection(DomConnection *connection);
    virtual void acceptConnectionHints(DomConnectionHints *connectionHints);
    virtual void acceptConnectionHint(DomConnectionHint *connectionHint);
    virtual void acceptButtonGroups(const DomButtonGroups *buttonGroups);
    virtual void acceptButtonGroup(const DomButtonGroup *buttonGroup);
};

QT_END_NAMESPACE

#endif // TREEWALKER_H
