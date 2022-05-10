// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include <QtWidgets>
#include <QtSql>
#include <QtXml>

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QSqlRelationalTableModel *albums, QDomDocument details,
           QFile *output, QWidget *parent = nullptr);

private slots:
    void revert();
    void submit();

private:
    int addNewAlbum(const QString &title, int artistId);
    int addNewArtist(const QString &name);
    void addTracks(int albumId, const QStringList &tracks);
    QDialogButtonBox *createButtons();
    QGroupBox *createInputWidgets();
    int findArtistId(const QString &artist);
    static int generateAlbumId();
    static int generateArtistId();
    void increaseAlbumCount(QModelIndex artistIndex);
    QModelIndex indexOfArtist(const QString &artist);

    QSqlRelationalTableModel *model;
    QDomDocument albumDetails;
    QFile *outputFile;

    QLineEdit *artistEditor;
    QLineEdit *titleEditor;
    QSpinBox *yearEditor;
    QLineEdit *tracksEditor;
};

#endif
