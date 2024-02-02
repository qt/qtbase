// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "glyphruninspector.h"
#include "singleglyphrun.h"

#include <QTextLayout>
#include <QtWidgets>

GlyphRunInspector::GlyphRunInspector(QWidget *parent)
    : QWidget{parent}
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    m_tabWidget = new QTabWidget;
    layout->addWidget(m_tabWidget);

    connect(m_tabWidget,
            &QTabWidget::currentChanged,
            this,
            &GlyphRunInspector::updateVisualizationForTab);
}

void GlyphRunInspector::updateVisualizationForTab()
{
    int currentTab = m_tabWidget->currentIndex();
    SingleGlyphRun *gr = currentTab >= 0 && currentTab < m_content.size() ? m_content.at(currentTab) : nullptr;
    if (gr != nullptr)
        emit updateBounds(gr->bounds());
}

void GlyphRunInspector::updateLayout(QTextLayout *layout, int start, int length)
{
    QList<QGlyphRun> glyphRuns = layout->glyphRuns(start, length, QTextLayout::RetrieveAll);

    m_tabWidget->clear();
    qDeleteAll(m_content);
    m_content.clear();
    for (int i = 0; i < glyphRuns.size(); ++i) {
        SingleGlyphRun *w = new SingleGlyphRun(m_tabWidget);
        w->updateGlyphRun(glyphRuns.at(i));

        m_tabWidget->addTab(w, QStringLiteral("%1").arg(glyphRuns.at(i).rawFont().familyName()));
        m_content.append(w);
    }

    updateVisualizationForTab();
}
