/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>

class Dialog : public QDialog
{
public:
    Dialog()
    {
        QString localFile("test.html");
        // server/shared/test.html should be replaced to point to a real file
        QString UNCPath("file://server/shared/test.html");

        QVBoxLayout* vBox = new QVBoxLayout();
        vBox->addWidget(new QLabel("Clicking on the links should open their"
                                   " contents in the default browser !"));
        vBox->addWidget(createLink(localFile));
        vBox->addWidget(new QLabel("The following link must point to "
                                   "a file in a shared folder on a network !"));
        vBox->addWidget(createLink(UNCPath));
        setLayout(vBox);
    }

protected:
    QLabel* createLink(QString path)
    {
        QLabel *label = new QLabel();
        label->setTextFormat(Qt::RichText);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setOpenExternalLinks(true);

        QString link("<a href=" + path + QLatin1Char('>') + path + "</a>");

        label->setText(link);
        return label;
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Dialog dlg;
    dlg.show();

    return app.exec();
}
