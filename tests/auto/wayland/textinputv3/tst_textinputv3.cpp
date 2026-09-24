// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"
#include "textinputv3.h"

#include <QtGui/QRasterWindow>
#include <QtGui/QInputMethod>
#include <QtGui/qevent.h>

using namespace MockCompositor;

class TextInputCompositor : public DefaultCompositor
{
public:
    TextInputCompositor()
    {
        exec([this] { add<TextInputManagerV3>(); });
    }
    TextInputV3 *textInput() { return get<TextInputManagerV3>()->m_textInput; }
};

// A window that accepts input method events and reports the given hints, like a text field.
class InputWindow : public QRasterWindow
{
public:
    InputWindow() { resize(64, 64); }

    void setHints(Qt::InputMethodHints hints)
    {
        m_hints = hints;
        // The Wayland client drops updates that don't include the surrounding text
        QGuiApplication::inputMethod()->update(Qt::ImQueryAll);
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() != QEvent::InputMethodQuery)
            return QRasterWindow::event(event);

        auto *query = static_cast<QInputMethodQueryEvent *>(event);
        const Qt::InputMethodQueries queries = query->queries();
        if (queries & Qt::ImEnabled)
            query->setValue(Qt::ImEnabled, true);
        if (queries & Qt::ImHints)
            query->setValue(Qt::ImHints, int(m_hints));
        if (queries & Qt::ImSurroundingText)
            query->setValue(Qt::ImSurroundingText, QStringLiteral("surrounding text"));
        if (queries & Qt::ImCursorPosition)
            query->setValue(Qt::ImCursorPosition, 0);
        if (queries & Qt::ImAnchorPosition)
            query->setValue(Qt::ImAnchorPosition, 0);
        if (queries & Qt::ImCursorRectangle)
            query->setValue(Qt::ImCursorRectangle, QRect(0, 0, 1, 16));
        query->accept();
        return true;
    }

private:
    Qt::InputMethodHints m_hints = Qt::ImhNone;
};

class tst_textinputv3 : public QObject, private TextInputCompositor
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void enableOnEnter();
    void contentType_data();
    void contentType();

private:
    std::unique_ptr<InputWindow> m_window;
};

// Show a window and give it keyboard and text input focus, once for the whole test.
void tst_textinputv3::initTestCase()
{
    m_window = std::make_unique<InputWindow>();
    m_window->show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());
    exec([&] { xdgToplevel()->sendCompleteConfigure(); });
    QCOMPOSITOR_TRY_VERIFY(xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        Surface *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface);
        textInput()->sendEnter(surface);
    });
    QTRY_COMPARE(QGuiApplication::focusWindow(), m_window.get());
}

void tst_textinputv3::cleanupTestCase()
{
    m_window.reset();
    QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
}

void tst_textinputv3::enableOnEnter()
{
    QCOMPOSITOR_TRY_VERIFY(textInput()->m_committed.enabled);

    exec([&] { textInput()->sendLeave(xdgSurface()->m_surface); });
    QCOMPOSITOR_TRY_VERIFY(!textInput()->m_committed.enabled);

    exec([&] { textInput()->sendEnter(xdgSurface()->m_surface); });
    QCOMPOSITOR_TRY_VERIFY(textInput()->m_committed.enabled);
}

void tst_textinputv3::contentType_data()
{
    QTest::addColumn<Qt::InputMethodHints>("hints");
    QTest::addColumn<uint>("contentHint");
    QTest::addColumn<uint>("contentPurpose");

    using TI = TextInputV3;
    // The client requests these for a plain text field, at protocol version 2
    const uint plain = TI::content_hint_completion
                       | TI::content_hint_spellcheck
                       | TI::content_hint_auto_capitalization
                       | TI::content_hint_preedit_shown;

    QTest::newRow("hidden text")
        << Qt::InputMethodHints(Qt::ImhHiddenText)
        << uint(plain | TI::content_hint_hidden_text)
        << uint(TI::content_purpose_normal);

    QTest::newRow("no predictive text")
        << Qt::InputMethodHints(Qt::ImhNoPredictiveText)
        << uint(TI::content_hint_auto_capitalization | TI::content_hint_preedit_shown)
        << uint(TI::content_purpose_normal);

    QTest::newRow("digits")
        << Qt::InputMethodHints(Qt::ImhDigitsOnly)
        << plain
        << uint(TI::content_purpose_digits);

    QTest::newRow("none")
        << Qt::InputMethodHints(Qt::ImhNone)
        << plain
        << uint(TI::content_purpose_normal);
}

void tst_textinputv3::contentType()
{
    QFETCH(Qt::InputMethodHints, hints);
    QFETCH(uint, contentHint);
    QFETCH(uint, contentPurpose);

    QCOMPOSITOR_TRY_VERIFY(textInput()->m_committed.enabled);

    // Start from a state no row expects, so that every row has to be sent by the client
    m_window->setHints(Qt::ImhUrlCharactersOnly | Qt::ImhSensitiveData);
    QCOMPOSITOR_TRY_COMPARE(textInput()->m_committed.contentPurpose,
                            uint(TextInputV3::content_purpose_url));

    m_window->setHints(hints);
    QCOMPOSITOR_TRY_COMPARE(textInput()->m_committed.contentHint, contentHint);
    QCOMPOSITOR_TRY_COMPARE(textInput()->m_committed.contentPurpose, contentPurpose);
}

int main(int argc, char **argv)
{
    QTemporaryDir tmpRuntimeDir;
    qputenv("XDG_RUNTIME_DIR", tmpRuntimeDir.path().toLocal8Bit());
    qputenv("XDG_CURRENT_DESKTOP", "qtwaylandtests");
    qputenv("QT_QPA_PLATFORM", "wayland");
    // Let the platform plugin pick the input context from what the compositor offers
    qunsetenv("QT_IM_MODULE");
    qunsetenv("QT_WAYLAND_TEXT_INPUT_PROTOCOL");
    tst_textinputv3 tc;
    QGuiApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_textinputv3.moc"
