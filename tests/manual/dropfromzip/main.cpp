// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class DropArea : public QWidget
{
public:
    DropArea()
    {
        setAcceptDrops(true);
        auto *layout = new QVBoxLayout(this);
        auto *label = new QLabel(
            "Drag a file from inside an opened .zip in Explorer onto this window.\n"
            "Each mime type and its data length will be logged below.\n"
            "Watch the \"FileContents index=0\" line: 0 bytes means the bug is "
            "present, a non-zero size means the fix is working.", this);
        label->setWordWrap(true);
        m_log = new QPlainTextEdit(this);
        m_log->setReadOnly(true);
        layout->addWidget(label);
        layout->addWidget(m_log, 1);
    }

protected:
    void dragEnterEvent(QDragEnterEvent *e) override { e->acceptProposedAction(); }

    void dropEvent(QDropEvent *e) override
    {
        const QMimeData *md = e->mimeData();
        m_log->appendPlainText(QStringLiteral("--- drop ---"));
        const QStringList formats = md->formats();
        if (formats.isEmpty()) {
            m_log->appendPlainText(QStringLiteral("(no formats)"));
        } else {
            for (const QString &f : formats) {
                const QByteArray bytes = md->data(f);
                m_log->appendPlainText(
                    QStringLiteral("%1 : %2 bytes").arg(f).arg(bytes.size()));
            }
        }
        // CFSTR_FILECONTENTS is delivered per file index; the QTBUG-126980 bug
        // fires here when reading index 0 (TYMED_HGLOBAL returns S_OK + null
        // and the IStream fallback recovers the bytes).
        const QByteArray fc0 = md->data(
            QStringLiteral("application/x-qt-windows-mime;value=\"FileContents\";index=0"));
        m_log->appendPlainText(
            QStringLiteral("FileContents index=0 : %1 bytes").arg(fc0.size()));
        e->acceptProposedAction();
    }

private:
    QPlainTextEdit *m_log = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow w;
    w.setWindowTitle(QStringLiteral("QTBUG-126980 drop-from-zip reproducer"));
    w.setCentralWidget(new DropArea);
    w.resize(640, 480);
    w.show();
    return app.exec();
}
