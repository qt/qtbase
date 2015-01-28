/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include <QUndoGroup>
#include <QUndoStack>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QToolButton>
#include "document.h"
#include "mainwindow.h"
#include "commands.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    QWidget *w = documentTabs->widget(0);
    documentTabs->removeTab(0);
    delete w;

    connect(actionOpen, SIGNAL(triggered()), this, SLOT(openDocument()));
    connect(actionClose, SIGNAL(triggered()), this, SLOT(closeDocument()));
    connect(actionNew, SIGNAL(triggered()), this, SLOT(newDocument()));
    connect(actionSave, SIGNAL(triggered()), this, SLOT(saveDocument()));
    connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(actionRed, SIGNAL(triggered()), this, SLOT(setShapeColor()));
    connect(actionGreen, SIGNAL(triggered()), this, SLOT(setShapeColor()));
    connect(actionBlue, SIGNAL(triggered()), this, SLOT(setShapeColor()));
    connect(actionAddCircle, SIGNAL(triggered()), this, SLOT(addShape()));
    connect(actionAddRectangle, SIGNAL(triggered()), this, SLOT(addShape()));
    connect(actionAddTriangle, SIGNAL(triggered()), this, SLOT(addShape()));
    connect(actionRemoveShape, SIGNAL(triggered()), this, SLOT(removeShape()));
    connect(actionAddRobot, SIGNAL(triggered()), this, SLOT(addRobot()));
    connect(actionAddSnowman, SIGNAL(triggered()), this, SLOT(addSnowman()));
    connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(aboutQt()));

    connect(undoLimit, SIGNAL(valueChanged(int)), this, SLOT(updateActions()));
    connect(documentTabs, SIGNAL(currentChanged(int)), this, SLOT(updateActions()));

    actionOpen->setShortcut(QString("Ctrl+O"));
    actionClose->setShortcut(QString("Ctrl+W"));
    actionNew->setShortcut(QString("Ctrl+N"));
    actionSave->setShortcut(QString("Ctrl+S"));
    actionExit->setShortcut(QString("Ctrl+Q"));
    actionRemoveShape->setShortcut(QString("Del"));
    actionRed->setShortcut(QString("Alt+R"));
    actionGreen->setShortcut(QString("Alt+G"));
    actionBlue->setShortcut(QString("Alt+B"));
    actionAddCircle->setShortcut(QString("Alt+C"));
    actionAddRectangle->setShortcut(QString("Alt+L"));
    actionAddTriangle->setShortcut(QString("Alt+T"));

    m_undoGroup = new QUndoGroup(this);
    undoView->setGroup(m_undoGroup);
    undoView->setCleanIcon(QIcon(":/icons/ok.png"));

    QAction *undoAction = m_undoGroup->createUndoAction(this);
    QAction *redoAction = m_undoGroup->createRedoAction(this);
    undoAction->setIcon(QIcon(":/icons/undo.png"));
    redoAction->setIcon(QIcon(":/icons/redo.png"));
    menuShape->insertAction(menuShape->actions().at(0), undoAction);
    menuShape->insertAction(undoAction, redoAction);

    toolBar->addAction(undoAction);
    toolBar->addAction(redoAction);

    newDocument();
    updateActions();
};

void MainWindow::updateActions()
{
    Document *doc = currentDocument();
    m_undoGroup->setActiveStack(doc == 0 ? 0 : doc->undoStack());
    QString shapeName = doc == 0 ? QString() : doc->currentShapeName();

    actionAddRobot->setEnabled(doc != 0);
    actionAddSnowman->setEnabled(doc != 0);
    actionAddCircle->setEnabled(doc != 0);
    actionAddRectangle->setEnabled(doc != 0);
    actionAddTriangle->setEnabled(doc != 0);
    actionClose->setEnabled(doc != 0);
    actionSave->setEnabled(doc != 0 && !doc->undoStack()->isClean());
    undoLimit->setEnabled(doc != 0 && doc->undoStack()->count() == 0);

    if (shapeName.isEmpty()) {
        actionRed->setEnabled(false);
        actionGreen->setEnabled(false);
        actionBlue->setEnabled(false);
        actionRemoveShape->setEnabled(false);
    } else {
        Shape shape = doc->shape(shapeName);
        actionRed->setEnabled(shape.color() != Qt::red);
        actionGreen->setEnabled(shape.color() != Qt::green);
        actionBlue->setEnabled(shape.color() != Qt::blue);
        actionRemoveShape->setEnabled(true);
    }

    if (doc != 0) {
        int index = documentTabs->indexOf(doc);
        Q_ASSERT(index != -1);
        static const QIcon unsavedIcon(":/icons/filesave.png");
        documentTabs->setTabIcon(index, doc->undoStack()->isClean() ? QIcon() : unsavedIcon);

        if (doc->undoStack()->count() == 0)
            doc->undoStack()->setUndoLimit(undoLimit->value());
    }
}

void MainWindow::openDocument()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,
                            tr("File error"),
                            tr("Failed to open\n%1").arg(fileName));
        return;
    }
    QTextStream stream(&file);

    Document *doc = new Document();
    if (!doc->load(stream)) {
        QMessageBox::warning(this,
                            tr("Parse error"),
                            tr("Failed to parse\n%1").arg(fileName));
        delete doc;
        return;
    }

    doc->setFileName(fileName);
    addDocument(doc);
}

QString MainWindow::fixedWindowTitle(const Document *doc) const
{
    QString title = doc->fileName();

    if (title.isEmpty())
        title = tr("Unnamed");
    else
        title = QFileInfo(title).fileName();

    QString result;

    for (int i = 0; ; ++i) {
        result = title;
        if (i > 0)
            result += QString::number(i);

        bool unique = true;
        for (int j = 0; j < documentTabs->count(); ++j) {
            const QWidget *widget = documentTabs->widget(j);
            if (widget == doc)
                continue;
            if (result == documentTabs->tabText(j)) {
                unique = false;
                break;
            }
        }

        if (unique)
            break;
    }

    return result;
}

void MainWindow::addDocument(Document *doc)
{
    if (documentTabs->indexOf(doc) != -1)
        return;
    m_undoGroup->addStack(doc->undoStack());
    documentTabs->addTab(doc, fixedWindowTitle(doc));
    connect(doc, SIGNAL(currentShapeChanged(QString)), this, SLOT(updateActions()));
    connect(doc->undoStack(), SIGNAL(indexChanged(int)), this, SLOT(updateActions()));
    connect(doc->undoStack(), SIGNAL(cleanChanged(bool)), this, SLOT(updateActions()));

    setCurrentDocument(doc);
}

void MainWindow::setCurrentDocument(Document *doc)
{
    documentTabs->setCurrentWidget(doc);
}

Document *MainWindow::currentDocument() const
{
    return qobject_cast<Document*>(documentTabs->currentWidget());
}

void MainWindow::removeDocument(Document *doc)
{
    int index = documentTabs->indexOf(doc);
    if (index == -1)
        return;

    documentTabs->removeTab(index);
    m_undoGroup->removeStack(doc->undoStack());
    disconnect(doc, SIGNAL(currentShapeChanged(QString)), this, SLOT(updateActions()));
    disconnect(doc->undoStack(), SIGNAL(indexChanged(int)), this, SLOT(updateActions()));
    disconnect(doc->undoStack(), SIGNAL(cleanChanged(bool)), this, SLOT(updateActions()));

    if (documentTabs->count() == 0) {
        newDocument();
        updateActions();
    }
}

void MainWindow::saveDocument()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    for (;;) {
        QString fileName = doc->fileName();

        if (fileName.isEmpty())
            fileName = QFileDialog::getSaveFileName(this);
        if (fileName.isEmpty())
            break;

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this,
                                tr("File error"),
                                tr("Failed to open\n%1").arg(fileName));
            doc->setFileName(QString());
        } else {
            QTextStream stream(&file);
            doc->save(stream);
            doc->setFileName(fileName);

            int index = documentTabs->indexOf(doc);
            Q_ASSERT(index != -1);
            documentTabs->setTabText(index, fixedWindowTitle(doc));

            break;
        }
    }
}

void MainWindow::closeDocument()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    if (!doc->undoStack()->isClean()) {
        int button
            = QMessageBox::warning(this,
                            tr("Unsaved changes"),
                            tr("Would you like to save this document?"),
                            QMessageBox::Yes, QMessageBox::No);
        if (button == QMessageBox::Yes)
            saveDocument();
    }

    removeDocument(doc);
    delete doc;
}

void MainWindow::newDocument()
{
    addDocument(new Document());
}

static QColor randomColor()
{
    int r = (int) (3.0*(rand()/(RAND_MAX + 1.0)));
    switch (r) {
        case 0:
            return Qt::red;
        case 1:
            return Qt::green;
        default:
            break;
    }
    return Qt::blue;
}

static QRect randomRect(const QSize &s)
{
    QSize min = Shape::minSize;

    int left = (int) ((0.0 + s.width() - min.width())*(rand()/(RAND_MAX + 1.0)));
    int top = (int) ((0.0 + s.height() - min.height())*(rand()/(RAND_MAX + 1.0)));
    int width = (int) ((0.0 + s.width() - left - min.width())*(rand()/(RAND_MAX + 1.0))) + min.width();
    int height = (int) ((0.0 + s.height() - top - min.height())*(rand()/(RAND_MAX + 1.0))) + min.height();

    return QRect(left, top, width, height);
}

void MainWindow::addShape()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    Shape::Type type;

    if (sender() == actionAddCircle)
        type = Shape::Circle;
    else if (sender() == actionAddRectangle)
        type = Shape::Rectangle;
    else if (sender() == actionAddTriangle)
        type = Shape::Triangle;
    else return;

    Shape newShape(type, randomColor(), randomRect(doc->size()));
    doc->undoStack()->push(new AddShapeCommand(doc, newShape));
}

void MainWindow::removeShape()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    QString shapeName = doc->currentShapeName();
    if (shapeName.isEmpty())
        return;

    doc->undoStack()->push(new RemoveShapeCommand(doc, shapeName));
}

void MainWindow::setShapeColor()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    QString shapeName = doc->currentShapeName();
    if (shapeName.isEmpty())
        return;

    QColor color;

    if (sender() == actionRed)
        color = Qt::red;
    else if (sender() == actionGreen)
        color = Qt::green;
    else if (sender() == actionBlue)
        color = Qt::blue;
    else
        return;

    if (color == doc->shape(shapeName).color())
        return;

    doc->undoStack()->push(new SetShapeColorCommand(doc, shapeName, color));
}

void MainWindow::addSnowman()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    // Create a macro command using beginMacro() and endMacro()

    doc->undoStack()->beginMacro(tr("Add snowman"));
    doc->undoStack()->push(new AddShapeCommand(doc,
                            Shape(Shape::Circle, Qt::blue, QRect(51, 30, 97, 95))));
    doc->undoStack()->push(new AddShapeCommand(doc,
                            Shape(Shape::Circle, Qt::blue, QRect(27, 123, 150, 133))));
    doc->undoStack()->push(new AddShapeCommand(doc,
                            Shape(Shape::Circle, Qt::blue, QRect(11, 253, 188, 146))));
    doc->undoStack()->endMacro();
}

void MainWindow::addRobot()
{
    Document *doc = currentDocument();
    if (doc == 0)
        return;

    // Compose a macro command by explicitly adding children to a parent command

    QUndoCommand *parent = new QUndoCommand(tr("Add robot"));

    new AddShapeCommand(doc, Shape(Shape::Rectangle, Qt::green, QRect(115, 15, 81, 70)), parent);
    new AddShapeCommand(doc, Shape(Shape::Rectangle, Qt::green, QRect(82, 89, 148, 188)), parent);
    new AddShapeCommand(doc, Shape(Shape::Rectangle, Qt::green, QRect(76, 280, 80, 165)), parent);
    new AddShapeCommand(doc, Shape(Shape::Rectangle, Qt::green, QRect(163, 280, 80, 164)), parent);
    new AddShapeCommand(doc, Shape(Shape::Circle, Qt::blue, QRect(116, 25, 80, 50)), parent);
    new AddShapeCommand(doc, Shape(Shape::Rectangle, Qt::green, QRect(232, 92, 80, 127)), parent);
    new AddShapeCommand(doc, Shape(Shape::Rectangle, Qt::green, QRect(2, 92, 80, 125)), parent);

    doc->undoStack()->push(parent);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Undo"), tr("The Undo demonstration shows how to use the Qt Undo framework."));
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
