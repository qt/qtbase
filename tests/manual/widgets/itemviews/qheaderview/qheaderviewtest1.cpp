/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtWidgets/QtWidgets>

struct ManualTask {
    const char *title;
    const char *instructions;
    unsigned sectionsMovable : 1;
    unsigned selectionMode : 3;
};

ManualTask tasks[] = {
{ QT_TR_NOOP("0. Default"),
        "Please provide instructions",
        true, QAbstractItemView::SingleSelection
},
{ QT_TR_NOOP("1. Autoscroll"),
        "<ol>"
        "<li>Press and hold on section 9 of vertical header.<br/>"
          "<em>(all cells in the row will be selected)</em>"
        "</li>"
        "<li>Extend the selection by moving the mouse down.<br/>"
          "<em>(selection will extend to the next rows)</em>"
        "</li>"
        "<li>Continue to move the mouse down and outside the window geometry.<br/>"
          "<em>(The view should scroll automatically and the selection should still extend)</em>"
        "</li>"
        "<li>While still holding the button, do the same in the opposite direction, i.e. move mouse up and outside the window geometry.<br/>"
          "<em>(Verify that the view scrolls automatically and the selection changes)</em>"
        "</li>"
        "<li>Verify that it works in the other dimension, i.e Press and hold section 9 of the horizontal header.<br/>"
          "<em>All cells in the column will be selected</em>"
        "</li>"
        "<li>Extend the selection by moving the mouse to the far right and outside the window geometry.<br/>"
          "<em>(selection will extend to the next columns)</em>"
        "</li>"
        "<li>Verify that it works in the opposite direction (i.e. move mouse to the left of the window geometry).<br/>"
          "<em>(Verify that the view scrolls automatically and the selection changes)</em>"
        "</li>"
        "</ol>",
        false, QAbstractItemView::ExtendedSelection
}

};


class Window : public QWidget
{
    Q_OBJECT
public:
    Window(QWidget *parent = 0): QWidget(parent), ckMovable(0), tableView(0), cbSelectionMode(0), m_taskInstructions(0)
    {
        m_taskInstructions = new QLabel();
        if (sizeof(tasks) > 0)
            m_taskInstructions->setText(tr(tasks[0].instructions));

        QVBoxLayout *vbox = new QVBoxLayout(this);
        vbox->addLayout(setupComboBox());
        vbox->addWidget(setupGroupBox());
        vbox->addWidget(setupTableView());
        vbox->addWidget(m_taskInstructions);
    }

    void updateControls()
    {
        ckMovable->setChecked(tableView->verticalHeader()->sectionsMovable());
        QAbstractItemView::SelectionMode sMode = tableView->selectionMode();
        cbSelectionMode->setCurrentIndex((int)sMode);
    }

private:
    QFormLayout *setupComboBox()
    {
        QComboBox *combo = new QComboBox;
        for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); ++i) {
            combo->addItem(tr(tasks[i].title));
        }

        connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(on_taskCombo_currentIndexChanged(int)));
        QFormLayout *form = new QFormLayout;
        form->addRow(tr("Choose task:"), combo);
        return form;
    }

    QGroupBox *setupGroupBox()
    {
        QGroupBox *grp = new QGroupBox(tr("Properties"));
        QFormLayout *form = new QFormLayout;
        grp->setLayout(form);
        ckMovable = new QCheckBox;
        ckMovable->setObjectName(QLatin1String("ckMovable"));
        connect(ckMovable, SIGNAL(toggled(bool)), this, SLOT(on_ckMovable_toggled(bool)));
        form->addRow(tr("SectionsMovable"), ckMovable);

        cbSelectionMode = new QComboBox;
        cbSelectionMode->setObjectName(QLatin1String("cbSelectionMode"));
        cbSelectionMode->addItems(QStringList() << QLatin1String("NoSelection")
                                                << QLatin1String("SingleSelection")
                                                << QLatin1String("MultiSelection")
                                                << QLatin1String("ExtendedSelection")
                                                << QLatin1String("ContiguousSelection")
                                                );

        connect(cbSelectionMode, SIGNAL(currentIndexChanged(int)), this, SLOT(on_cbSelectionMode_currentIndexChanged(int)));
        form->addRow(tr("SelectionMode"), cbSelectionMode);
        return grp;
    }

    QTableView *setupTableView()
    {
        tableView = new QTableView;
        const int rowCount = 200;
        m.setRowCount(rowCount);
        m.setColumnCount(250);
        tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        tableView->setModel(&m);
        tableView->verticalHeader()->swapSections(rowCount - 1, 5);
        return tableView;
    }

private Q_SLOTS:
    void on_ckMovable_toggled(bool arg)
    {
        tableView->verticalHeader()->setSectionsMovable(arg);
        tableView->horizontalHeader()->setSectionsMovable(arg);
    }

    void on_cbSelectionMode_currentIndexChanged(int idx)
    {
        tableView->setSelectionMode((QAbstractItemView::SelectionMode)idx);
    }

    void on_taskCombo_currentIndexChanged(int idx)
    {
        ManualTask &task = tasks[idx];
        m_taskInstructions->setText(tr(task.instructions));
        ckMovable->setChecked(task.sectionsMovable);
        cbSelectionMode->setCurrentIndex((QAbstractItemView::SelectionMode)task.selectionMode);
    }

public:
    QCheckBox *ckMovable;
    QTableView *tableView;
    QStandardItemModel m;
    QComboBox *cbSelectionMode;
    QLabel *m_taskInstructions;
};

class SomeHandler : public QObject
{
    Q_OBJECT
    QHeaderView *m_hv;
    QTableView *m_tv;
public:
    SomeHandler(QHeaderView *hv, QTableView *tv);
public slots:
    void slotSectionResized(int, int, int);
};

SomeHandler::SomeHandler(QHeaderView *hv, QTableView *tv)
{
    m_hv = hv;
    m_tv = tv;
    m_tv->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(hv, SIGNAL(sectionResized(int,int,int)), this, SLOT(slotSectionResized(int,int,int)));
}
void SomeHandler::slotSectionResized(int logsection, int oldsize, int newsize)
{
    int offset = m_hv->offset();
    m_tv->setUpdatesEnabled(false);
    // Do some manual resizing - lets make every section having the new size.
    m_hv->blockSignals(true);
    m_hv->setDefaultSectionSize(newsize);
    m_hv->blockSignals(false);

    // Adjust offset and scrollbar. Maybe it isn't 100% perfect
    // but proof of concept
    // The test has sense without the define, too.
#define DO_CORRECT_OFFSET_AND_SB
#ifdef  DO_CORRECT_OFFSET_AND_SB
    int leftRemoved =  (m_hv->visualIndex(logsection)) * (oldsize - newsize);
    int newoffset = offset - leftRemoved;
    if (newoffset < 0)
        newoffset = 0;

    if (newoffset > 0 && newoffset >= m_hv->count() * newsize - m_tv->viewport()->width())
        m_hv->setOffsetToLastSection();
    else
        m_hv->setOffset(newoffset);

    m_tv->horizontalScrollBar()->blockSignals(true);
    m_tv->horizontalScrollBar()->setRange(0, m_hv->count() * newsize - m_tv->viewport()->width() );
    m_tv->horizontalScrollBar()->setValue(newoffset);
    m_tv->horizontalScrollBar()->blockSignals(false);
#endif
    m_tv->setUpdatesEnabled(true);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window window;
    // Comment in the line below to test selection with keyboard (space)
    // tv.setEditTriggers(QAbstractItemView::NoEditTriggers);
    QHeaderView *hHeader = window.tableView->horizontalHeader();
    QHeaderView *vHeader = window.tableView->verticalHeader();
    SomeHandler handler(hHeader, window.tableView);
    hHeader->setDefaultSectionSize(30);
    window.resize(600, 600);
    window.show();
    hHeader->setSectionsMovable(true);
    vHeader->setSectionsMovable(true);
    window.updateControls();
    app.exec();
}
#include "qheaderviewtest1.moc"
