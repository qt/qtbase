// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>
#include <QtWidgets>

#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

class FontViewer : public QWidget
{
public:
    FontViewer() {
        QTextEdit *edit = new QTextEdit;
        edit->setPlainText("The quick brown fox jumps over the lazy dog\nHow quickly daft jumping zebras vex\nPack my box with five dozen liquor jugs");

        QComboBox *combo = new QComboBox;
        combo->addItems(QFontDatabase::families());

        connect(combo, &QComboBox::currentTextChanged, [=](const QString &family) {
            QFont font(family);
            edit->setFont(font);
        });

        QObject::connect(qApp, &QGuiApplication::fontDatabaseChanged, [=]() {
            QStringList families = QFontDatabase::families();
            combo->clear();
            combo->addItems(families);
        });

        QLayout *layout = new QVBoxLayout;
        layout->addWidget(edit);
        layout->addWidget(combo);
        setLayout(layout);
    }
};

FontViewer *g_viewer = nullptr;
QApplication *g_app = nullptr;

void deleteapp() {
    delete g_viewer;
    delete g_app;
};

EMSCRIPTEN_BINDINGS(fonloading) {
    function("deleteapp", &deleteapp);
}

int main(int argc, char **argv)
{
    qDebug() << "C++ main: Creating application";
    g_app = new QApplication(argc, argv);

    // Make sure there is one call to fontFamiliesLoaded at startup,
    // even if no further fonts are loaded.
    QTimer::singleShot(0, [=]() {
        emscripten::val window = emscripten::val::global("window");
        window.call<void>("fontFamiliesLoaded", QFontDatabase::families().count());
    });

    g_viewer = new FontViewer();
    g_viewer->show();

    QObject::connect(g_app, &QGuiApplication::fontDatabaseChanged, [=]() {
        QStringList families = QFontDatabase::families();

        emscripten::val window = emscripten::val::global("window");

        window.call<void>("fontFamiliesLoaded", families.count());
        for (int i = 0; i < families.count(); ++i) {
            window.call<void>("fontFamilyLoaded", families[i].toStdString());
        }
    });
}

