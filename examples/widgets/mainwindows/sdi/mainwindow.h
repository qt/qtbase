// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTextEdit;
QT_END_NAMESPACE

//! [class definition with macro]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
//! [class definition with macro]
    explicit MainWindow(const QString &fileName);

    void tile(const QMainWindow *previous);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void updateRecentFileActions();
    void openRecentFile();
    void about();
    void documentWasModified();

private:
    enum { MaxRecentFiles = 5 };

    void init();
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    void openFile(const QString &fileName);
    void loadFile(const QString &fileName);
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    static QString strippedName(const QString &fullFileName);
    MainWindow *findMainWindow(const QString &fileName) const;

    QTextEdit *textEdit;

    QAction *recentFileActs[MaxRecentFiles];
    QAction *recentFileSeparator;
    QAction *recentFileSubMenuAct;

    QString curFile;
    bool isUntitled;
};

#endif
