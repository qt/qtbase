// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

using SearchBar = QWidget;
using Element = QWidget;

class Window : public QMainWindow
{
public:
    Window(QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;

private:
    Element *elementAt(QPoint pos) const {
        return nullptr;
    }

    QToolBar *fileToolBar;
    QMenu *fileMenu;

    SearchBar *searchBar;
};


Window::Window(QWidget *parent)
    : QMainWindow(parent)
{
//! [action_tooltip]
    QAction *openAction = new QAction(tr("&Open..."));
    openAction->setToolTip(tr("Open an existing file"));

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileToolBar = addToolBar(tr("&File"));

    fileMenu->addAction(openAction);
    fileToolBar->addAction(openAction);
//! [action_tooltip]

//! [static_tooltip]
    searchBar = new SearchBar;
    searchBar->setToolTip(tr("Search in the current document"));
//! [static_tooltip]

    fileToolBar->addWidget(searchBar);
}

//! [dynamic_tooltip]
bool Window::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        if (Element *element = elementAt(helpEvent->pos())) {
            QToolTip::showText(helpEvent->globalPos(), element->toolTip());
        } else {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}
//! [dynamic_tooltip]

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window w;
    return 0;
}
