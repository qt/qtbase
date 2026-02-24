// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>
#include <QtWidgets>

#include <QtGui/qpa/qplatformwindow.h>

#include "nativewindow.h"

#if defined(Q_OS_MACOS)
#include <AppKit/AppKit.h>
#endif

#include <QDebug>

#define TEST_WINDOW_SIZE 150
#define WINDOW_PADDING 10

#define CHILD_WINDOW_POS (TEST_WINDOW_SIZE + (WINDOW_PADDING * 4))
#define CHILD_WINDOW_SIZE (TEST_WINDOW_SIZE + (WINDOW_PADDING * 2))

#define PARENT_WINDOW_SIZE ((TEST_WINDOW_SIZE * 2) + (WINDOW_PADDING * 7))

class ColorWindow : public QRasterWindow
{
public:
    using QRasterWindow::QRasterWindow;
    ColorWindow(const QBrush &brush) : m_brush(brush) {}

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(QRect(0, 0, width(), height()), m_brush);
    }

private:
    QBrush m_brush = QGradient(QGradient::DustyGrass);
};

class TestWindow : public ColorWindow
{
public:
    using ColorWindow::ColorWindow;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        ColorWindow::paintEvent(event);

        QPainter painter(this);
        painter.setPen(Qt::black);

        QFont font = painter.font();
        font.setPixelSize(12);
        painter.setFont(font);

        QRect geom = geometry();

        QStringList lines;
        lines << QString("%1,%2").arg(geom.x()).arg(geom.y());
        lines << QString("%1x%2").arg(geom.width()).arg(geom.height());
        lines << QString("Embedded: %1").arg(handle()->isEmbedded() ? "yes" : "no");
        lines << QString("Parent: %1").arg(QDebug::toString(QWindow::parent()));

        QString text = lines.join('\n');
        painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter, text);
    }

    void moveEvent(QMoveEvent *) override
    {
        update();
    }
};

class TopLevel : public ColorWindow
{
public:
    TopLevel(const QBrush &brush) : ColorWindow(brush)
    {
        resize(PARENT_WINDOW_SIZE, PARENT_WINDOW_SIZE);

        m_qtChildWindow = new ColorWindow{QGradient(QGradient::FebruaryInk)};
        m_qtChildWindow->setParent(this);
        m_qtChildWindow->setGeometry(WINDOW_PADDING, CHILD_WINDOW_POS, CHILD_WINDOW_SIZE, CHILD_WINDOW_SIZE);
        m_qtChildWindow->show();

        m_nativeChildWindowHandle.reset(new NativeWindow);
        m_nativeChildWindow.reset(QWindow::fromWinId(*m_nativeChildWindowHandle));
        m_nativeChildWindow->setParent(this);
        m_nativeChildWindow->setGeometry(CHILD_WINDOW_POS, CHILD_WINDOW_POS, CHILD_WINDOW_SIZE, CHILD_WINDOW_SIZE);
        m_nativeChildWindow->show();
    }

    ColorWindow *m_qtChildWindow = nullptr;

    std::unique_ptr<NativeWindow> m_nativeChildWindowHandle;
    std::unique_ptr<QWindow> m_nativeChildWindow;
};

class NativeWindowWithChildren : public NativeWindow
{
public:
    NativeWindowWithChildren() : NativeWindow()
    {
        m_selfAsForeignWindow.reset(QWindow::fromWinId(*this));
        m_qtChildWindow = new ColorWindow{QGradient(QGradient::FebruaryInk)};
        m_qtChildWindow->setObjectName("ChildOfNative");
        m_qtChildWindow->setParent(m_selfAsForeignWindow.get());
        m_qtChildWindow->setGeometry(WINDOW_PADDING, CHILD_WINDOW_POS, CHILD_WINDOW_SIZE, CHILD_WINDOW_SIZE);
        m_qtChildWindow->show();
    }

    std::unique_ptr<QWindow> m_selfAsForeignWindow;
    ColorWindow *m_qtChildWindow = nullptr;
};

class ControlWidget : public QWidget
{
    Q_OBJECT
public:
    ControlWidget() : QWidget()
    {
        setWindowTitle(tr("Hybrid UI Tester - Qt %1 (%2)")
            .arg(QLatin1String(qVersion()), qApp->platformName()));

        const int padding = WINDOW_PADDING;
        int xOffset = qGuiApp->primaryScreen()->availableGeometry().left() + padding;
        int yOffset = qGuiApp->primaryScreen()->availableGeometry().top() + padding;

        // FIXME: Why does the order of these matter?
        resize(250, 600);
        create();
        windowHandle()->setFramePosition(QPoint(xOffset, yOffset));

        m_qtWindowA.setTitle("Qt parent window A");
        m_qtWindowA.setFramePosition(QPoint(windowHandle()->frameGeometry().right() + padding, yOffset));
        m_qtWindowA.show();

        m_qtWindowB.setTitle("Qt parent window B");
        m_qtWindowB.setFramePosition(QPoint(m_qtWindowA.frameGeometry().right() + padding, yOffset));
        m_qtWindowB.show();

        m_nativeWindowHandle.reset(new NativeWindowWithChildren);
        m_nativeWindow.reset(QWindow::fromWinId(*m_nativeWindowHandle));
        m_nativeWindow->setGeometry(QRect(
            m_qtWindowA.frameGeometry().left(),
            m_qtWindowA.frameGeometry().bottom() + padding, PARENT_WINDOW_SIZE, PARENT_WINDOW_SIZE));
        m_nativeWindowHandle->setVisible(true);

#if defined(Q_OS_MACOS)
        m_contentViewWindow.reset(new NativeWindow);
        m_contentViewWindow->setGeometry(QRect(
            m_qtWindowA.frameGeometry().right() + padding,
            m_qtWindowA.frameGeometry().bottom() + padding, PARENT_WINDOW_SIZE, PARENT_WINDOW_SIZE));
        m_contentViewWindow->setVisible(true);
        m_contentWindow = m_contentViewWindow->handle().window;
#endif

        m_testWindow = new TestWindow{QGradient(QGradient::DustyGrass)};
        m_testWindow->setObjectName("TestWindow");
        m_testWindow->setGeometry(50, 800, TEST_WINDOW_SIZE, TEST_WINDOW_SIZE);

        setupControls();
    }

    ~ControlWidget()
    {
        delete m_testWindow;
    }

private slots:
    void onParentChanged()
    {
        auto newParent = [&]() -> QWindow* {
            switch (m_parentGroup->checkedId()) {
            // Top level
            case 0: return nullptr;
            // Qt window A
            case 10: return &m_qtWindowA;
            case 11: return m_qtWindowA.m_qtChildWindow;
            case 12: return m_qtWindowA.m_nativeChildWindow.get();
            // Qt window B
            case 20: return &m_qtWindowB;
            case 21: return m_qtWindowB.m_qtChildWindow;
            case 22: return m_qtWindowB.m_nativeChildWindow.get();
            // Native top level
            case 100: return m_nativeWindow.get();
            case 101: return m_nativeWindowHandle->m_qtChildWindow;
            default: return nullptr;
            }
        }();

        bool reparentUsingNativeApis = m_reparentMechanismGroup->checkedId() == 1
                                    && m_parentGroup->checkedId() != 0;

#if defined(Q_OS_MACOS)
        if (m_parentGroup->checkedId() == 500)
            reparentUsingNativeApis = true;
#endif

        if (!reparentUsingNativeApis) {
            m_testWindow->setFlags(m_testWindow->flags() & ~Qt::SubWindow);
            m_testWindow->setParent(newParent);
        } else {
            // We don't have QWSI parent change events, so we need to explicitly
            // move the window out of being a child window first.
            m_testWindow->setParent(nullptr);
            // Give the platform a hint that even though it's a parent-less
            // window it's not a top level.
            m_testWindow->setFlags(m_testWindow->flags() | Qt::SubWindow);

#if defined(Q_OS_MACOS)
            NSView *view = reinterpret_cast<NSView*>(m_testWindow->winId());
            if (m_parentGroup->checkedId() == 500) { // NSWindow content view
                m_contentWindow.contentView = view;
            } else if (newParent) {
                NSView *superview = reinterpret_cast<NSView*>(newParent->winId());
                [superview addSubview:view];
            } else {
                [view removeFromSuperview];
            }
#endif
        }

        // Moving from a top level to a child applies the window's global
        // position relative to its new parent, which will likely be outside
        // of the parent's bounds. FIXME: Can we do anything to improve this?
        m_testWindow->setGeometry(QRect(
            newParent ? QPoint(WINDOW_PADDING, WINDOW_PADDING) : QPoint(50, 800),
            QSize(TEST_WINDOW_SIZE, TEST_WINDOW_SIZE))
        );

        // FIXME: Moving to a top level on macOS also gives the top
        // level focus, stealing it from the control window. Is this
        // expected and does it behave the same cross platform?

        // Request update, to ensure we visualize the changes in the test window
        m_testWindow->update();
    }

private:
    void setupControls()
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        m_visibleCheckBox = new QCheckBox(tr("Visible"));
        m_visibleCheckBox->setChecked(false);
        mainLayout->addWidget(m_visibleCheckBox);

        QGroupBox *parentGroupBox = new QGroupBox(tr("Parent"));
        QVBoxLayout *parentLayout = new QVBoxLayout(parentGroupBox);

        m_parentGroup = new QButtonGroup(this);
        m_parentGroup->addButton(new QRadioButton(tr("Standalone top level")), 0);

        m_parentGroup->addButton(new QRadioButton(tr("Qt top level A")), 10);
        m_parentGroup->addButton(new QRadioButton(tr("\t\tQt child A")), 11);
        m_parentGroup->addButton(new QRadioButton(tr("\t\tNative child A")), 12);

        m_parentGroup->addButton(new QRadioButton(tr("Qt top level B")), 20);
        m_parentGroup->addButton(new QRadioButton(tr("\t\tQt child B")), 21);
        m_parentGroup->addButton(new QRadioButton(tr("\t\tNative child B")), 22);

        m_parentGroup->addButton(new QRadioButton(tr("Native top level A")), 100);
        m_parentGroup->addButton(new QRadioButton(tr("\t\tQt child A")), 101);

#if defined(Q_OS_MACOS)
        m_parentGroup->addButton(new QRadioButton(tr("NSWindow contentView")), 500);
#endif

        m_parentGroup->buttons().at(0)->setChecked(true);
        for (auto *button : m_parentGroup->buttons())
            parentLayout->addWidget(button);

        mainLayout->addWidget(parentGroupBox);

        QGroupBox *optionsGroupBox = new QGroupBox(tr("Reparent mechanism"));
        QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);

        m_reparentMechanismGroup = new QButtonGroup(this);
        m_reparentMechanismGroup->addButton(new QRadioButton(tr("fromWinId")), 0);
        m_reparentMechanismGroup->addButton(new QRadioButton(tr("winId")), 1);
        m_reparentMechanismGroup->buttons().at(0)->setChecked(true);
        for (auto *button : m_reparentMechanismGroup->buttons())
            optionsLayout->addWidget(button);

        mainLayout->addWidget(optionsGroupBox);

        mainLayout->addStretch();

        connect(m_parentGroup, &QButtonGroup::buttonClicked, this, &ControlWidget::onParentChanged);
        connect(m_visibleCheckBox, &QCheckBox::toggled, m_testWindow, &QWindow::setVisible);
    }

private:
    QPointer<TestWindow> m_testWindow;

    TopLevel m_qtWindowA{QGradient(QGradient::WinterNeva)};
    TopLevel m_qtWindowB{QGradient(QGradient::WinterNeva)};

    ColorWindow *m_qtChildWindowA = nullptr;
    ColorWindow *m_qtChildWindowB = nullptr;

    std::unique_ptr<NativeWindowWithChildren> m_nativeWindowHandle;
    std::unique_ptr<QWindow> m_nativeWindow;

#if defined(Q_OS_MACOS)
    std::unique_ptr<NativeWindow> m_contentViewWindow;
    NSWindow *m_contentWindow = nil;
#endif


    QButtonGroup *m_parentGroup;
    QButtonGroup *m_reparentMechanismGroup;
    QCheckBox *m_visibleCheckBox;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ControlWidget controlWidget;
    controlWidget.show();

    return app.exec();
}

#include "main.moc"
