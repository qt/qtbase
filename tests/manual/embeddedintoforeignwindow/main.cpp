/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "itemwindow.h"

#include <QtGui/QGuiApplication>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

#ifdef Q_OS_WIN
#  include <qpa/qplatformnativeinterface.h>
#  include <QtCore/QMetaObject>
#  include <QtCore/qt_windows.h>
#endif

#include <eventfilter.h> // diaglib
#include <qwindowdump.h>

#include <iostream>

QT_USE_NAMESPACE

static const char usage[] =
    "\nEmbeds a QWindow into a native foreign window passed on the command line.\n"
    "When no window ID is passed, a test window is created (Windows only).";

static QString windowTitle()
{
    return QLatin1String(QT_VERSION_STR) + QLatin1Char(' ') + QGuiApplication::platformName();
}

#ifdef Q_OS_WIN
// Helper to create a native test window (Windows)
static QString registerWindowClass(const QString &name)
{
    QString result;
    void *proc = DefWindowProc;
    QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();
    if (!QMetaObject::invokeMethod(ni, "registerWindowClass", Qt::DirectConnection,
                                   Q_RETURN_ARG(QString, result),
                                   Q_ARG(QString, name),
                                   Q_ARG(void *, proc))) {
        qWarning("registerWindowClass failed");
    }
    return result;
}

static HWND createNativeWindow(const QString &name)
{
    const HWND hwnd =
        CreateWindowEx(0, reinterpret_cast<const wchar_t *>(name.utf16()),
                       L"NativeWindow", WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                       0, 0, GetModuleHandle(NULL), NULL);

    if (!hwnd) {
        qErrnoWarning("Cannot create window \"%s\"", qPrintable(name));
        return 0;
    }

    const QString text = windowTitle() + QLatin1String(" 0x") + QString::number(quint64(hwnd), 16);
    SetWindowText(hwnd, reinterpret_cast<const wchar_t *>(text.utf16()));
    return hwnd;
}
#endif // Q_OS_WIN

// Helper functions for simple management of native windows.
static WId createNativeTestWindow()
{
    WId result = 0;
#ifdef Q_OS_WIN
    const QString className = registerWindowClass(QLatin1String("TestClass") + windowTitle());
    const HWND nativeWin = createNativeWindow(className);
    result = WId(nativeWin);
#else // Q_OS_WIN
    Q_UNIMPLEMENTED();
#endif
    return result;
}

static void showNativeWindow(WId wid)
{
#ifdef Q_OS_WIN
     ShowWindow(HWND(wid), SW_SHOW);
#else // Q_OS_WIN
    Q_UNUSED(wid)
    Q_UNIMPLEMENTED();
#endif
}

static void setFocusToNativeWindow(WId wid)
{
#ifdef Q_OS_WIN
     SetFocus(HWND(wid));
#else // Q_OS_WIN
    Q_UNUSED(wid)
    Q_UNIMPLEMENTED();
#endif
}

static void destroyNativeWindow(WId wid)
{
#ifdef Q_OS_WIN
     DestroyWindow(HWND(wid));
#else // Q_OS_WIN
    Q_UNUSED(wid)
    Q_UNIMPLEMENTED();
#endif
}

// Main test window to be embedded into foreign window with some buttons.
class EmbeddedTestWindow : public ItemWindow {
    Q_OBJECT
public:
    explicit EmbeddedTestWindow(QWindow *parent = nullptr) : ItemWindow(parent)
    {
        const int spacing = 10;
        const QSize buttonSize(100, 30);
        const int width = 3 * buttonSize.width() + 4 * spacing;

        QPoint pos(spacing, spacing);
        addItem(new TextItem(::windowTitle(), QRect(pos, QSize(width - 2 * spacing, buttonSize.height())),
                             Qt::white));

        pos.ry() += 2 * spacing + buttonSize.height();

        ButtonItem *mgi = new ButtonItem("Map to global", QRect(pos, buttonSize),
                                         QColor(Qt::yellow).lighter(), this);
        connect(mgi, &ButtonItem::clicked, this, &EmbeddedTestWindow::testMapToGlobal);
        addItem(mgi);

        pos.rx() += buttonSize.width() + spacing;
        ButtonItem *di = new ButtonItem("Dump Wins", QRect(pos, buttonSize),
                                        QColor(Qt::cyan).lighter(), this);
        connect(di, &ButtonItem::clicked, this, [] () { QtDiag::dumpAllWindows(); });
        addItem(di);

        pos.rx() += buttonSize.width() + spacing;
        ButtonItem *qi = new ButtonItem("Quit", QRect(pos, buttonSize),
                                        QColor(Qt::red).lighter(), this);
        qi->setShortcut(Qt::CTRL + Qt::Key_Q);
        connect(qi, &ButtonItem::clicked, qApp, &QCoreApplication::quit);
        addItem(qi);

        setBackground(Qt::lightGray);
        resize(width, pos.y() + buttonSize.height() + spacing);
    }

public slots:
    void testMapToGlobal();
};

void EmbeddedTestWindow::testMapToGlobal()
{
    const QPoint globalPos = mapToGlobal(QPoint(0,0));
    qDebug() << "mapToGlobal(QPoint(0,0)" << globalPos
        << "cursor at:"  << QCursor::pos();
}

struct EventFilterOption
{
    const char *name;
    const char *description;
    QtDiag::EventFilter::EventCategories categories;
};

EventFilterOption eventFilterOptions[] = {
{"mouse-events", "Dump mouse events.", QtDiag::EventFilter::MouseEvents},
{"keyboard-events", "Dump keyboard events.", QtDiag::EventFilter::KeyEvents},
{"state-events", "Dump state/focus change events.", QtDiag::EventFilter::StateChangeEvents | QtDiag::EventFilter::FocusEvents}
};

static inline bool isOptionSet(int argc, char *argv[], const char *option)
{
    return (argv + argc) !=
        std::find_if(argv + 1, argv + argc,
                     [option] (const char *arg) { return !qstrcmp(arg, option); });
}

int main(int argc, char *argv[])
{
    // Check for no scaling before QApplication is instantiated.
    if (isOptionSet(argc, argv, "-s"))
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));
    QGuiApplication::setApplicationDisplayName("Foreign Window Embedding Tester");

    QGuiApplication app(argc, argv);

    // Process command  line
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(QLatin1String(usage));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption noScalingDummy(QStringLiteral("s"),
                                      QStringLiteral("Disable High DPI scaling."));
    parser.addOption(noScalingDummy);
    const int eventFilterOptionCount = int(sizeof(eventFilterOptions) / sizeof(eventFilterOptions[0]));
    for (int i = 0; i < eventFilterOptionCount; ++i) {
        parser.addOption(QCommandLineOption(QLatin1String(eventFilterOptions[i].name),
                                            QLatin1String(eventFilterOptions[i].description)));
    }
    parser.addPositionalArgument(QStringLiteral("[windows]"), QStringLiteral("Window ID."));

    parser.process(QCoreApplication::arguments());

    QtDiag::EventFilter::EventCategories eventCategories = 0;
    for (int i = 0; i < eventFilterOptionCount; ++i) {
        if (parser.isSet(QLatin1String(eventFilterOptions[i].name)))
            eventCategories |= eventFilterOptions[i].categories;
    }
    if (eventCategories)
        app.installEventFilter(new QtDiag::EventFilter(eventCategories, &app));

    // Obtain foreign window to test with.
    WId testForeignWinId = 0;
    bool createdTestWindow = false;
    if (parser.positionalArguments().isEmpty()) {
        testForeignWinId = createNativeTestWindow();
        if (!testForeignWinId)
            parser.showHelp(-1);
        showNativeWindow(testForeignWinId);
        createdTestWindow = true;
    } else {
        bool ok;
        const QString &winIdArgument = parser.positionalArguments().constFirst();
        testForeignWinId = winIdArgument.toULongLong(&ok, 0);
        if (!ok) {
            std::cerr << "Invalid window id: \"" << qPrintable(winIdArgument) << "\"\n";
            return -1;
        }
    }
    if (!testForeignWinId)
        parser.showHelp(1);

    QWindow *foreignWindow = QWindow::fromWinId(testForeignWinId);
    if (!foreignWindow)
        return -2;
    foreignWindow->setObjectName("ForeignWindow");
    EmbeddedTestWindow *embeddedTestWindow = new EmbeddedTestWindow(foreignWindow);
    embeddedTestWindow->setObjectName("EmbeddedTestWindow");
    embeddedTestWindow->show();
    setFocusToNativeWindow(embeddedTestWindow->winId()); // Windows: Set keyboard focus.

    const int exitCode = app.exec();
    delete embeddedTestWindow;
    delete foreignWindow;
    if (createdTestWindow)
        destroyNativeWindow(testForeignWinId);
    return exitCode;
}

#include "main.moc"
