// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "document.h"
#include "commands.h"

#include <QUndoGroup>
#include <QUndoStack>
#include <QFileDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTextStream>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    QWidget *w = documentTabs->widget(0);
    documentTabs->removeTab(0);
    delete w;

    connect(actionOpen, &QAction::triggered, this, &MainWindow::openDocument);
    connect(actionClose, &QAction::triggered, this, &MainWindow::closeDocument);
    connect(actionNew, &QAction::triggered, this, &MainWindow::newDocument);
    connect(actionSave, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(actionExit, &QAction::triggered, this, &QWidget::close);
    connect(actionRed, &QAction::triggered, this, &MainWindow::setShapeColor);
    connect(actionGreen, &QAction::triggered, this, &MainWindow::setShapeColor);
    connect(actionBlue, &QAction::triggered, this, &MainWindow::setShapeColor);
    connect(actionAddCircle, &QAction::triggered, this, &MainWindow::addShape);
    connect(actionAddRectangle, &QAction::triggered, this, &MainWindow::addShape);
    connect(actionAddTriangle, &QAction::triggered, this, &MainWindow::addShape);
    connect(actionRemoveShape, &QAction::triggered, this, &MainWindow::removeShape);
    connect(actionAddRobot, &QAction::triggered, this, &MainWindow::addRobot);
    connect(actionAddSnowman, &QAction::triggered, this, &MainWindow::addSnowman);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(actionAboutQt, &QAction::triggered, this, &MainWindow::aboutQt);

    connect(undoLimit, &QSpinBox::valueChanged, this, &MainWindow::updateActions);
    connect(documentTabs, &QTabWidget::currentChanged, this, &MainWindow::updateActions);

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
    m_undoGroup->setActiveStack(doc == nullptr ? nullptr : doc->undoStack());
    QString shapeName = doc == nullptr ? QString() : doc->currentShapeName();

    actionAddRobot->setEnabled(doc != nullptr);
    actionAddSnowman->setEnabled(doc != nullptr);
    actionAddCircle->setEnabled(doc != nullptr);
    actionAddRectangle->setEnabled(doc != nullptr);
    actionAddTriangle->setEnabled(doc != nullptr);
    actionClose->setEnabled(doc != nullptr);
    actionSave->setEnabled(doc != nullptr && !doc->undoStack()->isClean());
    undoLimit->setEnabled(doc != nullptr && doc->undoStack()->count() == 0);

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

    if (doc != nullptr) {
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
    connect(doc, &Document::currentShapeChanged, this, &MainWindow::updateActions);
    connect(doc->undoStack(), &QUndoStack::indexChanged, this, &MainWindow::updateActions);
    connect(doc->undoStack(), &QUndoStack::cleanChanged, this, &MainWindow::updateActions);

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
    disconnect(doc, &Document::currentShapeChanged, this, &MainWindow::updateActions);
    disconnect(doc->undoStack(), &QUndoStack::indexChanged, this, &MainWindow::updateActions);
    disconnect(doc->undoStack(), &QUndoStack::cleanChanged, this, &MainWindow::updateActions);

    if (documentTabs->count() == 0) {
        newDocument();
        updateActions();
    }
}

void MainWindow::saveDocument()
{
    Document *doc = currentDocument();
    if (doc == nullptr)
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
    if (doc == nullptr)
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
    int r = QRandomGenerator::global()->bounded(3);
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

    int left = qRound((s.width() - min.width()) * (QRandomGenerator::global()->bounded(1.0)));
    int top = qRound((s.height() - min.height()) * (QRandomGenerator::global()->bounded(1.0)));
    int width = qRound((s.width() - left - min.width()) * (QRandomGenerator::global()->bounded(1.0))) + min.width();
    int height = qRound((s.height() - top - min.height()) * (QRandomGenerator::global()->bounded(1.0))) + min.height();

    return QRect(left, top, width, height);
}

void MainWindow::addShape()
{
    Document *doc = currentDocument();
    if (doc == nullptr)
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
    if (doc == nullptr)
        return;

    QString shapeName = doc->currentShapeName();
    if (shapeName.isEmpty())
        return;

    doc->undoStack()->push(new RemoveShapeCommand(doc, shapeName));
}

void MainWindow::setShapeColor()
{
    Document *doc = currentDocument();
    if (doc == nullptr)
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
    if (doc == nullptr)
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
    if (doc == nullptr)
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
