// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class QMenu;
class QPrinter;
QT_END_NAMESPACE

class TextEdit : public QMainWindow
{
    Q_OBJECT

public:
    TextEdit(QWidget *parent = nullptr);

    bool load(const QString &f);

public slots:
    void fileNew();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    enum class SaveContinuation { None, Close, Clear };

private slots:
    void fileOpen();
    void filePrint();
    void filePrintPreview();
    void filePrintPdf();

    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void textStyle(int styleIndex);
    void textColor();
    void underlineColor();
    void textAlign(QAction *a);
    void setChecked(bool checked);
    void indent();
    void unindent();

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

    void clipboardDataChanged();
    void about();

private:
    void setupFileActions();
    void setupEditActions();
    void setupTextActions();
    void maybeSave(SaveContinuation saveContinuation);
    void setCurrentFileName(const QString &fileName);
    void modifyIndentation(int amount);

    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);

    void fileSave(SaveContinuation continuation);
    void fileSaveAs(SaveContinuation continuation);
    void fileSaveComplete(SaveContinuation continuation);

    QAction *actionSave;
    QAction *actionTextBold;
    QAction *actionTextUnderline;
    QAction *actionTextItalic;
    QAction *actionTextColor;
    QAction *actionUnderlineColor;
    QAction *actionAlignLeft;
    QAction *actionAlignCenter;
    QAction *actionAlignRight;
    QAction *actionAlignJustify;
    QAction *actionIndentLess;
    QAction *actionIndentMore;
    QAction *actionToggleCheckState;
    QAction *actionUndo;
    QAction *actionRedo;
#ifndef QT_NO_CLIPBOARD
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
#endif

    QComboBox *comboStyle;
    QFontComboBox *comboFont;
    QComboBox *comboSize;

    QString fileName;
    QTextEdit *textEdit;

    bool closeAccepted = false;
};

#endif // TEXTEDIT_H
