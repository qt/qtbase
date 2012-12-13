/********************************************************************************
** Form generated from reading UI file 'addtorrentform.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ADDTORRENTFORM_H
#define ADDTORRENTFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AddTorrentFile
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QWidget *widget;
    QGridLayout *gridLayout;
    QLabel *label_4;
    QLineEdit *torrentFile;
    QLabel *label_2;
    QPushButton *browseTorrents;
    QLabel *label_5;
    QLabel *label_3;
    QLabel *label_6;
    QTextEdit *torrentContents;
    QLineEdit *destinationFolder;
    QLabel *announceUrl;
    QLabel *label;
    QPushButton *browseDestination;
    QLabel *label_7;
    QLabel *commentLabel;
    QLabel *creatorLabel;
    QLabel *sizeLabel;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *AddTorrentFile)
    {
        if (AddTorrentFile->objectName().isEmpty())
            AddTorrentFile->setObjectName(QStringLiteral("AddTorrentFile"));
        AddTorrentFile->resize(464, 385);
        AddTorrentFile->setSizeGripEnabled(false);
        AddTorrentFile->setModal(true);
        vboxLayout = new QVBoxLayout(AddTorrentFile);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        groupBox = new QGroupBox(AddTorrentFile);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        widget = new QWidget(groupBox);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(10, 40, 364, 33));
        gridLayout = new QGridLayout(groupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 6, 0, 1, 1);

        torrentFile = new QLineEdit(groupBox);
        torrentFile->setObjectName(QStringLiteral("torrentFile"));

        gridLayout->addWidget(torrentFile, 0, 1, 1, 2);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        browseTorrents = new QPushButton(groupBox);
        browseTorrents->setObjectName(QStringLiteral("browseTorrents"));
        browseTorrents->setDefault(true);

        gridLayout->addWidget(browseTorrents, 0, 3, 1, 1);

        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout->addWidget(label_5, 5, 0, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 4, 0, 1, 1);

        label_6 = new QLabel(groupBox);
        label_6->setObjectName(QStringLiteral("label_6"));

        gridLayout->addWidget(label_6, 2, 0, 1, 1);

        torrentContents = new QTextEdit(groupBox);
        torrentContents->setObjectName(QStringLiteral("torrentContents"));
        torrentContents->setFocusPolicy(Qt::NoFocus);
        torrentContents->setTabChangesFocus(true);
        torrentContents->setLineWrapMode(QTextEdit::NoWrap);
        torrentContents->setReadOnly(true);

        gridLayout->addWidget(torrentContents, 5, 1, 1, 3);

        destinationFolder = new QLineEdit(groupBox);
        destinationFolder->setObjectName(QStringLiteral("destinationFolder"));
        destinationFolder->setFocusPolicy(Qt::StrongFocus);

        gridLayout->addWidget(destinationFolder, 6, 1, 1, 2);

        announceUrl = new QLabel(groupBox);
        announceUrl->setObjectName(QStringLiteral("announceUrl"));

        gridLayout->addWidget(announceUrl, 1, 1, 1, 3);

        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        browseDestination = new QPushButton(groupBox);
        browseDestination->setObjectName(QStringLiteral("browseDestination"));

        gridLayout->addWidget(browseDestination, 6, 3, 1, 1);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QStringLiteral("label_7"));

        gridLayout->addWidget(label_7, 3, 0, 1, 1);

        commentLabel = new QLabel(groupBox);
        commentLabel->setObjectName(QStringLiteral("commentLabel"));

        gridLayout->addWidget(commentLabel, 3, 1, 1, 3);

        creatorLabel = new QLabel(groupBox);
        creatorLabel->setObjectName(QStringLiteral("creatorLabel"));

        gridLayout->addWidget(creatorLabel, 2, 1, 1, 3);

        sizeLabel = new QLabel(groupBox);
        sizeLabel->setObjectName(QStringLiteral("sizeLabel"));

        gridLayout->addWidget(sizeLabel, 4, 1, 1, 3);


        vboxLayout->addWidget(groupBox);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        spacerItem = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        okButton = new QPushButton(AddTorrentFile);
        okButton->setObjectName(QStringLiteral("okButton"));
        okButton->setEnabled(false);

        hboxLayout->addWidget(okButton);

        cancelButton = new QPushButton(AddTorrentFile);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));

        hboxLayout->addWidget(cancelButton);


        vboxLayout->addLayout(hboxLayout);

        QWidget::setTabOrder(torrentFile, browseTorrents);
        QWidget::setTabOrder(browseTorrents, torrentContents);
        QWidget::setTabOrder(torrentContents, destinationFolder);
        QWidget::setTabOrder(destinationFolder, browseDestination);
        QWidget::setTabOrder(browseDestination, okButton);
        QWidget::setTabOrder(okButton, cancelButton);

        retranslateUi(AddTorrentFile);
        QObject::connect(okButton, SIGNAL(clicked()), AddTorrentFile, SLOT(accept()));
        QObject::connect(cancelButton, SIGNAL(clicked()), AddTorrentFile, SLOT(reject()));

        QMetaObject::connectSlotsByName(AddTorrentFile);
    } // setupUi

    void retranslateUi(QDialog *AddTorrentFile)
    {
        AddTorrentFile->setWindowTitle(QApplication::translate("AddTorrentFile", "Add a torrent", 0));
        groupBox->setTitle(QApplication::translate("AddTorrentFile", "Select a torrent source", 0));
        label_4->setText(QApplication::translate("AddTorrentFile", "Destination:", 0));
        label_2->setText(QApplication::translate("AddTorrentFile", "Tracker URL:", 0));
        browseTorrents->setText(QApplication::translate("AddTorrentFile", "Browse", 0));
        label_5->setText(QApplication::translate("AddTorrentFile", "File(s):", 0));
        label_3->setText(QApplication::translate("AddTorrentFile", "Size:", 0));
        label_6->setText(QApplication::translate("AddTorrentFile", "Creator:", 0));
        announceUrl->setText(QApplication::translate("AddTorrentFile", "<none>", 0));
        label->setText(QApplication::translate("AddTorrentFile", "Torrent file:", 0));
        browseDestination->setText(QApplication::translate("AddTorrentFile", "Browse", 0));
        label_7->setText(QApplication::translate("AddTorrentFile", "Comment:", 0));
        commentLabel->setText(QApplication::translate("AddTorrentFile", "<none>", 0));
        creatorLabel->setText(QApplication::translate("AddTorrentFile", "<none>", 0));
        sizeLabel->setText(QApplication::translate("AddTorrentFile", "0", 0));
        okButton->setText(QApplication::translate("AddTorrentFile", "&OK", 0));
        cancelButton->setText(QApplication::translate("AddTorrentFile", "&Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class AddTorrentFile: public Ui_AddTorrentFile {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ADDTORRENTFORM_H
