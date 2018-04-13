/****************************************************************************
 **
 ** Copyright (C) 2019 The Qt Company Ltd.
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

// Test that the size of the saved state bytearray does not change due to moving a
// toolbar from one area to another. It should stay the same size as the first time
// it had that toolbar in it

#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow() : QMainWindow()
    {
        auto *tb = new QToolBar(this);
        tb->setObjectName("Toolbar");
        tb->addAction("Test action");
        tb->addAction("Test action");
        addToolBar(Qt::TopToolBarArea, tb);
        auto *movableTb = new QToolBar(this);
        movableTb->setObjectName("Movable Toolbar");
        movableTb->addAction("Test action");
        movableTb->addAction("Test action");
        addToolBar(Qt::TopToolBarArea, movableTb);
        auto *widget = new QWidget;
        auto *vbox = new QVBoxLayout;
        auto *label = new QLabel;
        label->setText("1. Click on check state size to save initial state\n"
                       "2. Drag the movable toolbar in the top dock area to the left area."
                       " Click on check state size to save moved state\n"
                       "3. Drag the movable toolbar from the left dock area to the top area."
                       " Click on check state size to compare the state sizes.\n"
                       "4. Drag the movable toolbar in the top dock area to the left area."
                       " Click on check state size to compare the state sizes.\n"
                       "5. Drag the movable toolbar from the left dock area to the top area."
                       " Click on check state size to compare the state sizes.\n");
        vbox->addWidget(label);
        auto *pushButton = new QPushButton("Check state size");
        connect(pushButton, &QPushButton::clicked, this, &MainWindow::checkState);
        vbox->addWidget(pushButton);
        widget->setLayout(vbox);
        setCentralWidget(widget);
    }
public slots:
    void checkState()
    {
        stepCounter++;
        QString messageText;
        if (stepCounter == 1) {
            beforeMoveStateData = saveState();
            messageText = QLatin1String("Initial state saved");
        } else if (stepCounter == 2) {
            afterMoveStateData = saveState();
            messageText = QLatin1String("Moved state saved");
        } else {
            const int currentSaveSize = saveState().size();
            const int compareValue = (stepCounter == 4) ? afterMoveStateData.size() : beforeMoveStateData.size();
            messageText = QString::fromLatin1("%1 step %2")
                .arg((currentSaveSize == compareValue) ? QLatin1String("SUCCESS") : QLatin1String("FAIL"))
                .arg(stepCounter);
        }
        QMessageBox::information(this, "Step done", messageText);
    }
private:
    int stepCounter = 0;
    QByteArray beforeMoveStateData;
    QByteArray afterMoveStateData;
};

#include "main.moc"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    MainWindow mw;
    mw.show();
    return a.exec();
}

