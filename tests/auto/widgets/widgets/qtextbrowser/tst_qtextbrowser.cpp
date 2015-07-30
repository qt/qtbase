/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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


#include <QtTest/QtTest>
#include <qtextbrowser.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <qscrollbar.h>

#include <qtextbrowser.h>
#include <qtextobject.h>

class TestBrowser : public QTextBrowser
{
public:
    inline TestBrowser() : htmlLoadAttempts(0) {
        show();
        QApplication::setActiveWindow(this);
        activateWindow();
        setFocus();
        QVERIFY(QTest::qWaitForWindowActive(this));
        QVERIFY(hasFocus());
    }

    virtual QVariant loadResource(int type, const QUrl &name);

    int htmlLoadAttempts;
    QUrl lastResource;
    QUrl sourceInsideLoadResource;
};

QVariant TestBrowser::loadResource(int type, const QUrl &name)
{
    if (type == QTextDocument::HtmlResource)
        htmlLoadAttempts++;
    lastResource = name;
    sourceInsideLoadResource = source();
    return QTextBrowser::loadResource(type, name);
}

class tst_QTextBrowser : public QObject
{
    Q_OBJECT
public:
    tst_QTextBrowser();
    virtual ~tst_QTextBrowser();

public slots:
    void init();
    void cleanup();

private slots:
    void noReloadOnAnchorJump();
    void bgColorOnSourceChange();
    void forwardButton();
    void viewportPositionInHistory();
    void relativeLinks();
    void anchors();
    void resourceAutoDetection();
    void forwardBackwardAvailable();
    void clearHistory();
    void sourceInsideLoadResource();
    void textInteractionFlags_vs_readOnly();
    void anchorsWithSelfBuiltHtml();
    void relativeNonLocalUrls();
    void adjacentAnchors();
    void loadResourceOnRelativeLocalFiles();
    void focusIndicator();
    void focusHistory();
    void urlEncoding();

private:
    TestBrowser *browser;
};

tst_QTextBrowser::tst_QTextBrowser()
{
}

tst_QTextBrowser::~tst_QTextBrowser()
{
}

void tst_QTextBrowser::init()
{
    QString prefix = QFileInfo(QFINDTESTDATA("subdir")).absolutePath();
    QVERIFY2(!prefix.isEmpty(), "Test data directory not found");
    QDir::setCurrent(prefix);

    browser = new TestBrowser;
    browser->show();
}

void tst_QTextBrowser::cleanup()
{
    delete browser;
    browser = 0;
}

void tst_QTextBrowser::noReloadOnAnchorJump()
{
    QUrl url = QUrl::fromLocalFile("anchor.html");

    browser->htmlLoadAttempts = 0;
    browser->setSource(url);
    QCOMPARE(browser->htmlLoadAttempts, 1);
    QVERIFY(!browser->toPlainText().isEmpty());

    url.setFragment("jumphere"); // anchor.html#jumphere
    browser->setSource(url);
    QCOMPARE(browser->htmlLoadAttempts, 1);
    QVERIFY(!browser->toPlainText().isEmpty());
    QCOMPARE(browser->source(), url);
}

void tst_QTextBrowser::bgColorOnSourceChange()
{
    browser->setSource(QUrl::fromLocalFile("pagewithbg.html"));
    QVERIFY(browser->document()->rootFrame()->frameFormat().hasProperty(QTextFormat::BackgroundBrush));
    QCOMPARE(browser->document()->rootFrame()->frameFormat().background().color(), QColor(Qt::blue));

    browser->setSource(QUrl::fromLocalFile("pagewithoutbg.html"));
    QVERIFY(!browser->document()->rootFrame()->frameFormat().hasProperty(QTextFormat::BackgroundBrush));
}

void tst_QTextBrowser::forwardButton()
{
    QSignalSpy forwardEmissions(browser, SIGNAL(forwardAvailable(bool)));
    QSignalSpy backwardEmissions(browser, SIGNAL(backwardAvailable(bool)));

    QVERIFY(browser->historyTitle(-1).isEmpty());
    QVERIFY(browser->historyTitle(0).isEmpty());
    QVERIFY(browser->historyTitle(1).isEmpty());

    browser->setSource(QUrl::fromLocalFile("pagewithbg.html"));

    QVERIFY(!forwardEmissions.isEmpty());
    QVariant val = forwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(!val.toBool());

    QVERIFY(!backwardEmissions.isEmpty());
    val = backwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(!val.toBool());

    QVERIFY(browser->historyTitle(-1).isEmpty());
    QCOMPARE(browser->historyUrl(0), QUrl::fromLocalFile("pagewithbg.html"));
    QCOMPARE(browser->documentTitle(), QString("Page With BG"));
    QCOMPARE(browser->historyTitle(0), QString("Page With BG"));
    QVERIFY(browser->historyTitle(1).isEmpty());

    browser->setSource(QUrl::fromLocalFile("anchor.html"));

    QVERIFY(!forwardEmissions.isEmpty());
    val = forwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(!val.toBool());

    QVERIFY(!backwardEmissions.isEmpty());
    val = backwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(val.toBool());

    QCOMPARE(browser->historyTitle(-1), QString("Page With BG"));
    QCOMPARE(browser->historyTitle(0), QString("Sample Anchor"));
    QVERIFY(browser->historyTitle(1).isEmpty());

    browser->backward();

    QVERIFY(!forwardEmissions.isEmpty());
    val = forwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(val.toBool());

    QVERIFY(!backwardEmissions.isEmpty());
    val = backwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(!val.toBool());

    QVERIFY(browser->historyTitle(-1).isEmpty());
    QCOMPARE(browser->historyTitle(0), QString("Page With BG"));
    QCOMPARE(browser->historyTitle(1), QString("Sample Anchor"));

    browser->setSource(QUrl("pagewithoutbg.html"));

    QVERIFY(!forwardEmissions.isEmpty());
    val = forwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(!val.toBool());

    QVERIFY(!backwardEmissions.isEmpty());
    val = backwardEmissions.takeLast()[0];
    QCOMPARE(val.type(), QVariant::Bool);
    QVERIFY(val.toBool());
}

void tst_QTextBrowser::viewportPositionInHistory()
{
    browser->setSource(QUrl::fromLocalFile("bigpage.html"));
    browser->scrollToAnchor("bottom");
    QVERIFY(browser->verticalScrollBar()->value() > 0);

    browser->setSource(QUrl::fromLocalFile("pagewithbg.html"));
    QCOMPARE(browser->verticalScrollBar()->value(), 0);

    browser->backward();
    QVERIFY(browser->verticalScrollBar()->value() > 0);
}

void tst_QTextBrowser::relativeLinks()
{
    QSignalSpy sourceChangedSpy(browser, SIGNAL(sourceChanged(QUrl)));
    browser->setSource(QUrl("subdir/../qtextbrowser.html"));
    QVERIFY(!browser->document()->isEmpty());
    QCOMPARE(sourceChangedSpy.count(), 1);
    QCOMPARE(sourceChangedSpy.takeFirst()[0].toUrl(), QUrl("subdir/../qtextbrowser.html"));
    browser->setSource(QUrl("subdir/index.html"));
    QVERIFY(!browser->document()->isEmpty());
    QCOMPARE(sourceChangedSpy.count(), 1);
    QCOMPARE(sourceChangedSpy.takeFirst()[0].toUrl(), QUrl("subdir/index.html"));
    browser->setSource(QUrl("anchor.html"));
    QVERIFY(!browser->document()->isEmpty());
    QCOMPARE(sourceChangedSpy.count(), 1);
    QCOMPARE(sourceChangedSpy.takeFirst()[0].toUrl(), QUrl("anchor.html"));
    browser->setSource(QUrl("subdir/index.html"));
    QVERIFY(!browser->document()->isEmpty());
    QCOMPARE(sourceChangedSpy.count(), 1);
    QCOMPARE(sourceChangedSpy.takeFirst()[0].toUrl(), QUrl("subdir/index.html"));

    // using QUrl::fromLocalFile()
    browser->setSource(QUrl::fromLocalFile("anchor.html"));
    QVERIFY(!browser->document()->isEmpty());
    QCOMPARE(sourceChangedSpy.count(), 1);
    QCOMPARE(sourceChangedSpy.takeFirst()[0].toUrl(), QUrl("file:anchor.html"));
    browser->setSource(QUrl("subdir/../qtextbrowser.html"));
    QVERIFY(!browser->document()->isEmpty());
    QCOMPARE(sourceChangedSpy.count(), 1);
    QCOMPARE(sourceChangedSpy.takeFirst()[0].toUrl(), QUrl("subdir/../qtextbrowser.html"));
}

void tst_QTextBrowser::anchors()
{
    browser->setSource(QUrl::fromLocalFile("bigpage.html"));
    browser->setSource(QUrl("#bottom"));
    QVERIFY(browser->verticalScrollBar()->value() > 0);

    browser->setSource(QUrl::fromLocalFile("bigpage.html"));
    browser->setSource(QUrl("#id-anchor"));
    QVERIFY(browser->verticalScrollBar()->value() > 0);
}

void tst_QTextBrowser::resourceAutoDetection()
{
    browser->setHtml("<img src=\":/some/resource\"/>");
    QCOMPARE(browser->lastResource.toString(), QString("qrc:/some/resource"));
}

void tst_QTextBrowser::forwardBackwardAvailable()
{
    QSignalSpy backwardSpy(browser, SIGNAL(backwardAvailable(bool)));
    QSignalSpy forwardSpy(browser, SIGNAL(forwardAvailable(bool)));

    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());

    browser->setSource(QUrl::fromLocalFile("anchor.html"));
    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(!backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->setSource(QUrl::fromLocalFile("bigpage.html"));
    QVERIFY(browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->setSource(QUrl::fromLocalFile("pagewithbg.html"));
    QVERIFY(browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->backward();
    QVERIFY(browser->isBackwardAvailable());
    QVERIFY(browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->backward();
    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(!backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->forward();
    QVERIFY(browser->isBackwardAvailable());
    QVERIFY(browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->forward();
    QVERIFY(browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();
}

void tst_QTextBrowser::clearHistory()
{
    QSignalSpy backwardSpy(browser, SIGNAL(backwardAvailable(bool)));
    QSignalSpy forwardSpy(browser, SIGNAL(forwardAvailable(bool)));

    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());

    browser->clearHistory();
    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(!backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());
    QVERIFY(browser->historyTitle(-1).isEmpty());
    QVERIFY(browser->historyTitle(0).isEmpty());
    QVERIFY(browser->historyTitle(1).isEmpty());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->setSource(QUrl::fromLocalFile("anchor.html"));
    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(!backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->setSource(QUrl::fromLocalFile("bigpage.html"));
    QVERIFY(browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());

    backwardSpy.clear();
    forwardSpy.clear();

    browser->clearHistory();
    QVERIFY(!browser->isBackwardAvailable());
    QVERIFY(!browser->isForwardAvailable());
    QCOMPARE(backwardSpy.count(), 1);
    QVERIFY(!backwardSpy.at(0).at(0).toBool());
    QCOMPARE(forwardSpy.count(), 1);
    QVERIFY(!forwardSpy.at(0).at(0).toBool());
    QVERIFY(browser->historyTitle(-1).isEmpty());
    QVERIFY(browser->historyTitle(1).isEmpty());

    QCOMPARE(browser->source(), QUrl::fromLocalFile("bigpage.html"));
    browser->backward();
    QCOMPARE(browser->source(), QUrl::fromLocalFile("bigpage.html"));
    browser->home();
    QCOMPARE(browser->source(), QUrl::fromLocalFile("bigpage.html"));
}

void tst_QTextBrowser::sourceInsideLoadResource()
{
    QUrl url = QUrl::fromLocalFile("pagewithimage.html");
    browser->setSource(url);
    QCOMPARE(browser->lastResource, QUrl::fromLocalFile(QDir::current().filePath("foobar.png")));
    QEXPECT_FAIL("", "This is currently not supported", Continue);
    QCOMPARE(browser->sourceInsideLoadResource.toString(), url.toString());
}

void tst_QTextBrowser::textInteractionFlags_vs_readOnly()
{
    QVERIFY(browser->isReadOnly());
    QCOMPARE(browser->textInteractionFlags(), Qt::TextBrowserInteraction);
    browser->setReadOnly(true);
    QCOMPARE(browser->textInteractionFlags(), Qt::TextBrowserInteraction);
    browser->setReadOnly(false);
    QCOMPARE(browser->textInteractionFlags(), Qt::TextEditorInteraction);
    browser->setReadOnly(true);
    QCOMPARE(browser->textInteractionFlags(), Qt::TextBrowserInteraction);
}

void tst_QTextBrowser::anchorsWithSelfBuiltHtml()
{
    browser->setHtml("<p>Hello <a href=\"#anchor\">Link</a>"
                     "<p><a name=\"anchor\"/>Blah</p>");
    QVERIFY(browser->document()->blockCount() > 1);
    browser->setSource(QUrl("#anchor"));
    QVERIFY(browser->document()->blockCount() > 1);
}

class HelpBrowser : public QTextBrowser
{
public:
    virtual QVariant loadResource(int /*type*/, const QUrl &name) {
        QString url = name.toString();
        if(url == "qhelp://docs/index.html") {
            return "index";
        } else if (url == "qhelp://docs/classes.html") {
            return "classes";
        } else if (url == "qhelp://docs/someclass.html") {
            return "someclass";
        }
        return QVariant();
    }
};

void tst_QTextBrowser::relativeNonLocalUrls()
{
    HelpBrowser browser;
    browser.setSource(QUrl("qhelp://docs/index.html"));
    QCOMPARE(browser.toPlainText(), QString("index"));
    browser.setSource(QUrl("classes.html"));
    QCOMPARE(browser.toPlainText(), QString("classes"));
    browser.setSource(QUrl("someclass.html"));
    QCOMPARE(browser.toPlainText(), QString("someclass"));
}

class HackBrowser : public TestBrowser
{
public:
    inline bool focusTheNextChild() { return QTextBrowser::focusNextChild(); }
    inline bool focusThePreviousChild() { return QTextBrowser::focusPreviousChild(); }
};

void tst_QTextBrowser::adjacentAnchors()
{
    HackBrowser *browser = new HackBrowser;
    browser->setHtml("<a href=\"#foo\">foo</a><a href=\"#bar\">bar</a>");
    QVERIFY(browser->focusTheNextChild());
    QCOMPARE(browser->textCursor().selectedText(), QString("foo"));

    QVERIFY(browser->focusTheNextChild());
    QCOMPARE(browser->textCursor().selectedText(), QString("bar"));

    QVERIFY(!browser->focusTheNextChild());

    browser->moveCursor(QTextCursor::End);
    QVERIFY(browser->focusThePreviousChild());
    QCOMPARE(browser->textCursor().selectedText(), QString("bar"));
    QVERIFY(browser->focusThePreviousChild());
    QCOMPARE(browser->textCursor().selectedText(), QString("foo"));

    delete browser;
}

void tst_QTextBrowser::loadResourceOnRelativeLocalFiles()
{
    browser->setSource(QUrl::fromLocalFile("subdir/index.html"));
    QVERIFY(!browser->toPlainText().isEmpty());
    QVariant v = browser->loadResource(QTextDocument::HtmlResource, QUrl("../anchor.html"));
    QVERIFY(v.isValid());
    QCOMPARE(v.type(), QVariant::ByteArray);
    QVERIFY(!v.toByteArray().isEmpty());
}

void tst_QTextBrowser::focusIndicator()
{
    HackBrowser *browser = new HackBrowser;
    browser->setSource(QUrl::fromLocalFile("firstpage.html"));
    QVERIFY(!browser->textCursor().hasSelection());

    browser->focusTheNextChild();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to second page"));

#ifdef QT_KEYPAD_NAVIGATION
    browser->setEditFocus(true);
#endif
    QTest::keyClick(browser, Qt::Key_Enter);
    QVERIFY(!browser->textCursor().hasSelection());

    browser->focusTheNextChild();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page from second page"));

    QTest::keyClick(browser, Qt::Key_Enter);
    QVERIFY(!browser->textCursor().hasSelection());

    browser->backward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page from second page"));

    browser->backward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to second page"));

    browser->forward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page from second page"));

    browser->backward();
    browser->backward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to second page"));

    QTest::keyClick(browser, Qt::Key_Enter);
    QVERIFY(!browser->textCursor().hasSelection());

    delete browser;
}

void tst_QTextBrowser::focusHistory()
{
    HackBrowser *browser = new HackBrowser;
    browser->setSource(QUrl::fromLocalFile("firstpage.html"));
    QVERIFY(!browser->textCursor().hasSelection());

    browser->focusTheNextChild();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to second page"));

#ifdef QT_KEYPAD_NAVIGATION
    browser->setEditFocus(true);
#endif
    QTest::keyClick(browser, Qt::Key_Enter);
    QVERIFY(!browser->textCursor().hasSelection());

    browser->focusTheNextChild();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page from second page"));

    browser->backward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to second page"));

    browser->focusTheNextChild();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page"));

    // Despite the third page link being highlighted, going forward should go to second,
    // and going back after that should still highlight the third link
    browser->forward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page from second page"));

    browser->backward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page"));

    browser->forward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page from second page"));

    QTest::keyClick(browser, Qt::Key_Enter);
    QVERIFY(!browser->textCursor().hasSelection());

    browser->backward();
    browser->backward();

    QVERIFY(browser->textCursor().hasSelection());
    QCOMPARE(browser->textCursor().selectedText(), QString("Link to third page"));

    delete browser;
}

void tst_QTextBrowser::urlEncoding()
{
    HackBrowser *browser = new HackBrowser;
    browser->setOpenLinks(false);
    browser->setHtml("<a href=\"http://www.google.com/q=%22\">link</a>");
    browser->focusTheNextChild();

    QSignalSpy spy(browser, SIGNAL(anchorClicked(QUrl)));

#ifdef QT_KEYPAD_NAVIGATION
    browser->setEditFocus(true);
#endif
    QTest::keyClick(browser, Qt::Key_Enter);
    QCOMPARE(spy.count(), 1);

    QUrl url = spy.at(0).at(0).toUrl();
    QCOMPARE(url.toEncoded(), QByteArray("http://www.google.com/q=%22"));

    delete browser;
}

QTEST_MAIN(tst_QTextBrowser)
#include "tst_qtextbrowser.moc"
