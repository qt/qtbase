// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtWidgets/QtWidgets>

#include <emscripten/val.h>
#include <emscripten.h>

class AppWindow : public QObject
{
Q_OBJECT
public:
    AppWindow() : m_layout(new QVBoxLayout(&m_loadFileUi)),
                  m_window(emscripten::val::global("window")),
                  m_showOpenFilePickerFunction(m_window["showOpenFilePicker"]),
                  m_showSaveFilePickerFunction(m_window["showSaveFilePicker"])
    {
        addWidget<QLabel>("Filename filter");

        const bool localFileApiAvailable =
            !m_showOpenFilePickerFunction.isUndefined() && !m_showSaveFilePickerFunction.isUndefined();

        m_useLocalFileApisCheckbox = addWidget<QCheckBox>("Use the window.showXFilePicker APIs");
        m_useLocalFileApisCheckbox->setEnabled(localFileApiAvailable);
        m_useLocalFileApisCheckbox->setChecked(localFileApiAvailable);

        m_filterEdit = addWidget<QLineEdit>("Images (*.png *.jpg);;PDF (*.pdf);;*.txt");

        auto* loadFile = addWidget<QPushButton>("Load File");

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

        QObject::connect(m_useLocalFileApisCheckbox, &QCheckBox::toggled, std::bind(&AppWindow::onUseLocalFileApisCheckboxToggled, this));

        QObject::connect(loadFile, &QPushButton::clicked, this, &AppWindow::onLoadClicked);

        QObject::connect(m_saveFile, &QPushButton::clicked, std::bind(&AppWindow::onSaveClicked, this));
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
        QFileDialog::getOpenFileContent(
            m_filterEdit->text(),
            std::bind(&AppWindow::onFileContentReady, this, std::placeholders::_1, std::placeholders::_2),
            &m_loadFileUi);
    }

    void onSaveClicked()
    {
        m_fileInfo->setText("Saving file... (no result information with current API)");
        QFileDialog::saveFileContent(m_fileContent, m_savedFileNameEdit->text());
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
    QLineEdit* m_filterEdit;
    QVBoxLayout *m_layout;
    QLabel* m_fileInfo;
    QLabel* m_fileHash;
    QLineEdit* m_savedFileNameEdit;
    QPushButton* m_saveFile;

    emscripten::val m_window;
    emscripten::val m_showOpenFilePickerFunction;
    emscripten::val m_showSaveFilePickerFunction;

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
