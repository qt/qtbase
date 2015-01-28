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

#ifndef QMLMARKUPVISITOR_H
#define QMLMARKUPVISITOR_H

#include <qstring.h>
#include "qqmljsastvisitor_p.h"
#include "node.h"
#include "tree.h"

QT_BEGIN_NAMESPACE

class QmlMarkupVisitor : public QQmlJS::AST::Visitor
{
public:
    enum ExtraType{
        Comment,
        Pragma
    };

    QmlMarkupVisitor(const QString &code,
                     const QList<QQmlJS::AST::SourceLocation> &pragmas,
                     QQmlJS::Engine *engine);
    virtual ~QmlMarkupVisitor();

    QString markedUpCode();

    virtual bool visit(QQmlJS::AST::UiImport *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::UiImport *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::UiPublicMember *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UiObjectDefinition *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::UiObjectInitializer *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::UiObjectInitializer *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::UiObjectBinding *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UiScriptBinding *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UiArrayBinding *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UiArrayMemberList *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UiQualifiedId *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::ThisExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::IdentifierExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::NullExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::TrueLiteral *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::FalseLiteral *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::NumericLiteral *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::StringLiteral *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::RegExpLiteral *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ArrayLiteral *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::ObjectLiteral *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::ObjectLiteral *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::ElementList *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::Elision *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::PropertyNameAndValue *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ArrayMemberExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::FieldMemberExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::NewMemberExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::NewExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ArgumentList *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::PostIncrementExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::PostDecrementExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::DeleteExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::VoidExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::TypeOfExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::PreIncrementExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::PreDecrementExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UnaryPlusExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::UnaryMinusExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::TildeExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::NotExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::BinaryExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ConditionalExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::Expression *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::Block *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::Block *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::VariableStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::VariableDeclarationList *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::VariableDeclaration *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::EmptyStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ExpressionStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::IfStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::DoWhileStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::WhileStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ForStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::LocalForStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ForEachStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::LocalForEachStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ContinueStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::BreakStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ReturnStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::WithStatement *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::CaseBlock *) Q_DECL_OVERRIDE;
    virtual void endVisit(QQmlJS::AST::CaseBlock *) Q_DECL_OVERRIDE;

    virtual bool visit(QQmlJS::AST::SwitchStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::CaseClause *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::DefaultClause *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::LabelledStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::ThrowStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::TryStatement *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::Catch *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::Finally *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::FunctionDeclaration *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::FunctionExpression *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::FormalParameterList *) Q_DECL_OVERRIDE;
    virtual bool visit(QQmlJS::AST::DebuggerStatement *) Q_DECL_OVERRIDE;

protected:
    QString protect(const QString &string);

private:
    typedef QHash<QString, QString> StringHash;
    void addExtra(quint32 start, quint32 finish);
    void addMarkedUpToken(QQmlJS::AST::SourceLocation &location,
                          const QString &text,
                          const StringHash &attributes = StringHash());
    void addVerbatim(QQmlJS::AST::SourceLocation first,
                     QQmlJS::AST::SourceLocation last = QQmlJS::AST::SourceLocation());
    QString sourceText(QQmlJS::AST::SourceLocation &location);

    QQmlJS::Engine *engine;
    QList<ExtraType> extraTypes;
    QList<QQmlJS::AST::SourceLocation> extraLocations;
    QString source;
    QString output;
    quint32 cursor;
    int extraIndex;
};

QT_END_NAMESPACE

#endif
