// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets>
#include <QtCore/QCommandLineParser>

#include <QtWidgets/private/qapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#ifdef QT_QUICKWIDGETS_LIB
#include <QQuickWidget>
#endif

using namespace Qt::StringLiterals;

class IconModel : public QAbstractItemModel
{
    const QStringList themedIcons = {
        u"address-book-new"_s,
        u"application-exit"_s,
        u"appointment-new"_s,
        u"call-start"_s,
        u"call-stop"_s,
        u"contact-new"_s,
        u"document-new"_s,
        u"document-open"_s,
        u"document-open-recent"_s,
        u"document-page-setup"_s,
        u"document-print"_s,
        u"document-print-preview"_s,
        u"document-properties"_s,
        u"document-revert"_s,
        u"document-save"_s,
        u"document-save-as"_s,
        u"document-send"_s,
        u"edit-clear"_s,
        u"edit-copy"_s,
        u"edit-cut"_s,
        u"edit-delete"_s,
        u"edit-find"_s,
        u"edit-find-replace"_s,
        u"edit-paste"_s,
        u"edit-redo"_s,
        u"edit-select-all"_s,
        u"edit-undo"_s,
        u"folder-new"_s,
        u"format-indent-less"_s,
        u"format-indent-more"_s,
        u"format-justify-center"_s,
        u"format-justify-fill"_s,
        u"format-justify-left"_s,
        u"format-justify-right"_s,
        u"format-text-direction-ltr"_s,
        u"format-text-direction-rtl"_s,
        u"format-text-bold"_s,
        u"format-text-italic"_s,
        u"format-text-underline"_s,
        u"format-text-strikethrough"_s,
        u"go-bottom"_s,
        u"go-down"_s,
        u"go-first"_s,
        u"go-home"_s,
        u"go-jump"_s,
        u"go-last"_s,
        u"go-next"_s,
        u"go-previous"_s,
        u"go-top"_s,
        u"go-up"_s,
        u"help-about"_s,
        u"help-contents"_s,
        u"help-faq"_s,
        u"insert-image"_s,
        u"insert-link"_s,
        u"insert-object"_s,
        u"insert-text"_s,
        u"list-add"_s,
        u"list-remove"_s,
        u"mail-forward"_s,
        u"mail-mark-important"_s,
        u"mail-mark-junk"_s,
        u"mail-mark-notjunk"_s,
        u"mail-mark-read"_s,
        u"mail-mark-unread"_s,
        u"mail-message-new"_s,
        u"mail-reply-all"_s,
        u"mail-reply-sender"_s,
        u"mail-send"_s,
        u"mail-send-receive"_s,
        u"media-eject"_s,
        u"media-playback-pause"_s,
        u"media-playback-start"_s,
        u"media-playback-stop"_s,
        u"media-record"_s,
        u"media-seek-backward"_s,
        u"media-seek-forward"_s,
        u"media-skip-backward"_s,
        u"media-skip-forward"_s,
        u"object-flip-horizontal"_s,
        u"object-flip-vertical"_s,
        u"object-rotate-left"_s,
        u"object-rotate-right"_s,
        u"process-stop"_s,
        u"system-lock-screen"_s,
        u"system-log-out"_s,
        u"system-run"_s,
        u"system-search"_s,
        u"system-reboot"_s,
        u"system-shutdown"_s,
        u"tools-check-spelling"_s,
        u"view-fullscreen"_s,
        u"view-refresh"_s,
        u"view-restore"_s,
        u"view-sort-ascending"_s,
        u"view-sort-descending"_s,
        u"window-close"_s,
        u"window-new"_s,
        u"zoom-fit-best"_s,
        u"zoom-in"_s,
        u"zoom-original"_s,
        u"zoom-out"_s,


        u"process-working"_s,


        u"accessories-calculator"_s,
        u"accessories-character-map"_s,
        u"accessories-dictionary"_s,
        u"accessories-text-editor"_s,
        u"help-browser"_s,
        u"multimedia-volume-control"_s,
        u"preferences-desktop-accessibility"_s,
        u"preferences-desktop-font"_s,
        u"preferences-desktop-keyboard"_s,
        u"preferences-desktop-locale"_s,
        u"preferences-desktop-multimedia"_s,
        u"preferences-desktop-screensaver"_s,
        u"preferences-desktop-theme"_s,
        u"preferences-desktop-wallpaper"_s,
        u"system-file-manager"_s,
        u"system-software-install"_s,
        u"system-software-update"_s,
        u"utilities-system-monitor"_s,
        u"utilities-terminal"_s,


        u"applications-accessories"_s,
        u"applications-development"_s,
        u"applications-engineering"_s,
        u"applications-games"_s,
        u"applications-graphics"_s,
        u"applications-internet"_s,
        u"applications-multimedia"_s,
        u"applications-office"_s,
        u"applications-other"_s,
        u"applications-science"_s,
        u"applications-system"_s,
        u"applications-utilities"_s,
        u"preferences-desktop"_s,
        u"preferences-desktop-peripherals"_s,
        u"preferences-desktop-personal"_s,
        u"preferences-other"_s,
        u"preferences-system"_s,
        u"preferences-system-network"_s,
        u"system-help"_s,


        u"audio-card"_s,
        u"audio-input-microphone"_s,
        u"battery"_s,
        u"camera-photo"_s,
        u"camera-video"_s,
        u"camera-web"_s,
        u"computer"_s,
        u"drive-harddisk"_s,
        u"drive-optical"_s,
        u"drive-removable-media"_s,
        u"input-gaming"_s,
        u"input-keyboard"_s,
        u"input-mouse"_s,
        u"input-tablet"_s,
        u"media-flash"_s,
        u"media-floppy"_s,
        u"media-optical"_s,
        u"media-tape"_s,
        u"modem"_s,
        u"multimedia-player"_s,
        u"network-wired"_s,
        u"network-wireless"_s,
        u"pda"_s,
        u"phone"_s,
        u"printer"_s,
        u"scanner"_s,
        u"video-display"_s,


        u"emblem-default"_s,
        u"emblem-documents"_s,
        u"emblem-downloads"_s,
        u"emblem-favorite"_s,
        u"emblem-important"_s,
        u"emblem-mail"_s,
        u"emblem-photos"_s,
        u"emblem-readonly"_s,
        u"emblem-shared"_s,
        u"emblem-symbolic-link"_s,
        u"emblem-synchronized"_s,
        u"emblem-system"_s,
        u"emblem-unreadable"_s,


        u"face-angel"_s,
        u"face-angry"_s,
        u"face-cool"_s,
        u"face-crying"_s,
        u"face-devilish"_s,
        u"face-embarrassed"_s,
        u"face-kiss"_s,
        u"face-laugh"_s,
        u"face-monkey"_s,
        u"face-plain"_s,
        u"face-raspberry"_s,
        u"face-sad"_s,
        u"face-sick"_s,
        u"face-smile"_s,
        u"face-smile-big"_s,
        u"face-smirk"_s,
        u"face-surprise"_s,
        u"face-tired"_s,
        u"face-uncertain"_s,
        u"face-wink"_s,
        u"face-worried"_s,


        u"flag-aa"_s,


        u"application-x-executable"_s,
        u"audio-x-generic"_s,
        u"font-x-generic"_s,
        u"image-x-generic"_s,
        u"package-x-generic"_s,
        u"text-html"_s,
        u"text-x-generic"_s,
        u"text-x-generic-template"_s,
        u"text-x-script"_s,
        u"video-x-generic"_s,
        u"x-office-address-book"_s,
        u"x-office-calendar"_s,
        u"x-office-document"_s,
        u"x-office-presentation"_s,
        u"x-office-spreadsheet"_s,


        u"folder"_s,
        u"folder-remote"_s,
        u"network-server"_s,
        u"network-workgroup"_s,
        u"start-here"_s,
        u"user-bookmarks"_s,
        u"user-desktop"_s,
        u"user-home"_s,
        u"user-trash"_s,


        u"appointment-missed"_s,
        u"appointment-soon"_s,
        u"audio-volume-high"_s,
        u"audio-volume-low"_s,
        u"audio-volume-medium"_s,
        u"audio-volume-muted"_s,
        u"battery-caution"_s,
        u"battery-low"_s,
        u"dialog-error"_s,
        u"dialog-information"_s,
        u"dialog-password"_s,
        u"dialog-question"_s,
        u"dialog-warning"_s,
        u"folder-drag-accept"_s,
        u"folder-open"_s,
        u"folder-visiting"_s,
        u"image-loading"_s,
        u"image-missing"_s,
        u"mail-attachment"_s,
        u"mail-unread"_s,
        u"mail-read"_s,
        u"mail-replied"_s,
        u"mail-signed"_s,
        u"mail-signed-verified"_s,
        u"media-playlist-repeat"_s,
        u"media-playlist-shuffle"_s,
        u"network-error"_s,
        u"network-idle"_s,
        u"network-offline"_s,
        u"network-receive"_s,
        u"network-transmit"_s,
        u"network-transmit-receive"_s,
        u"printer-error"_s,
        u"printer-printing"_s,
        u"security-high"_s,
        u"security-medium"_s,
        u"security-low"_s,
        u"software-update-available"_s,
        u"software-update-urgent"_s,
        u"sync-error"_s,
        u"sync-synchronizing"_s,
        u"task-due"_s,
        u"task-past-due"_s,
        u"user-available"_s,
        u"user-away"_s,
        u"user-idle"_s,
        u"user-offline"_s,
        u"user-trash-full"_s,
        u"weather-clear"_s,
        u"weather-clear-night"_s,
        u"weather-few-clouds"_s,
        u"weather-few-clouds-night"_s,
        u"weather-fog"_s,
        u"weather-overcast"_s,
        u"weather-severe-alert"_s,
        u"weather-showers"_s,
        u"weather-showers-scattered"_s,
        u"weather-snow"_s,
        u"weather-storm"_s,
    };

    const QStringList fileIconTypes = {
        u"Computer"_s,
        u"Desktop"_s,
        u"Trashcan"_s,
        u"Network"_s,
        u"Drive"_s,
        u"Folder"_s,
        u"File"_s
    };

    QAbstractFileIconProvider m_fileIconProvider;
public:
    using QAbstractItemModel::QAbstractItemModel;

    enum Columns {
        Name,
        StyleIcon,
        StylePixmap,
        File,
        Theme,
        Icon
    };

    int rowCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;
        return themedIcons.size() + QStyle::NStandardPixmap;
    }
    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;
        return Icon + 1;
    }
    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return {};
        if (column > columnCount(parent) || row > rowCount(parent))
            return {};
        return createIndex(row, column, quintptr(row));
    }
    QModelIndex parent(const QModelIndex &) const override
    {
        return {};
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        int row = index.row();
        const Columns column = Columns(index.column());
        if (!index.isValid() || row >= rowCount(index.parent()) || column >= columnCount(index.parent()))
            return {};

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case StylePixmap:
            case StyleIcon:
            case Theme: {
                const QMetaObject *styleMO = &QStyle::staticMetaObject;
                const int pixmapIndex = styleMO->indexOfEnumerator("StandardPixmap");
                Q_ASSERT(pixmapIndex >= 0);
                const QMetaEnum pixmapEnum = styleMO->enumerator(pixmapIndex);
                const QString pixmapName = QString::fromUtf8(pixmapEnum.key(row));
                return QVariant(pixmapName);
            }
            case File:
                return row < fileIconTypes.size() ? fileIconTypes.at(row) : QVariant();
            default:
                return themedIcons.at(row);
            }
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case Name:
                break;
            case StylePixmap:
                if (row >= themedIcons.size())
                    break;
                return QIcon(QApplication::style()->standardPixmap(QStyle::StandardPixmap(row)));
            case StyleIcon:
                if (row >= themedIcons.size())
                    break;
                return QApplication::style()->standardIcon(QStyle::StandardPixmap(row));
            case Theme:
                if (row >= themedIcons.size())
                    break;
                return QIcon(QApplicationPrivate::platformTheme()->standardPixmap(
                    QPlatformTheme::StandardPixmap(row), QSize(64, 64) * qGuiApp->devicePixelRatio()));
            case Icon:
                if (row < themedIcons.size())
                    return QIcon::fromTheme(themedIcons.at(row));
                break;
            case File:
                if (row >= fileIconTypes.size())
                    break;
                return m_fileIconProvider.icon(QAbstractFileIconProvider::IconType(row));
            }
            break;
        default:
            break;
        }
        return {};
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        switch (orientation) {
        case Qt::Vertical:
            break;
        case Qt::Horizontal:
            if (role == Qt::DisplayRole) {
                switch (section) {
                case Name:
                    return "Name";
                case StylePixmap:
                    return "Style::standardPixmap";
                case StyleIcon:
                    return "Style::standardIcon";
                case Theme:
                    return "Theme";
                case Icon:
                    return "Icon";
                case File:
                    return "File";
                }
            }
        }
        return QAbstractItemModel::headerData(section, orientation, role);
    }
};

template<IconModel::Columns Column>
struct ColumnModel : public QSortFilterProxyModel
{
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &) const override
    {
        return sourceColumn == Column;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex sourceIndex = sourceModel()->index(sourceRow, Column, sourceParent);
        const QIcon iconData = sourceModel()->data(sourceIndex, Qt::DecorationRole).template value<QIcon>();
        return !iconData.isNull();
    }
};

template<IconModel::Columns Column>
struct IconView : public QListView
{
    ColumnModel<Column> proxyModel;

    IconView(QAbstractItemModel *model)
    {
        setViewMode(QListView::ListMode);
        setIconSize(QSize(64, 64));
        setUniformItemSizes(true);
        proxyModel.setSourceModel(model);
        setModel(&proxyModel);
    }
};

class IconInspector : public QFrame
{
public:
    IconInspector()
    {
        setFrameShape(QFrame::StyledPanel);

        QLineEdit *lineEdit = new QLineEdit;
        connect(lineEdit, &QLineEdit::textChanged,
                this, &IconInspector::updateIcon);

        button = new QToolButton;
        button->setCheckable(true);

        QVBoxLayout *vbox = new QVBoxLayout;
        QHBoxLayout *hbox = new QHBoxLayout;
        vbox->addStretch(10);
        hbox->addWidget(lineEdit);
        hbox->addWidget(button);
        vbox->addLayout(hbox);
        setLayout(vbox);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.fillRect(event->rect(), palette().window());

        // some fonts use icon names as ligatures
        if (const QString themeName = QIcon::themeName(); !themeName.isEmpty()) {
            const QFont themeFont(themeName, 24);
            if (QFontInfo(themeFont).family() == themeName) {
                painter.save();
                painter.setFont(themeFont);
                painter.drawText(rect(), icon.name());
                painter.restore();
            }
        }

        if (!icon.isNull()) {
            const QString modeLabels[] = { u"Normal"_s, u"Disabled"_s, u"Active"_s, u"Selected"_s};
            const QString stateLabels[] = { u"On"_s, u"Off"_s};
            const int labelWidth = fontMetrics().horizontalAdvance(u"Disabled"_s);
            const int labelHeight = fontMetrics().height();
            int labelYs[4] = {};
            int labelXs[2] = {};

            painter.save();
            painter.translate(labelWidth + contentsMargins().left(), labelHeight * 2);
            const QBrush brush(palette().base().color(), Qt::CrossPattern);

            QPoint point;
            for (const auto &mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected}) {
                int height = 0;
                for (const auto &state : {QIcon::On, QIcon::Off}) {
                    int totalWidth = 0;
                    const int relativeX = point.x();
                    const auto sizes = icon.availableSizes(mode, state);
                    for (const auto &size : sizes) {
                        if (size.width() > 256)
                            continue;
                        const QRect iconRect(point, size);
                        painter.fillRect(iconRect, brush);
                        icon.paint(&painter, iconRect, Qt::AlignCenter, mode, state);
                        totalWidth += size.width();
                        point.rx() += size.width();
                        height = std::max(height, size.height());
                    }
                    labelXs[state] = relativeX + totalWidth / 2;
                }
                point.rx() = 0;
                labelYs[mode] = point.ry() + height / 2;
                point.ry() += height;
            }
            painter.restore();

            painter.translate(contentsMargins().left(), labelHeight);
            for (const auto &mode : {QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected})
                painter.drawText(QPoint(0, labelYs[mode]), modeLabels[mode]);
            painter.translate(labelWidth, 0);
            for (const auto &state : {QIcon::On, QIcon::Off})
                painter.drawText(QPoint(labelXs[state], 0), stateLabels[state]);
        }
        QFrame::paintEvent(event);
    }
private:
    QToolButton *button;
    QIcon icon;
    void updateIcon(const QString &iconName)
    {
        icon = QIcon::fromTheme(iconName);
        button->setIcon(icon);
        update();
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationVersion(QT_VERSION_STR);
    QApplication::setApplicationName(QLatin1String("IconBrowser Manual Test"));
    QApplication::setOrganizationName(QLatin1String("QtProject"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption themeOption({u"theme"_s, u"t"_s},
                                   u"The name of the icon theme"_s, u"theme"_s);
    parser.addOption(themeOption);
    parser.process(app);
    if (const QString theme = parser.value(themeOption); !theme.isEmpty())
        QIcon::setThemeName(theme);

#ifdef ICONBROWSER_RESOURCE
    Q_INIT_RESOURCE(icons);
#endif

    IconModel model;

    QTabWidget widget;
    widget.setTabPosition(QTabWidget::West);
    widget.addTab(new IconInspector, "Inspect");
    widget.addTab(new IconView<IconModel::Icon>(&model), "QIcon::fromTheme");
    widget.addTab(new IconView<IconModel::StylePixmap>(&model), "QStyle::standardPixmap");
    widget.addTab(new IconView<IconModel::StyleIcon>(&model), "QStyle::standardIcon");
    widget.addTab(new IconView<IconModel::Theme>(&model), "QPlatformTheme");
    widget.addTab(new IconView<IconModel::File>(&model), "QAbstractFileIconProvider");

#ifdef QT_QUICKWIDGETS_LIB
    QQuickWidget *quickBrowser = new QQuickWidget;
    quickBrowser->setSource(QUrl(u"qrc:/Main.qml"_s));
    quickBrowser->setResizeMode(QQuickWidget::SizeRootObjectToView);
    widget.addTab(quickBrowser, "Qt Quick");
    QObject::connect(quickBrowser, &QQuickWidget::statusChanged, quickBrowser,
                     [](QQuickWidget::Status status){
        qDebug() << status;
    });
    QObject::connect(quickBrowser, &QQuickWidget::sceneGraphError, quickBrowser,
                     [](QQuickWindow::SceneGraphError error, const QString &message){
        qDebug() << error << message;
    });
#endif

    widget.show();
    return app.exec();
}
