// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets>
#include <vector>
#include <memory>
#include <type_traits>

#include <private/qkeymapper_p.h>
#include <private/qguiapplication_p.h>

static const QKeySequence keySequences[] = {
    QKeySequence("Ctrl+C"),
    QKeySequence("Ctrl++"),
};

class KeyEventModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit KeyEventModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    enum Columns {
        Language,
        Direction,
        Type,
        ScanCode,
        VirtualKey,
        Modifiers,
        Key,
        Text,
        PortableText,
        NativeText,
        PossibleKeys,
        FirstKeySequence,
        LastKeySequence = FirstKeySequence + std::size(keySequences) - 1,
        KeySequenceEdit,
        ColumnCount
    };

    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return int(m_events.size());
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return ColumnCount;
    }

    QVariant headerData(int column, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            switch (column) {
            case Language: return QString("language");
            case Direction: return QString("direction");
            case Type: return QString("type");
            case ScanCode: return QString("nativeScanCode");
            case VirtualKey: return QString("nativeVirtualKey");
            case Modifiers: return QString("modifiers");
            case Key: return QString("key");
            case Text: return QString("text");
            case PortableText: return QString("PortableText");
            case NativeText: return QString("NativeText");
            case PossibleKeys: return QString("keyCombinations");
            case KeySequenceEdit: return m_customKeySequence.toString();
            default: {
                auto keySequence = keySequences[column - FirstKeySequence];
                return keySequence.toString();
            }
            }
        }
        return QVariant();
    }

    template <typename T>
    static QString toString(T &&object, int verbosity = QDebug::DefaultVerbosity)
    {
        QString buffer;
        QDebug stream(&buffer);
        stream.setVerbosity(verbosity);
        stream.nospace() << std::forward<T>(object);
        return buffer;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole) {
            auto &event = m_events.at(index.row());
            auto *keyEvent = event.keyEvent.get();
            switch (int column = index.column()) {
            case Language: return event.language;
            case Direction: return toString(event.layoutDirection, QDebug::MinimumVerbosity);
            case Type: return toString(keyEvent->type(), QDebug::MinimumVerbosity);
            case ScanCode: return keyEvent->nativeScanCode();
            case VirtualKey: return keyEvent->nativeVirtualKey();
            case Modifiers: return toString(keyEvent->modifiers(), QDebug::MinimumVerbosity);
            case Key: return toString(Qt::Key(keyEvent->key()), QDebug::MinimumVerbosity);
            case Text: return keyEvent->text();
            case PortableText: return event.keySequence.toString(QKeySequence::PortableText);
            case NativeText: return event.keySequence.toString(QKeySequence::NativeText);
            case PossibleKeys: {
                QStringList keyCombinations;
                for (auto combination : event.possibleKeyCombinations)
                    keyCombinations << QKeySequence(combination).toString(QKeySequence::NativeText);
                static constexpr auto leftToRightOverride = QChar(0x202d);
                return leftToRightOverride + keyCombinations.join("    ");
            }
            default: {
                QStringList matches;
                if (event.keySequenceEquals[column - FirstKeySequence])
                    matches << "K";
                if (event.shortcutMatches[column - FirstKeySequence])
                    matches << "S";
                return matches.join(" ");
            }
            }
        } else if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }

        return QVariant();
    }

    struct Event {
        std::unique_ptr<QKeyEvent> keyEvent;
        QString language;
        Qt::LayoutDirection layoutDirection;
        QKeySequence keySequence;
        using PossibleKeysList = decltype(std::declval<QKeyMapper>().possibleKeys(nullptr));
        PossibleKeysList possibleKeyCombinations;
        // Hard-coded key sequences, plus room for KeySequenceEdit
        bool keySequenceEquals[std::size(keySequences) + 1] = {};
        bool shortcutMatches[std::size(keySequences) + 1] = {};
    };

    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (!m_enabled)
            return false;

        switch (auto type = event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            auto *inputMethod = qGuiApp->inputMethod();
            auto row = int(m_events.size());
            beginInsertRows(QModelIndex(), row, row);
            m_events.push_back({
                std::unique_ptr<QKeyEvent>(keyEvent->clone()),
                QLocale::languageToString(inputMethod->locale().language()),
                inputMethod->inputDirection(),
                QKeySequence(keyEvent->keyCombination()),
                QKeyMapper::instance()->possibleKeys(keyEvent)
            });

            Event &event = m_events.back();

            if (type == QEvent::KeyPress) {
                for (size_t i = 0; i < std::size(event.keySequenceEquals); ++i) {
                    QKeySequence keySequence = i == std::size(keySequences) ?
                        m_customKeySequence : keySequences[i];

                    event.keySequenceEquals[i] = event.keySequence == keySequence;

                    QShortcut shortcut(keySequence, object, [&] {
                         event.shortcutMatches[i] = true;
                    }, Qt::ApplicationShortcut);
                    QShortcutMap &shortcutMap = QGuiApplicationPrivate::instance()->shortcutMap;
                    shortcutMap.tryShortcut(keyEvent);
                }
            }

            endInsertRows();
            return false;
        }
        case QEvent::ShortcutOverride: {
            auto *keyEvent = static_cast<QKeyEvent*>(event);
            if (!keyEvent->matches(QKeySequence::Quit)) {
                event->accept();
                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }

    void reset()
    {
        beginResetModel();
        m_events.clear();
        endResetModel();
    }

    Q_SLOT void setCustomKeySequence(const QKeySequence &keySequence)
    {
        m_customKeySequence = keySequence;
        emit headerDataChanged(Qt::Horizontal, KeySequenceEdit, ColumnCount);
    }

    bool m_enabled = true;
    std::vector<Event> m_events;
    QKeySequence m_customKeySequence;
};

class KeyEventWindow : public QMainWindow
{
public:
    KeyEventWindow()
    {
        setWindowTitle(QString("Qt %1 on %2").arg(QT_VERSION_STR).arg(QSysInfo::prettyProductName()));
        auto *tableView = new QTableView(this);
        m_keyEventModel = new KeyEventModel(this);
        tableView->setModel(m_keyEventModel);
        tableView->installEventFilter(m_keyEventModel);

        tableView->setFocusPolicy(Qt::ClickFocus);
        tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableView->setSelectionMode(QAbstractItemView::NoSelection);
        tableView->setWordWrap(false);

        QObject::connect(tableView->model(), &QAbstractItemModel::rowsInserted,
                         tableView, &QTableView::scrollToBottom);

        QMenu *menu = menuBar()->addMenu("File");
        menu->addAction("Save...", this, &KeyEventWindow::save);
        menu->addAction("Clear", this, [this]{
            m_keyEventModel->reset();
        });
        auto *enableAction = menu->addAction("Enabled", this, [this]{
            auto *action = static_cast<QAction*>(sender());
            m_keyEventModel->m_enabled = action->isChecked();
        });
        enableAction->setCheckable(true);
        enableAction->setChecked(true);

        auto *toolBar = addToolBar("Tools");
        toolBar->setMovable(false);
        toolBar->addWidget(new QLabel("Key sequence editor:"));
        auto *keySequenceEdit = new QKeySequenceEdit;
        keySequenceEdit->setMaximumSequenceLength(1);
        connect(keySequenceEdit, &QKeySequenceEdit::keySequenceChanged,
                m_keyEventModel, &KeyEventModel::setCustomKeySequence);
        keySequenceEdit->installEventFilter(m_keyEventModel);
        toolBar->addWidget(keySequenceEdit);
        toolBar->addWidget(new QLabel("Free form text input:"));
        toolBar->addWidget(new QLineEdit);

        setCentralWidget(tableView);
        centralWidget()->setFocus();
    }

    void save()
    {
        auto homeDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString fileName = QFileDialog::getSaveFileName(this, "Save events",
                            QString("%1/events.csv").arg(homeDirectory));

        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            QMessageBox::critical(this, "Could not open file", file.errorString());
            return;
        }
        QTextStream output(&file);
        const auto columns = m_keyEventModel->columnCount();
        for (int c = 0; c < columns; ++c) {
            output << m_keyEventModel->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString()
                   << ((c < columns - 1) ? ";" : "");
        }
        output << "\n";
        for (int r = 0; r < m_keyEventModel->rowCount(); ++r) {
            for (int c = 0; c < m_keyEventModel->columnCount(); ++c) {
                auto index = m_keyEventModel->index(r, c);
                output << m_keyEventModel->data(index).toString()
                       << ((c < columns - 1) ? ";" : "");
            }
            output << "\n";
        }
    }

    void keyPressEvent(QKeyEvent *keyEvent) override
    {
        if (keyEvent->matches(QKeySequence::Quit))
            qGuiApp->quit();
    }

    KeyEventModel *m_keyEventModel = nullptr;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    KeyEventWindow keyEventWindow;
    keyEventWindow.showMaximized();

    return a.exec();
}

#include "main.moc"
