// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "filterwidget.h"

#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidgetAction>

#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QIcon>

using namespace Qt::StringLiterals;

FilterWidget::FilterWidget(QWidget *parent)
    : QLineEdit(parent)
    , m_patternGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);
    connect(this, &QLineEdit::textChanged, this, &FilterWidget::filterChanged);

    auto *menu = new QMenu(this);
    m_caseSensitivityAction = menu->addAction(tr("Case Sensitive"));
    m_caseSensitivityAction->setCheckable(true);
    connect(m_caseSensitivityAction, &QAction::toggled, this, &FilterWidget::filterChanged);

    menu->addSeparator();
    m_patternGroup->setExclusive(true);
    QAction *patternAction = menu->addAction(tr("Regular Expression"));
    patternAction->setCheckable(true);
    patternAction->setChecked(true);
    patternAction->setData(QVariant(int(RegularExpression)));
    m_patternGroup->addAction(patternAction);
    patternAction = menu->addAction(tr("Wildcard"));
    patternAction->setCheckable(true);
    patternAction->setData(QVariant(int(Wildcard)));
    m_patternGroup->addAction(patternAction);
    patternAction = menu->addAction(tr("Fixed String"));
    patternAction->setData(QVariant(int(FixedString)));
    patternAction->setCheckable(true);
    m_patternGroup->addAction(patternAction);
    connect(m_patternGroup, &QActionGroup::triggered, this, &FilterWidget::filterChanged);

    const QIcon icon = QIcon(u":/images/find.png"_s);
    auto *optionsButton = new QToolButton;
#ifndef QT_NO_CURSOR
    optionsButton->setCursor(Qt::ArrowCursor);
#endif
    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setIcon(icon);
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    auto *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}

Qt::CaseSensitivity FilterWidget::caseSensitivity() const
{
    return m_caseSensitivityAction->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
}

void FilterWidget::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    m_caseSensitivityAction->setChecked(cs == Qt::CaseSensitive);
}

static inline FilterWidget::PatternSyntax patternSyntaxFromAction(const QAction *a)
{
    return static_cast<FilterWidget::PatternSyntax>(a->data().toInt());
}

FilterWidget::PatternSyntax FilterWidget::patternSyntax() const
{
    return patternSyntaxFromAction(m_patternGroup->checkedAction());
}

void FilterWidget::setPatternSyntax(PatternSyntax s)
{
    const QList<QAction*> actions = m_patternGroup->actions();
    for (QAction *a : actions) {
        if (patternSyntaxFromAction(a) == s) {
            a->setChecked(true);
            break;
        }
    }
}
