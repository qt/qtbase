// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "filterwidget.h"

#include <QIcon>
#include <QPixmap>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QWidgetAction>

FilterWidget::FilterWidget(QWidget *parent)
    : QLineEdit(parent)
    , m_patternGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);
    connect(this, &QLineEdit::textChanged, this, &FilterWidget::filterChanged);

    QMenu *menu = new QMenu(this);
    m_caseSensitivityAction = menu->addAction(tr("Case Sensitive"));
    m_caseSensitivityAction->setCheckable(true);
    connect(m_caseSensitivityAction, &QAction::toggled, this, &FilterWidget::filterChanged);

    menu->addSeparator();
    m_patternGroup->setExclusive(true);
    QAction *patternAction = menu->addAction("Regular Expression");
    patternAction->setCheckable(true);
    patternAction->setChecked(true);
    patternAction->setData(QVariant(int(RegularExpression)));
    m_patternGroup->addAction(patternAction);
    patternAction = menu->addAction("Wildcard");
    patternAction->setCheckable(true);
    patternAction->setData(QVariant(int(Wildcard)));
    m_patternGroup->addAction(patternAction);
    patternAction = menu->addAction("Fixed String");
    patternAction->setData(QVariant(int(FixedString)));
    patternAction->setCheckable(true);
    m_patternGroup->addAction(patternAction);
    connect(m_patternGroup, &QActionGroup::triggered, this, &FilterWidget::filterChanged);

    const QIcon icon = QIcon(QPixmap(":/images/find.png"));
    QToolButton *optionsButton = new QToolButton;
#ifndef QT_NO_CURSOR
    optionsButton->setCursor(Qt::ArrowCursor);
#endif
    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setStyleSheet("* { border: none; }");
    optionsButton->setIcon(icon);
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
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
