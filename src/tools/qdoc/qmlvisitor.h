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

#ifndef QMLVISITOR_H
#define QMLVISITOR_H

#include <qstring.h>
#include "qqmljsastvisitor_p.h"
#include "node.h"

QT_BEGIN_NAMESPACE

struct QmlPropArgs
{
    QString type_;
    QString module_;
    QString component_;
    QString name_;

    void clear() {
        type_.clear();
        module_.clear();
        component_.clear();
        name_.clear();
    }
};

class QmlDocVisitor : public QQmlJS::AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::QmlDocVisitor)

public:
    QmlDocVisitor(const QString &filePath,
                  const QString &code,
                  QQmlJS::Engine *engine,
                  const QSet<QString> &commands,
                  const QSet<QString> &topics);
    virtual ~QmlDocVisitor();

    bool visit(QQmlJS::AST::UiImport *import) Q_DECL_OVERRIDE;
    void endVisit(QQmlJS::AST::UiImport *definition) Q_DECL_OVERRIDE;

    bool visit(QQmlJS::AST::UiObjectDefinition *definition) Q_DECL_OVERRIDE;
    void endVisit(QQmlJS::AST::UiObjectDefinition *definition) Q_DECL_OVERRIDE;

    bool visit(QQmlJS::AST::UiPublicMember *member) Q_DECL_OVERRIDE;
    void endVisit(QQmlJS::AST::UiPublicMember *definition) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::UiObjectBinding *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::UiObjectBinding *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::UiArrayBinding *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UiArrayBinding *) Q_DECL_OVERRIDE;

    bool visit(QQmlJS::AST::IdentifierPropertyName *idproperty) Q_DECL_OVERRIDE;

    bool visit(QQmlJS::AST::FunctionDeclaration *) Q_DECL_OVERRIDE;
    void endVisit(QQmlJS::AST::FunctionDeclaration *) Q_DECL_OVERRIDE;

    bool visit(QQmlJS::AST::UiScriptBinding *) Q_DECL_OVERRIDE;
    void endVisit(QQmlJS::AST::UiScriptBinding *) Q_DECL_OVERRIDE;

    bool visit(QQmlJS::AST::UiQualifiedId *) Q_DECL_OVERRIDE;
    void endVisit(QQmlJS::AST::UiQualifiedId *) Q_DECL_OVERRIDE;

private:
    QString getFullyQualifiedId(QQmlJS::AST::UiQualifiedId *id);
    QQmlJS::AST::SourceLocation precedingComment(quint32 offset) const;
    bool applyDocumentation(QQmlJS::AST::SourceLocation location, Node *node);
    void applyMetacommands(QQmlJS::AST::SourceLocation location, Node* node, Doc& doc);
    bool splitQmlPropertyArg(const Doc& doc,
                             const QString& arg,
                             QmlPropArgs& qpa);

    QQmlJS::Engine *engine;
    quint32 lastEndOffset;
    quint32 nestingLevel;
    QString filePath_;
    QString name;
    QString document;
    ImportList importList;
    QSet<QString> commands_;
    QSet<QString> topics_;
    QSet<quint32> usedComments;
    InnerNode *current;
};

QT_END_NAMESPACE

#endif
