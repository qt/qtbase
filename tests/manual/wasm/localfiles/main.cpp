// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtWidgets/QtWidgets>

#include <emscripten/val.h>
#include <emscripten.h>

class DropZone : public QLabel
{
    Q_OBJECT
public:
    explicit DropZone(QWidget *parent = nullptr) : QLabel(parent)
    {
        setAcceptDrops(true);
        setFrameStyle(QFrame::Box | QFrame::Sunken);
        setAlignment(Qt::AlignCenter);
        setText("Drop files here\n(will read first file)");
        setMinimumSize(400, 150);
        setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px dashed #999; padding: 20px; }");
    }

Q_SIGNALS:
    void filesDropped(const QList<QUrl> &urls);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
            setStyleSheet("QLabel { background-color: #e0f0ff; border: 2px dashed #0066cc; padding: 20px; }");
        }
    }

    void dragLeaveEvent(QDragLeaveEvent *event) override
    {
        Q_UNUSED(event);
        setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px dashed #999; padding: 20px; }");
    }

    void dropEvent(QDropEvent *event) override
    {
        const QMimeData *mimeData = event->mimeData();

        if (mimeData->hasUrls()) {
            QList<QUrl> urls = mimeData->urls();

            qDebug() << "=== Files Dropped ===";
            qDebug() << "Number of files:" << urls.size();

            for (int i = 0; i < urls.size(); ++i) {
                const QUrl &url = urls.at(i);
                qDebug() << "\n--- File" << (i + 1) << "---";
                qDebug() << "URL:" << url;
                qDebug() << "URL toString:" << url.toString();
                qDebug() << "URL scheme:" << url.scheme();
                qDebug() << "URL path:" << url.path();
                qDebug() << "URL fileName:" << url.fileName();
                qDebug() << "isLocalFile:" << url.isLocalFile();

                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    qDebug() << "Local file path:" << filePath;

                    QFileInfo fileInfo(filePath);
                    qDebug() << "File name:" << fileInfo.fileName();
                    qDebug() << "File size:" << fileInfo.size();
                    qDebug() << "File exists:" << fileInfo.exists();
                    qDebug() << "Is readable:" << fileInfo.isReadable();
                    qDebug() << "Absolute path:" << fileInfo.absoluteFilePath();
                    qDebug() << "Last modified:" << fileInfo.lastModified().toString();
                }
            }
            qDebug() << "===================\n";

            event->acceptProposedAction();
            emit filesDropped(urls);
        }

        setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px dashed #999; padding: 20px; }");
    }
};

class AppWindow : public QObject
{
Q_OBJECT
public:
    AppWindow() : m_layout(new QVBoxLayout(&m_loadFileUi)),
                  m_window(emscripten::val::global("window")),
                  m_showOpenFilePickerFunction(m_window["showOpenFilePicker"]),
                  m_showSaveFilePickerFunction(m_window["showSaveFilePicker"]),
                  m_fileDialog(new QFileDialog(&m_loadFileUi)),
                  m_isLoadOperation(true)
    {
        const bool localFileApiAvailable =
            !m_showOpenFilePickerFunction.isUndefined() && !m_showSaveFilePickerFunction.isUndefined();

        m_useLocalFileApisCheckbox = addWidget<QCheckBox>("Use the window.showXFilePicker APIs");
        m_useLocalFileApisCheckbox->setEnabled(localFileApiAvailable);
        m_useLocalFileApisCheckbox->setChecked(localFileApiAvailable);
        connect(m_useLocalFileApisCheckbox, &QCheckBox::toggled,
            std::bind(&AppWindow::onUseLocalFileApisCheckboxToggled, this));

        m_useStandardFileDialogCheckbox = addWidget<QCheckBox>("Use standard QFileDialog API");
        connect(m_useStandardFileDialogCheckbox, &QCheckBox::toggled,
            std::bind(&AppWindow::onUseStandardFileDialogCheckboxToggled, this));
        m_useStandardFileDialogCheckbox->setChecked(true);

        m_useExecModeCheckbox = addWidget<QCheckBox>("Use exec() instead of open()");
        m_useExecModeCheckbox->setChecked(false);

        addWidget<QLabel>("Filename filter");

        m_filterCombo = addWidget<QComboBox>();
        m_filterCombo->addItem("*");
        m_filterCombo->addItem("Images (*.png *.jpg);;PDF (*.pdf);;*.txt");
        m_filterCombo->setCurrentIndex(0); // Make "*" the default

        auto* loadFile = addWidget<QPushButton>("Load File");

        m_dropZone = addWidget<DropZone>();
        connect(m_dropZone, &DropZone::filesDropped, this, &AppWindow::onFilesDropped);

        m_fileInfo = addWidget<QLabel>("Opened file:");
        m_fileInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);

        m_fileHash = addWidget<QLabel>("Sha256:");
        m_fileHash->setTextInteractionFlags(Qt::TextSelectableByMouse);

        addWidget<QLabel>("Saved file name");
        m_savedFileNameEdit = addWidget<QLineEdit>("qttestresult");

        m_saveFile = addWidget<QPushButton>("Save File");
        m_saveFile->setEnabled(false);

        m_layout->addStretch();

        m_loadFileUi.setLayout(m_layout);


        QObject::connect(loadFile, &QPushButton::clicked, this, &AppWindow::onLoadClicked);
        QObject::connect(m_saveFile, &QPushButton::clicked, std::bind(&AppWindow::onSaveClicked, this));

        // Connect to both fileSelected and accepted signals for compatibility
        QObject::connect(m_fileDialog, &QFileDialog::fileSelected, this, &AppWindow::onFileSelected);
        QObject::connect(m_fileDialog, &QFileDialog::accepted, this, &AppWindow::onDialogAccepted);
    }

    void show() {
        m_loadFileUi.show();
    }

    ~AppWindow() = default;

private Q_SLOTS:
    void onUseLocalFileApisCheckboxToggled()
    {
        m_window.set("showOpenFilePicker",
            m_useLocalFileApisCheckbox->isChecked() ?
                m_showOpenFilePickerFunction : emscripten::val::undefined());
        m_window.set("showSaveFilePicker",
            m_useLocalFileApisCheckbox->isChecked() ?
                m_showSaveFilePickerFunction : emscripten::val::undefined());
    }

    void onUseStandardFileDialogCheckboxToggled()
    {
        m_useLocalFileApisCheckbox->setChecked(m_useStandardFileDialogCheckbox->isChecked());
    }

    void onFilesDropped(const QList<QUrl> &urls)
    {
        if (urls.isEmpty())
            return;

        // Load the first dropped file
        const QUrl &url = urls.first();

        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            loadFileWithQFile(filePath);
        } else {
            // Try using the URL string directly for non-file:// URLs (like weblocalfile://)
            QString urlString = url.toString();
            loadFileWithQFile(urlString);
        }
    }

    void onFileContentReady(const QString &fileName, const QByteArray &fileContents)
    {
        m_fileContent = fileContents;
        m_fileInfo->setText(QString("Opened file: %1 size: %2").arg(fileName).arg(fileContents.size()));
        m_saveFile->setEnabled(true);

        QTimer::singleShot(100, this, &AppWindow::computeAndDisplayFileHash); // update UI before computing hash
    }

    void computeAndDisplayFileHash()
    {
        QByteArray hash = QCryptographicHash::hash(m_fileContent, QCryptographicHash::Sha256);
        m_fileHash->setText(QString("Sha256: %1").arg(QString(hash.toHex())));
    }

    void onFileSaved(bool success)
    {
        m_fileInfo->setText(QString("File save result: %1").arg(success ? "success" : "failed"));
    }

    void onLoadClicked()
    {
        if (m_useStandardFileDialogCheckbox->isChecked()) {
            m_isLoadOperation = true;
            m_fileDialog->setFileMode(QFileDialog::ExistingFile);
            m_fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
            m_fileDialog->setNameFilter(m_filterCombo->currentText());
            m_fileDialog->setWindowTitle("Open File");

            if (m_useExecModeCheckbox->isChecked()) {
                qDebug() << "Using exec() mode";
                int result = m_fileDialog->exec();
                if (result == QDialog::Accepted) {
                    QStringList files = m_fileDialog->selectedFiles();
                    if (!files.isEmpty()) {
                        onFileSelected(files.first());
                    }
                }
            } else {
                qDebug() << "Using open() mode";
                m_fileDialog->open();
            }
        } else {
            QFileDialog::getOpenFileContent(
                m_filterCombo->currentText(),
                std::bind(&AppWindow::onFileContentReady, this, std::placeholders::_1, std::placeholders::_2),
                &m_loadFileUi);
        }
    }

    void onSaveClicked()
    {
        if (m_useStandardFileDialogCheckbox->isChecked()) {
            m_isLoadOperation = false;
            m_fileDialog->setFileMode(QFileDialog::AnyFile);
            m_fileDialog->setAcceptMode(QFileDialog::AcceptSave);
            m_fileDialog->setNameFilter(m_filterCombo->currentText());
            m_fileDialog->setWindowTitle("Save File");
            m_fileDialog->selectFile(m_savedFileNameEdit->text());

            if (m_useExecModeCheckbox->isChecked()) {
                qDebug() << "Using exec() mode for save";
                int result = m_fileDialog->exec();
                if (result == QDialog::Accepted) {
                    QStringList files = m_fileDialog->selectedFiles();
                    if (!files.isEmpty()) {
                        onFileSelected(files.first());
                    }
                }
            } else {
                qDebug() << "Using open() mode for save";
                m_fileDialog->open();
            }
        } else {
            m_fileInfo->setText("Saving file... (no result information with current API)");
            QFileDialog::saveFileContent(m_fileContent, m_savedFileNameEdit->text());
        }
    }

    void onDialogAccepted()
    {
        QStringList files = m_fileDialog->selectedFiles();
        if (!files.isEmpty()) {
            onFileSelected(files.first());
        }
    }

    void onFileSelected(const QString &fileName)
    {
        qDebug() << "onFileSelected" << fileName;

        if (m_isLoadOperation) {
            loadFileWithQFile(fileName);
        } else {
            saveFileWithQFile(fileName);
        }
    }

    void loadFileWithQFile(const QString &fileName)
    {
        qDebug() << "loadFileWithQFile" << fileName;

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            qDebug() << "loadFileWithQFile" << fileName;
            QByteArray fileContents = file.readAll();
            file.close();
            onFileContentReady(QFileInfo(fileName).fileName(), fileContents);
        } else {
            m_fileInfo->setText(QString("Failed to open file: %1").arg(file.errorString()));
        }
    }

    void saveFileWithQFile(const QString &fileName)
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            qint64 bytesWritten = file.write(m_fileContent);
            file.close();
            bool success = (bytesWritten == m_fileContent.size());
            m_fileInfo->setText(QString("File save result: %1").arg(success ? "success" : "failed"));
        } else {
            m_fileInfo->setText(QString("Failed to save file: %1").arg(file.errorString()));
        }
    }

private:
    template <class T, class... Args>
    T* addWidget(Args... args)
    {
        T* widget = new T(std::forward<Args>(args)..., &m_loadFileUi);
        m_layout->addWidget(widget);
        return widget;
    }

    QWidget m_loadFileUi;

    QCheckBox* m_useLocalFileApisCheckbox;
    QCheckBox* m_useStandardFileDialogCheckbox;
    QCheckBox* m_useExecModeCheckbox;
    DropZone* m_dropZone;
    QComboBox* m_filterCombo;
    QVBoxLayout *m_layout;
    QLabel* m_fileInfo;
    QLabel* m_fileHash;
    QLineEdit* m_savedFileNameEdit;
    QPushButton* m_saveFile;

    emscripten::val m_window;
    emscripten::val m_showOpenFilePickerFunction;
    emscripten::val m_showSaveFilePickerFunction;

    QFileDialog* m_fileDialog;
    bool m_isLoadOperation;

    QByteArray m_fileContent;
};

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    AppWindow window;
    window.show();
    return application.exec();
}

#include "main.moc"
