// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef GLYPHRUNINSPECTOR_H
#define GLYPHRUNINSPECTOR_H

#include <QWidget>

class QTextLayout;
class QTabWidget;
class SingleGlyphRun;
class GlyphRunInspector : public QWidget
{
    Q_OBJECT
public:
    explicit GlyphRunInspector(QWidget *parent = nullptr);

    void updateLayout(QTextLayout *layout, int start, int length);

private slots:
    void updateVisualizationForTab();

signals:
    void updateBounds(const QRegion &region);

private:
    QTabWidget *m_tabWidget;
    QList<SingleGlyphRun *> m_content;
};

#endif // GLYPHRUNINSPECTOR_H
