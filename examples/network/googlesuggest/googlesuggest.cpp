// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "googlesuggest.h"

//! [1]
const QString gsuggestUrl(QStringLiteral("http://google.com/complete/search?output=toolbar&q=%1"));
//! [1]

//! [2]
GSuggestCompletion::GSuggestCompletion(QLineEdit *parent): QObject(parent), editor(parent)
{
    popup = new QTreeWidget;
    popup->setWindowFlags(Qt::Popup);
    popup->setFocusPolicy(Qt::NoFocus);
    popup->setFocusProxy(parent);
    popup->setMouseTracking(true);

    popup->setColumnCount(1);
    popup->setUniformRowHeights(true);
    popup->setRootIsDecorated(false);
    popup->setEditTriggers(QTreeWidget::NoEditTriggers);
    popup->setSelectionBehavior(QTreeWidget::SelectRows);
    popup->setFrameStyle(QFrame::Box | QFrame::Plain);
    popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    popup->header()->hide();

    popup->installEventFilter(this);

    connect(popup, &QTreeWidget::itemClicked,
            this, &GSuggestCompletion::doneCompletion);

    timer.setSingleShot(true);
    timer.setInterval(500);
    connect(&timer, &QTimer::timeout,
            this, &GSuggestCompletion::autoSuggest);
    connect(editor, &QLineEdit::textEdited,
            &timer, QOverload<>::of(&QTimer::start));

    connect(&networkManager, &QNetworkAccessManager::finished,
            this, &GSuggestCompletion::handleNetworkData);

}
//! [2]

//! [3]
GSuggestCompletion::~GSuggestCompletion()
{
    delete popup;
}
//! [3]

//! [4]
bool GSuggestCompletion::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != popup)
        return false;

    if (ev->type() == QEvent::MouseButtonPress) {
        popup->hide();
        editor->setFocus();
        return true;
    }

    if (ev->type() == QEvent::KeyPress) {
        bool consumed = false;
        int key = static_cast<QKeyEvent*>(ev)->key();
        switch (key) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            doneCompletion();
            consumed = true;
            break;

        case Qt::Key_Escape:
            editor->setFocus();
            popup->hide();
            consumed = true;
            break;

        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            break;

        default:
            editor->setFocus();
            editor->event(ev);
            popup->hide();
            break;
        }

        return consumed;
    }

    return false;
}
//! [4]

//! [5]
void GSuggestCompletion::showCompletion(const QList<QString> &choices)
{
    if (choices.isEmpty())
        return;

    const QPalette &pal = editor->palette();
    QColor color = pal.color(QPalette::Disabled, QPalette::WindowText);

    popup->setUpdatesEnabled(false);
    popup->clear();

    for (const auto &choice : choices) {
        auto item  = new QTreeWidgetItem(popup);
        item->setText(0, choice);
        item->setForeground(0, color);
    }

    popup->setCurrentItem(popup->topLevelItem(0));
    popup->resizeColumnToContents(0);
    popup->setUpdatesEnabled(true);

    popup->move(editor->mapToGlobal(QPoint(0, editor->height())));
    popup->setFocus();
    popup->show();
}
//! [5]

//! [6]
void GSuggestCompletion::doneCompletion()
{
    timer.stop();
    popup->hide();
    editor->setFocus();
    QTreeWidgetItem *item = popup->currentItem();
    if (item) {
        editor->setText(item->text(0));
        QMetaObject::invokeMethod(editor, "returnPressed");
    }
}
//! [6]

//! [7]
void GSuggestCompletion::autoSuggest()
{
    QString str = editor->text();
    QString url = gsuggestUrl.arg(str);
    networkManager.get(QNetworkRequest(url));
}
//! [7]

//! [8]
void GSuggestCompletion::preventSuggest()
{
    timer.stop();
}
//! [8]

//! [9]
void GSuggestCompletion::handleNetworkData(QNetworkReply *networkReply)
{
    QUrl url = networkReply->url();
    if (networkReply->error() == QNetworkReply::NoError) {
        QList<QString> choices;

        QByteArray response(networkReply->readAll());
        QXmlStreamReader xml(response);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement)
                if (xml.name() == u"suggestion") {
                    auto str = xml.attributes().value("data");
                    choices << str.toString();
                }
        }

        showCompletion(choices);
    }

    networkReply->deleteLater();
}
//! [9]
