/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets/QtWidgets>

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
    QTableView tv;
    QStandardItemModel m;
    m.setRowCount(500);
    m.setColumnCount(250);
    tv.setModel(&m);
    tv.setSelectionMode(QAbstractItemView::SingleSelection);
    // Comment in the line below to test selection with keyboard (space)
    // tv.setEditTriggers(QAbstractItemView::NoEditTriggers);
    SomeHandler handler(tv.horizontalHeader(), &tv);
    tv.horizontalHeader()->setDefaultSectionSize(30);
    tv.show();
    tv.horizontalHeader()->setSectionsMovable(true);
    tv.verticalHeader()->setSectionsMovable(true);
    app.exec();
}
#include "qheaderviewtest1.moc"
