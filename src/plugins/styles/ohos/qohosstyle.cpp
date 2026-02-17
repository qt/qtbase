// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosstyle_p.h"
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qmdisubwindow.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qradiobutton.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qstyleditemdelegate.h>
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qtoolbutton.h>
#include <QtWidgets/qtreeview.h>
#include "qpa/qplatformtheme.h"
#include "private/qguiapplication_p.h"
#include "private/qstylehelper_p.h"
#include <algorithm>

namespace {

constexpr int checkBoxOrRadioButtonIndicatorSize = 24;
constexpr int checkBoxOrRadioButtonIndicatorStrokeWidth = 1;
constexpr int checkBoxOrRadioButtonIndicatorGap = 2;
constexpr int checkBoxOrRadioButtonHoverSize =
    checkBoxOrRadioButtonIndicatorSize + (2 * checkBoxOrRadioButtonIndicatorGap);
constexpr int checkBoxOrRadioButtonFocusStrokeWidth = 2;
constexpr int checkBoxOrRadioButtonSize =
    checkBoxOrRadioButtonIndicatorSize + (2 * checkBoxOrRadioButtonIndicatorGap)
    + (2 * checkBoxOrRadioButtonFocusStrokeWidth);
constexpr int radioButtonIndicatorStrokeWidth = 6;

constexpr int textCursorWidth = 1;

constexpr int itemViewItemHorizontalMargin = 16;
constexpr int itemViewItemVerticalMargin = 8;
constexpr int itemViewItemWithDecorationVerticalMargin = 10;
constexpr int itemViewItemSpacing = 8;
constexpr int itemViewSeparatorHeight = 5;

constexpr int focusFrameWidth = 2;
constexpr int focusFrameCornerRadius = 16;

constexpr int headerMargin = 16;
constexpr int headerDefaultSectionSizeVertical = 48;

constexpr double lineEditCornerRadius = 32.0;
constexpr int lineEditHorizontalMargin = 16;
constexpr int lineEditVerticalMargin = 9;

constexpr int menuBarItemSpacing = 16;
constexpr int menuBarVerticalMargin = 8;
constexpr int menuBarHorizontalMargin = 8;

constexpr int menuItemVerticalMargin = 8;
constexpr int menuItemCornerRadius = 4;
constexpr int menuItemHoverHorizontalMargin = 4;

constexpr int pushButtonHorizontalMargin = 16;
constexpr int pushButtonVerticalMargin = 8;
constexpr int pushButtonCornerRadius = 8;
constexpr int pushButtonFocusFrameGap = 2;
constexpr int pushButtonFocusFrameWidth = 2;

constexpr int progressBarMinHeight = 24;
constexpr int progressBarGrooveHeight = 4;
constexpr int progressBarCornerRadius = 2;

constexpr int shadowXSRadius = 22;

constexpr int sliderThickness = 40;
constexpr int sliderGrooveHeight = 4;
constexpr int sliderCornerRadius = 2;
constexpr int sliderThumbSize = 16;
constexpr int sliderHoverSize = 4;
constexpr int sliderFocusStrokeWidth = 2;

constexpr int smallIconSize = 12;

constexpr int splitterHandleWidth = 6;
constexpr int splitterWidth = 12;

constexpr int titleBarHeight = 46;

constexpr int toolButtonCornerRadius = 4;
constexpr int toolButtonFocusFrameWidth = 2;

constexpr int toolTipCornerRadius = 8;
constexpr int toolTipFrameWidth = 8;
constexpr int toolTipBackgroundAlpha = 204;

constexpr int spinBoxFrameHeight = 32;
constexpr int spinBoxButtonBoxWidth = 32;
constexpr int spinBoxButtonBoxHeight = 16;
constexpr int spinBoxEditHorizontalMargin = 4;
constexpr double spinBoxFrameRadius = 12.0;
constexpr int spinBoxFrameStroke = 2;

constexpr int maxScrollBarSize = 9;
constexpr int thinScrollBarSize = 3;
constexpr int thickScrollBarSize = 6;

constexpr int tabBarTabRadius = 8;
constexpr int tabBarTabFrameWidth = 2;
constexpr int tabBarTabUnderLineWidth = 2;

constexpr int groupBoxBottomPadding = 8;
constexpr int groupBoxFrameCornerRadius = 8;
constexpr int groupBoxHorizontalPadding = 12;
constexpr int groupBoxTitleTextFontSize = 20;
constexpr int groupBoxTitleToContentSpacing = 8;
constexpr int groupBoxTopPadding = 24;

constexpr int comboBoxVerticalMargin = 4;
constexpr int comboBoxFrameRadius = 8;
constexpr int comboBoxFrameLeftRightPadding = 16;
constexpr int comboBoxFocusFrameWidth = 2;

constexpr int tabWidgetFrameWidth = 1;

constexpr double ohos_id_alpha_disabled = 0.4;
constexpr double ohos_id_alpha_content_tertiary = 0.4;

QColor makeInactiveOrDisabledFromColor(const QColor &color)
{
    auto disabledColor = color;
    disabledColor.setAlphaF(ohos_id_alpha_disabled);

    return disabledColor;
}

template<typename T>
bool qobjectIsInstanceOf(const QObject *obj)
{
    return qobject_cast<const T *>(obj) != nullptr;
}

template<typename T>
bool qStyleOptionIs(const QStyleOption *option)
{
    return qstyleoption_cast<const T *>(option) != nullptr;
}

bool isItemViewSeparator(const QModelIndex &index)
{
    return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
}

bool isFirstItem(const QModelIndex &index)
{
    return index.isValid() && index.row() == 0 && index.column() == 0;
}

bool isLastItem(const QModelIndex &index)
{
    return index.isValid()
        && index.row() == index.model()->rowCount(index.parent()) - 1
        && index.column() == index.model()->columnCount(index.parent()) - 1;
}

bool isHoverable(const QWidget *widget)
{
    static bool (* const hoverTypeCheckFuncs[])(const QObject *obj) = {
        &qobjectIsInstanceOf<QCheckBox>,
        &qobjectIsInstanceOf<QPushButton>,
        &qobjectIsInstanceOf<QRadioButton>,
        &qobjectIsInstanceOf<QSlider>,
        &qobjectIsInstanceOf<QToolButton>,
        &qobjectIsInstanceOf<QComboBox>,
        &qobjectIsInstanceOf<QScrollBar>,
    };

    return std::any_of(
        std::begin(hoverTypeCheckFuncs), std::end(hoverTypeCheckFuncs),
        [&](const auto &typeCheckFunc) {
            return typeCheckFunc(widget);
        });
}

QRect adjusted(const QRect &rect, int growth)
{
    return rect.adjusted(-growth, -growth, growth, growth);
}

QColor getSunkenOrHoverColor(bool isSunken, const QPalette &palette)
{
    return isSunken
        ? palette.color(QPalette::Active, QPalette::Dark)
        : palette.color(QPalette::Active, QPalette::Light);
}

void paintOnStackTop(QPainter &painter, const std::function<void(QPainter &painter)> &paint)
{
    painter.save();
    paint(painter);
    painter.restore();
}

void drawLine(QPainter &painter, const QPen &pen, const QPoint &aPoint, const QPoint &bPoint)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.setPen(pen);
        painter.drawLine(aPoint, bPoint);
    });
}

void drawEllipse(QPainter &painter, const QPen &pen, const QBrush &brush, const QRect &rect)
{
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawEllipse(rect);
    painter.restore();
}

void drawRect(QPainter &painter, const QPen &pen, const QBrush &brush, const QRect &rect)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRect(rect);
    });
}

void drawRoundedRect(
    QPainter &painter, const QPen &pen, const QBrush &brush, const QRect &rect, qreal xRadius, qreal yRadius)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRoundedRect(rect, xRadius, yRadius);
    });
}

QPainterPath roundedPathInRect(
    const QRect &rect, qreal topLeftCornerRadius, qreal topRightCornerRadius, qreal bottomLeftCornerRadius,
    qreal bottomRightCornerRadius)
{
    QPainterPath path;

    const auto bottom = rect.y() + rect.height();
    const auto right = rect.x() + rect.width();

    const auto topLeftDiameter = 2 * topLeftCornerRadius;
    path.moveTo(rect.x() + topLeftCornerRadius, rect.y());
    path.arcTo(rect.x(), rect.y(), topLeftDiameter, topLeftDiameter, 90.0, 90.0);

    const auto bottomLeftDiameter = 2 * bottomLeftCornerRadius;
    path.lineTo(rect.x(), bottom - bottomLeftCornerRadius);
    path.arcTo(rect.x(), bottom - bottomLeftDiameter, bottomLeftDiameter, bottomLeftDiameter, 180.0, 90.0);

    const auto bottomRightDiameter = 2 * bottomRightCornerRadius;
    path.lineTo(right - bottomRightCornerRadius, bottom);
    path.arcTo(
        right - bottomRightDiameter, bottom - bottomRightDiameter, bottomRightDiameter, bottomRightDiameter, 270.0,
        90.0);

    const auto topRightDiameter = 2 * topRightCornerRadius;
    path.lineTo(right, rect.y() + topRightCornerRadius);
    path.arcTo(right - topRightDiameter, rect.y(), topRightDiameter, topRightDiameter, 360.0, 90.0);

    path.closeSubpath();

    return path;
}

void drawPath(QPainter &painter, const QPen &pen, const QBrush &brush, const QPainterPath& path)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawPath(path);
    });
}

void drawRoundedRect(
    QPainter &painter, const QPen &pen, const QBrush brush, const QRect &rect, qreal topLeftCornerRadius,
    qreal topRightCornerRadius, qreal bottomLeftCornerRadius, qreal bottomRightCornerRadius)
{
    drawPath(
        painter, pen, brush, roundedPathInRect(rect, topLeftCornerRadius, topRightCornerRadius, bottomLeftCornerRadius,
        bottomRightCornerRadius));
}

void drawPolyline(
    QPainter &painter, const QPen &pen, const QBrush &brush, const std::vector<QPointF> &polygonPoints)
{
    paintOnStackTop(painter, [&](auto &p) {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(pen);
        p.setBrush(brush);
        p.drawPolyline(polygonPoints.data(), polygonPoints.size());
    });
}

void drawCheckBoxOrRadioButtonIndicator(QPainter &painter, const QStyleOption &option, bool isCheckBox)
{
    const auto isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const auto isOn = option.state.testFlag(QStyle::State_On);
    const auto isMouseOver = option.state.testFlag(QStyle::State_MouseOver);
    const auto isSunken = option.state.testFlag(QStyle::State_Sunken);

    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);

    painter.setOpacity(isEnabled ? 1.0 : ohos_id_alpha_disabled);

    QRect hoverRect(0, 0, checkBoxOrRadioButtonHoverSize, checkBoxOrRadioButtonHoverSize);
    hoverRect.moveCenter(option.rect.center());
    if (isMouseOver || isSunken)
        drawEllipse(painter, Qt::NoPen, getSunkenOrHoverColor(isSunken, option.palette), hoverRect);

    QRect indicatorRect(0, 0, checkBoxOrRadioButtonIndicatorSize, checkBoxOrRadioButtonIndicatorSize);
    indicatorRect.moveCenter(option.rect.center());

    auto activeColor = option.palette.color(QPalette::Active, QPalette::Button);
    if (isOn) {
        if (isCheckBox) {
            drawEllipse(painter, Qt::NoPen, activeColor, indicatorRect);

            QPixmap svgPixmap(indicatorRect.size());
            if (svgPixmap.load(QString::fromStdString(":/resources/ic_gallery_material_select_checkbo.svg"))) {
                painter.drawPixmap(indicatorRect.topLeft(), svgPixmap);
            }
        } else {
            drawEllipse(
                painter, QPen(activeColor, radioButtonIndicatorStrokeWidth), Qt::NoBrush,
                adjusted(indicatorRect, -radioButtonIndicatorStrokeWidth / 2));
        }
    } else {
        drawEllipse(
            painter, QPen(option.palette.color(QPalette::Inactive, QPalette::Button),
            checkBoxOrRadioButtonIndicatorStrokeWidth), option.palette.color(QPalette::Inactive, QPalette::Base),
            adjusted(indicatorRect, -checkBoxOrRadioButtonIndicatorStrokeWidth));
    }

    painter.restore();
}

void drawLineEditBackground(QPainter &painter, const QStyleOption &option)
{
    auto radius = std::min(
        std::min(option.rect.size().width(), option.rect.size().height()) * 0.5, lineEditCornerRadius);
    drawRoundedRect(painter, Qt::NoPen, option.palette.brush(QPalette::Base), option.rect, radius, radius);
}

QBrush getPushButtonBackgroundBrush(const QStyleOptionButton &option)
{
    const auto isDefaultButton = option.features.testFlag(QStyleOptionButton::DefaultButton);
    const auto isAutoDefaultButton = option.features.testFlag(QStyleOptionButton::AutoDefaultButton);
    const auto isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const auto isOn = option.state.testFlag(QStyle::State_On);

    const auto defaultButtonBackgroundColor = isEnabled
        ? option.palette.color(QPalette::Active, QPalette::AlternateBase)
        : makeInactiveOrDisabledFromColor(option.palette.color(QPalette::Active, QPalette::AlternateBase));

    return isOn
        ? isDefaultButton || isAutoDefaultButton
            ? defaultButtonBackgroundColor
            : option.palette.color(QPalette::Active, QPalette::Dark)
        : isDefaultButton
            ? defaultButtonBackgroundColor
            : option.palette.brush(
                isEnabled
                    ? QPalette::Active
                    : QPalette::Inactive,
                QPalette::Button);
}

QColor getPushButtonTextColor(const QStyleOptionButton &option)
{
    const auto isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const auto isDefaultButton = option.features.testFlag(QStyleOptionButton::DefaultButton);
    const auto isOn = option.state.testFlag(QStyle::State_On);
    const QPalette *themePalette =
        QGuiApplicationPrivate::platformTheme()->palette(QPlatformTheme::Palette::ButtonPalette);

    return isDefaultButton || isOn
        ? themePalette->color(QPalette::Active, QPalette::BrightText)
        : themePalette->color(
            isEnabled
                ? QPalette::Active
                : QPalette::Disabled,
            QPalette::ButtonText);
}

void drawPushButtonBackground(QPainter &painter, const QStyleOptionButton &option)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        const auto isMouseOver = option.state.testFlag(QStyle::State_MouseOver);
        const auto isSunken = option.state.testFlag(QStyle::State_Sunken);

        painter.setRenderHints(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        auto adjustment = pushButtonFocusFrameGap + pushButtonFocusFrameWidth;
        auto rect = option.rect.adjusted(adjustment, adjustment, -adjustment, -adjustment);

        painter.setBrush(getPushButtonBackgroundBrush(option));
        painter.drawRoundedRect(rect, pushButtonCornerRadius, pushButtonCornerRadius);

        if (isMouseOver || isSunken) {
            painter.setBrush(getSunkenOrHoverColor(isSunken, option.palette));
            painter.drawRoundedRect(rect, pushButtonCornerRadius, pushButtonCornerRadius);
        }
    });
}

void drawPushButtonFocus(QPainter &painter, const QStyleOption &option)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(option.palette.color(QPalette::Active, QPalette::Highlight), pushButtonFocusFrameWidth));
        auto cornerRadius = pushButtonCornerRadius + pushButtonFocusFrameGap;
        painter.drawRoundedRect(option.rect, cornerRadius, cornerRadius);
    });
}

QRect getProgressBarGrooveRect(const QStyleOptionProgressBar &option)
{
    auto rect = option.rect;
    const bool horizontal = option.state & QStyle::State_Horizontal;
    if (horizontal) {
        rect.setHeight(progressBarGrooveHeight);
        rect.moveTop(option.rect.center().y());
    } else {
        rect.setWidth(progressBarGrooveHeight);
        rect.moveRight(option.rect.center().x());
    }

    return rect;
}

void drawProgressBarContent(QPainter &painter, const QStyleOptionProgressBar &option)
{
    auto rect = getProgressBarGrooveRect(option);
    const double fraction = static_cast<double>(option.progress - option.minimum) / (option.maximum - option.minimum);
    const bool horizontal = option.state & QStyle::State_Horizontal;
    if (horizontal) {
        rect.setWidth(fraction * rect.width());
        rect.moveLeft(option.invertedAppearance ? option.rect.width() - rect.width() : 0);
    } else {
        rect.setHeight(fraction * rect.height());
        rect.moveTop(option.invertedAppearance ? 0 : option.rect.height() - rect.height());
    }

    drawRoundedRect(
        painter, Qt::NoPen, option.palette.brush(QPalette::Highlight), rect, progressBarCornerRadius,
        progressBarCornerRadius);
}

void drawSliderGroove(QPainter &painter, const QStyleOptionSlider &option)
{
    drawRoundedRect(
        painter, Qt::NoPen, option.palette.color(QPalette::Active, QPalette::Mid),
        option.rect, sliderCornerRadius, sliderCornerRadius);

    QRect fillRect;
    if (option.orientation == Qt::Horizontal) {
        const auto sliderPosition = QStyle::sliderPositionFromValue(
            option.minimum, option.maximum, option.sliderPosition, option.rect.width(), option.upsideDown);
        fillRect = QRect(option.rect.topLeft(), QSize(sliderPosition, option.rect.height()));
    } else {
        const auto sliderPosition = QStyle::sliderPositionFromValue(
            option.minimum, option.maximum, option.sliderPosition, option.rect.height(), option.upsideDown);
        fillRect = QRect(
            option.rect.x(), option.rect.y() + sliderPosition, option.rect.width(), option.rect.height() - sliderPosition);
    }
    drawRoundedRect(
        painter, Qt::NoPen, option.palette.brush(QPalette::Highlight), fillRect, sliderCornerRadius,
        sliderCornerRadius);
}

void drawSliderThumb(QPainter &painter, const QStyleOptionSlider &option)
{
    QRect shadowRect(0, 0, shadowXSRadius, shadowXSRadius);
    shadowRect.moveCenter(option.rect.center());

    QRadialGradient radialGrad(shadowRect.center(), shadowXSRadius * 1.0);
    radialGrad.setColorAt(0.0, Qt::transparent);
    radialGrad.setColorAt(1.0, option.palette.color(QPalette::Active, QPalette::Shadow));

    drawEllipse(painter, Qt::NoPen, QBrush(radialGrad), shadowRect);

    if (option.state.testFlag(QStyle::State_MouseOver))
        drawEllipse(painter, Qt::NoPen, option.palette.color(QPalette::Active, QPalette::Light), option.rect);

    auto adjustedRect = option.rect.adjusted(sliderHoverSize, sliderHoverSize, -sliderHoverSize, -sliderHoverSize);
    drawEllipse(painter, Qt::NoPen, option.palette.color(QPalette::Active, QPalette::AlternateBase), adjustedRect);

    if (option.state.testFlag(QStyle::State_Sunken))
        drawEllipse(painter, Qt::NoPen, option.palette.color(QPalette::Active, QPalette::Dark), adjustedRect);

    if (option.state.testFlag(QStyle::State_HasFocus)) {
        drawEllipse(
            painter, QPen(option.palette.color(QPalette::Active, QPalette::Highlight), sliderFocusStrokeWidth),
            Qt::NoBrush, adjusted(option.rect, -1));
    }
}

void drawToolButton(QPainter &painter, const QStyleOption &option)
{
    const auto isEnabled = option.state.testFlag(QStyle::State_Enabled);
    const auto isMouseOver = option.state.testFlag(QStyle::State_MouseOver);
    const auto isSunken = option.state.testFlag(QStyle::State_Sunken);

    drawRoundedRect(
        painter, Qt::NoPen, option.palette.brush(isEnabled ? QPalette::Active : QPalette::Disabled, QPalette::Button),
        option.rect, toolButtonCornerRadius, toolButtonCornerRadius);

    if (isMouseOver || isSunken) {
        drawRoundedRect(
            painter, Qt::NoPen, getSunkenOrHoverColor(isSunken, option.palette), option.rect, toolButtonCornerRadius,
            toolButtonCornerRadius);
    }
}

void drawToolBarbackground(QPainter &painter, const QStyleOption &option)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.fillRect(option.rect, option.palette.brush(QPalette::Active, QPalette::Window));
    });
}

void drawMenuBarBarbackground(QPainter &painter, const QStyleOption &option)
{
    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setRenderHints(QPainter::Antialiasing);
        painter.fillRect(option.rect, option.palette.brush(QPalette::Active, QPalette::Window));
    });
}

void drawListItemSeparator(QPainter &painter, const QStyleOption &option)
{
    QPoint center = option.rect.center();
    drawLine(
        painter, option.palette.color(QPalette::Active, QPalette::Mid),
        QPoint(option.rect.left(), center.y()), QPoint(option.rect.right(), center.y()));
}

void drawMenuItem(QPainter &painter, const QStyleOptionMenuItem &option, int checkMarkSize, int iconSize)
{
    const auto contentRect = option.rect.adjusted(itemViewItemHorizontalMargin, 0, -itemViewItemHorizontalMargin, 0);
    if (option.menuItemType == QStyleOptionMenuItem::Separator) {
        auto copyOption = option;
        copyOption.rect = contentRect;
        drawListItemSeparator(painter, copyOption);
    } else {
        const bool isSelected = option.state.testFlag(QStyle::State_Selected);
        const bool isSunken = option.state.testFlag(QStyle::State_Sunken);

        if (isSelected || isSunken) {
            drawRoundedRect(
                painter, Qt::NoPen, getSunkenOrHoverColor(isSunken, option.palette),
                option.rect.adjusted(menuItemHoverHorizontalMargin, 0, -menuItemHoverHorizontalMargin, 0),
                menuItemCornerRadius, menuItemCornerRadius);
        }

        const bool isCheckable = option.checkType != QStyleOptionMenuItem::NotCheckable;
        QRect checkMarkRect;
        if (isCheckable) {
            checkMarkRect = contentRect;
            checkMarkRect.setWidth(checkMarkSize);

            auto copyOption = option;
            copyOption.rect = checkMarkRect;
            if (option.checked)
                copyOption.state |= QStyle::State_On;

            drawCheckBoxOrRadioButtonIndicator(painter, copyOption, true);
        }

        const bool hasIcon = !option.icon.isNull();
        QRect iconRect;
        if (hasIcon) {
            iconRect = QRect(contentRect.x(), contentRect.center().y() - iconSize * 0.5, iconSize, iconSize);
            iconRect.moveLeft(checkMarkRect.right() + itemViewItemHorizontalMargin);

            option.icon.paint(
                &painter, iconRect, Qt::AlignCenter,
                option.state.testFlag(QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                option.checked ? QIcon::On : QIcon::Off);
        }

        QRect textRect = contentRect;
        if (isCheckable)
            textRect.setLeft(checkMarkRect.right() + itemViewItemHorizontalMargin);
        if (hasIcon)
            textRect.setLeft(iconRect.right() + itemViewItemHorizontalMargin);
        paintOnStackTop(painter, [&](QPainter &painter) {
            painter.setPen(option.palette.color(QPalette::Text));
            painter.drawText(
                textRect, (Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic), option.text);
        });
    }
}

void drawMenuTearOff(QPainter &painter, const QStyleOption &option)
{
    const auto pointY = option.rect.y() + option.rect.height() / 2;
    drawLine(
        painter, QPen(option.palette.color(QPalette::Text), 1, Qt::DashLine), QPoint(option.rect.x(), pointY),
        QPoint(option.rect.x() + option.rect.width(), pointY)
    );
}

void drawSplitter(QPainter &painter, const QStyleOption &option)
{
    constexpr int splitterAvailableSizeDivider = 12;
    const bool isHorizontal = option.state.testFlag(QStyle::State_Horizontal);

    if (option.rect.width() > 1 && option.rect.height() > 1) {
        const auto handleSize = isHorizontal
            ? QSize(splitterHandleWidth, option.rect.height() / splitterAvailableSizeDivider)
            : QSize(option.rect.width() / splitterAvailableSizeDivider, splitterHandleWidth);
        const auto handleRect = QRect(
            QPoint(
                (option.rect.width() - handleSize.width()) * 0.5,
                (option.rect.height() - handleSize.height()) * 0.5),
            handleSize);
        const auto cornerRadius = std::min(handleSize.width(), handleSize.height()) * 0.5;
        drawRoundedRect(
            painter, Qt::NoPen, option.palette.color(QPalette::Active, QPalette::Mid),
            handleRect, cornerRadius, cornerRadius);
    } else {
        auto endPoint = isHorizontal ? option.rect.bottomLeft() : option.rect.topRight();
        drawLine(painter, option.palette.dark().color(), option.rect.topLeft(), endPoint);
    }
}

void drawSpinBoxFrame(
    QPainter &painter, const QStyleOptionSpinBox &option, const QRect &frameRect, int frameWidth)
{
    auto isEnabled = option.state.testFlag(QStyle::State_Enabled);

    QPen pen =
        option.frame && option.subControls.testFlag(QStyle::SC_SpinBoxFrame)
            ? QPen(option.palette.color(QPalette::Active, QPalette::Mid), frameWidth)
            : Qt::NoPen;

    paintOnStackTop(painter, [&](auto &p) {
        p.setOpacity(isEnabled ? 1.0 : ohos_id_alpha_disabled);
        drawRoundedRect(
            p, pen, option.palette.window(), frameRect, spinBoxFrameRadius, spinBoxFrameRadius);
    });
}

void drawSpinBoxButton(QPainter &painter, const QStyleOptionSpinBox &option, int penWidth)
{
    auto rect = option.rect;
    auto isEnabled = option.state.testFlag(QStyle::State_Enabled);
    auto isPressed = option.state.testFlag(QStyle::State_Sunken);

    QPen pen(option.palette.color(QPalette::Active, QPalette::Mid), penWidth);
    QBrush brush(option.palette.window());

    if (isPressed)
        brush = QBrush(option.palette.highlight());

    qreal rectX = rect.x();
    qreal rectY = rect.y();
    qreal rectWidth = rect.width();
    qreal rectHeight = rect.height();
    qreal cornerRadius = static_cast<qreal>(spinBoxFrameRadius);

    QPainterPath path;
    path.moveTo(rectX, rectY);
    path.lineTo(rectX, rectY + rectHeight);
    if (option.subControls == QStyle::SC_SpinBoxDown) {
        path.lineTo(rectX + rectWidth - cornerRadius, rectY + rectHeight);
        path.arcTo(
            rectX + rectWidth - 2 * cornerRadius, rectY + rectHeight - 2 * cornerRadius,
            2 * cornerRadius, 2 * cornerRadius, 270, 90);
        path.lineTo(rectX + rectWidth, rectY);
    } else {
        path.lineTo(rectX + rectWidth, rectY + rectHeight);
        path.lineTo(rectX + rectWidth, rectY + cornerRadius);
        path.arcTo(rectX + rectWidth - 2 * cornerRadius, rectY, 2 * cornerRadius, 2 * cornerRadius, 0, 90);
    }
    path.closeSubpath();

    paintOnStackTop(painter, [&](auto &p) {
        p.setOpacity(isEnabled ? 1.0 : ohos_id_alpha_disabled);
        drawPath(p, pen, brush, path);
    });
}

void drawScrollBarSlider(QPainter &painter, const QStyleOptionSlider &option)
{
    int scrollBarRadius =
        option.orientation == Qt::Vertical
            ? option.rect.width() * 0.5
            : option.rect.height() * 0.5;

    paintOnStackTop(painter, [&](QPainter &painter) {
        painter.setOpacity(ohos_id_alpha_content_tertiary);
        drawRoundedRect(
            painter, Qt::NoPen, option.palette.color(QPalette::Active, QPalette::Midlight),
            option.rect, scrollBarRadius, scrollBarRadius);
    });
}

Qt::Orientation tabBarShapeToOrientation(QTabBar::Shape shape)
{
    switch (shape) {
    case QTabBar::RoundedNorth:
    case QTabBar::RoundedSouth:
    case QTabBar::TriangularNorth:
    case QTabBar::TriangularSouth:
        return Qt::Horizontal;
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        return Qt::Vertical;
    }
}

void drawTabBarTabShape(QPainter &painter, const QStyleOptionTab &option)
{
    auto isSelected = option.state.testFlag(QStyle::State_Selected);
    auto isFocused = option.state.testFlag(QStyle::State_HasFocus);
    QRect rect(option.rect);
    adjusted(rect, -tabBarTabFrameWidth - 1);

    QPoint underLineStart, underLineEnd;
    if (tabBarShapeToOrientation(option.shape) == Qt::Horizontal) {
        int lineY = rect.y() + rect.height() - tabBarTabUnderLineWidth;
        underLineStart = QPoint(rect.x() + tabBarTabRadius, lineY);
        underLineEnd = QPoint(rect.x() + rect.width() - tabBarTabRadius, lineY);
    } else {
        int lineX = rect.x();

        if (option.shape == QTabBar::RoundedWest || option.shape == QTabBar::TriangularWest)
            lineX += rect.width() - tabBarTabUnderLineWidth;
        else
            lineX += tabBarTabUnderLineWidth;

        underLineStart = QPoint(lineX, rect.y() + tabBarTabRadius);
        underLineEnd = QPoint(lineX, rect.y() + rect.height() - tabBarTabRadius);
    }

    drawRoundedRect(painter, Qt::NoPen, option.palette.window(), rect, tabBarTabRadius, tabBarTabRadius);
    if (!isFocused && isSelected)
        drawLine(painter, QPen(option.palette.highlight(), tabBarTabUnderLineWidth), underLineStart, underLineEnd);
}

void drawComboBoxFrame(QPainter &painter, const QStyleOptionComboBox &option)
{
    const auto isMouseOver = option.state.testFlag(QStyle::State_MouseOver);
    const auto isOpened = option.state.testFlag(QStyle::State_On);
    const auto isFocused = option.state.testFlag(QStyle::State_HasFocus);

    auto focusedPen = QPen(
        option.palette.color(QPalette::Active, QPalette::AlternateBase),
        comboBoxFocusFrameWidth);

    drawRoundedRect(
        painter, isFocused ? focusedPen : Qt::NoPen, option.palette.brush(QPalette::Window), option.rect,
        comboBoxFrameRadius, comboBoxFrameRadius);

    if (isMouseOver || isOpened) {
        auto pen = isOpened ? focusedPen : Qt::NoPen;
        auto brush = getSunkenOrHoverColor(isOpened, option.palette);
        drawRoundedRect(painter, pen, brush, option.rect, comboBoxFrameRadius, comboBoxFrameRadius);
    }
}

void drawComboBoxListViewItem(QPainter &painter, const QStyleOptionViewItem &option, const QBrush &brush)
{
    if (isFirstItem(option.index)) {
        drawRoundedRect(
            painter, Qt::NoPen, brush, option.rect, focusFrameCornerRadius, focusFrameCornerRadius, 0, 0);
    } else if (isLastItem(option.index)) {
        drawRoundedRect(
            painter, Qt::NoPen, brush, option.rect, 0, 0, focusFrameCornerRadius, focusFrameCornerRadius);
    } else {
        drawRect(painter, Qt::NoPen, brush, option.rect);
    }
}

void drawTreeViewHighlightItem(QPainter &painter, const QStyleOptionViewItem &option, const QBrush &brush)
{
    switch (option.viewItemPosition) {
    case QStyleOptionViewItem::OnlyOne:
        drawRoundedRect(
            painter, Qt::NoPen, brush, option.rect, focusFrameCornerRadius, focusFrameCornerRadius);
        break;
    case QStyleOptionViewItem::Beginning:
        drawRoundedRect(
            painter, Qt::NoPen, brush, option.rect, focusFrameCornerRadius, 0, focusFrameCornerRadius, 0);
        break;
    case QStyleOptionViewItem::Invalid:
    case QStyleOptionViewItem::Middle:
        drawRect(painter, Qt::NoPen, brush, option.rect);
        break;
    case QStyleOptionViewItem::End:
        drawRoundedRect(
            painter, Qt::NoPen, brush, option.rect, 0, focusFrameCornerRadius, 0, focusFrameCornerRadius);
        break;
    }
}

void drawPanelItemViewItem(QPainter &painter, const QStyleOptionViewItem &option, bool isComboBoxListViewItem)
{
    if (isItemViewSeparator(option.index)) {
        auto copyOption = option;
        copyOption.rect = option.rect.adjusted(itemViewItemHorizontalMargin, 0, -itemViewItemHorizontalMargin, 0);
        drawListItemSeparator(painter, copyOption);
    } else {
        auto isSelected = option.state.testFlag(QStyle::State_Selected);
        auto isMouseOver = option.state.testFlag(QStyle::State_MouseOver);

        auto colorGroup = option.state.testFlag(QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        if (colorGroup == QPalette::Normal && !option.state.testFlag(QStyle::State_Active))
            colorGroup = QPalette::Inactive;

        const QBrush brush =
            isSelected
                ? option.palette.brush(colorGroup, QPalette::Highlight)
                : isMouseOver
                    ? option.palette.color(QPalette::Active, QPalette::Light)
                    : option.backgroundBrush.style() == Qt::NoBrush
                        ? Qt::NoBrush
                        : option.backgroundBrush;

        if (isComboBoxListViewItem)
            drawComboBoxListViewItem(painter, option, brush);
        else
            drawTreeViewHighlightItem(painter, option, brush);
    }
}

void drawHeaderSection(QPainter &painter, const QStyleOptionHeader &option)
{
    QRect rect =
        option.position == QStyleOptionHeader::OnlyOneSection
            ? option.rect
            : option.orientation == Qt::Horizontal
                ? option.rect.adjusted(-1, 0, 0, 0)
                : option.rect.adjusted(0, -1, 0, 0);

    drawRect(painter, QPen(Qt::black), option.palette.brush(QPalette::Button), rect);
}

void drawWindowBackgroundAndFrame(
    QPainter &painter, const QStyleOptionFrame &option, QFlags<Qt::WindowState> state, int titleBarHeight)
{
    const auto offset = option.lineWidth * 0.5;
    const auto windowFrameColor =
        state.testFlag(Qt::WindowActive)
            ? option.palette.color(QPalette::Active, QPalette::Base)
            : option.palette.color(QPalette::Inactive, QPalette::Base);
    if (state.testFlag(Qt::WindowMinimized)) {
        drawRoundedRect(
            painter, QPen(windowFrameColor, option.lineWidth), Qt::NoBrush,
            option.rect.adjusted(offset, offset, -offset, -offset), focusFrameCornerRadius, focusFrameCornerRadius, 0,
            0);
        drawRect(
            painter, Qt::NoPen, option.palette.brush(QPalette::Window),
            option.rect.adjusted(option.lineWidth, titleBarHeight, -option.lineWidth, 0));
    } else {
        drawRoundedRect(
            painter, Qt::NoPen, windowFrameColor, option.rect.adjusted(0, titleBarHeight, 0, 0), 0, 0,
            focusFrameCornerRadius, focusFrameCornerRadius);
        drawRoundedRect(
            painter, Qt::NoPen, option.palette.brush(QPalette::Window),
            option.rect.adjusted(option.lineWidth, titleBarHeight, -option.lineWidth, -option.lineWidth),
            focusFrameCornerRadius, focusFrameCornerRadius);
        drawRoundedRect(
            painter, QPen(windowFrameColor, option.lineWidth), Qt::NoBrush,
            option.rect.adjusted(offset, offset, -offset, -offset), focusFrameCornerRadius, focusFrameCornerRadius);
    }
}

void drawToolTip(QPainter &painter, const QStyleOptionFrame &option)
{
    drawRoundedRect(
        painter, option.palette.color(QPalette::Active, QPalette::Base),
        option.palette.toolTipBase(), option.rect, toolTipCornerRadius,
        toolTipCornerRadius);
}

QRect titleBarSubControlRect(const QStyleOptionTitleBar &option, QCommonStyle::SubControl subControl)
{
    const bool isMinimized = (option.titleBarState & Qt::WindowMinimized) != 0;
    const bool isMaximized = (option.titleBarState & Qt::WindowMaximized) != 0;

    const bool hasTitle = option.titleBarFlags.testFlag(Qt::WindowTitleHint);
    const bool hasSystemMenuButton = option.titleBarFlags.testFlag(Qt::WindowSystemMenuHint);
    const bool hasShadeButton = option.titleBarFlags.testFlag(Qt::WindowShadeButtonHint);
    const bool hasContextHelpButton = option.titleBarFlags.testFlag(Qt::WindowContextHelpButtonHint);
    const bool hasMinimizeButton = option.titleBarFlags.testFlag(Qt::WindowMinimizeButtonHint);
    const bool hasMaximizeButton = option.titleBarFlags.testFlag(Qt::WindowMaximizeButtonHint);

    const auto controlSize = QSize(30, 30);
    const int titleBarHorizontalMargin = 25;
    const int titleBarControlVerticalMargin = 15;

    const auto titleBarContentRect = QRect(
        option.rect.x() + titleBarHorizontalMargin, (option.rect.height() - controlSize.height()) / 2,
        option.rect.width() - 2 * titleBarHorizontalMargin, controlSize.height());
    const auto titleBarContentRectRight = titleBarContentRect.x() + titleBarContentRect.width();
    const auto delta = controlSize.width() + titleBarControlVerticalMargin;

    QRect result;
    switch (subControl) {
    case QCommonStyle::SC_TitleBarSysMenu:
        if (hasSystemMenuButton)
            result = QRect(titleBarContentRect.topLeft(), controlSize);
        break;
    case QCommonStyle::SC_TitleBarLabel:
        if (hasTitle) {
            result = titleBarContentRect;
            if (hasSystemMenuButton)
                result.adjust(delta, 0, -delta, 0);
            if (hasMinimizeButton)
                result.adjust(0, 0, -delta, 0);
            if (hasMaximizeButton)
                result.adjust(0, 0, -delta, 0);
            if (hasShadeButton)
                result.adjust(0, 0, -delta, 0);
            if (hasContextHelpButton)
                result.adjust(0, 0, -delta, 0);
        }
        break;
    case QCommonStyle::SC_TitleBarMaxButton:
        if (!isMaximized && hasMaximizeButton) {
            result = QRect(
                QPoint(titleBarContentRectRight - 3 * delta + titleBarControlVerticalMargin,
                titleBarContentRect.y()), controlSize);
        }
        break;
    case QCommonStyle::SC_TitleBarNormalButton:
        if (isMinimized && hasMinimizeButton) {
            result = QRect(
                QPoint(titleBarContentRectRight - 2 * delta + titleBarControlVerticalMargin,
                titleBarContentRect.y()), controlSize);
        } else if (isMaximized && hasMaximizeButton) {
            result = QRect(
                QPoint(titleBarContentRectRight - 3 * delta + titleBarControlVerticalMargin,
                titleBarContentRect.y()), controlSize);
        }
        break;
    case QCommonStyle::SC_TitleBarMinButton:
        if (!isMinimized && hasMinimizeButton) {
            result = QRect(
                QPoint(titleBarContentRectRight - 2 * delta + titleBarControlVerticalMargin,
                titleBarContentRect.y()), controlSize);
        }
        break;
    case QCommonStyle::SC_TitleBarCloseButton:
        result = QRect(
            QPoint(titleBarContentRectRight - controlSize.width(), titleBarContentRect.y()), controlSize);
        break;
    case QCommonStyle::SC_TitleBarShadeButton:
    case QCommonStyle::SC_TitleBarUnshadeButton:
        result = QRect(
            QPoint(titleBarContentRectRight - 4 * delta + titleBarControlVerticalMargin,
            titleBarContentRect.y()), controlSize);
        break;
    case QCommonStyle::SC_TitleBarContextHelpButton:
        result = QRect(
            QPoint(titleBarContentRectRight - (hasShadeButton ? 5 : 4) * delta + titleBarControlVerticalMargin,
            titleBarContentRect.y()), controlSize);
        break;
    default:
        break;
    }

    return result;
}

}

QOhosStyle::QOhosStyle()
    : QCommonStyle()
{
}

void QOhosStyle::drawPrimitive(
    PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case PE_Frame: {
        if (widget != nullptr && qobjectIsInstanceOf<QComboBox>(widget->parent()))
            break;
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
    case PE_FrameWindow:
        if (qStyleOptionIs<QStyleOptionFrame>(option)) {
            drawWindowBackgroundAndFrame(
                *painter, *qstyleoption_cast<const QStyleOptionFrame *>(option), widget->windowState(),
                pixelMetric(PM_TitleBarHeight, option, widget));
        }
        break;
    case PE_FrameFocusRect:
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
            if (qobjectIsInstanceOf<QPushButton>(widget)) {
                drawPushButtonFocus(*painter, *option);
            } else if (qobjectIsInstanceOf<QToolButton>(widget)) {
                const auto menuButtonIndicatorSize =
                    qobject_cast<const QToolButton *>(widget)->popupMode() == QToolButton::MenuButtonPopup
                        ? pixelMetric(PM_MenuButtonIndicator, option, widget)
                        : 0;
                drawRoundedRect(
                    *painter,
                    QPen(option->palette.color(QPalette::Active, QPalette::Highlight), toolButtonFocusFrameWidth),
                    Qt::NoBrush, option->rect.adjusted(-1, -1, 1 + menuButtonIndicatorSize, 1),
                    toolButtonCornerRadius, toolButtonCornerRadius);
            } else if (qobjectIsInstanceOf<QCheckBox>(widget) || qobjectIsInstanceOf<QRadioButton>(widget)) {
                drawEllipse(
                    *painter, QPen(option->palette.color(QPalette::Active, QPalette::Button),
                    checkBoxOrRadioButtonFocusStrokeWidth), Qt::NoBrush, option->rect);
            } else if (qobjectIsInstanceOf<QTabBar>(widget)) {
                drawRoundedRect(
                    *painter, QPen(option->palette.highlight(), tabBarTabFrameWidth), Qt::NoBrush,
                    option->rect, tabBarTabRadius, tabBarTabRadius);
            } else {
                drawRoundedRect(
                    *painter, QPen(option->palette.color(QPalette::Active, QPalette::Highlight), focusFrameWidth),
                    Qt::NoBrush, option->rect, focusFrameCornerRadius, focusFrameCornerRadius);
            }
        }
        break;
    case PE_PanelButtonCommand:
        if (qStyleOptionIs<QStyleOptionButton>(option))
            drawPushButtonBackground(*painter, *qstyleoption_cast<const QStyleOptionButton *>(option));
        break;
    case PE_PanelButtonTool:
        if (qobjectIsInstanceOf<QMdiSubWindow>(widget)) {
            const QPalette *toolButtonThemePalette =
                QGuiApplicationPrivate::platformTheme()->palette(QPlatformTheme::Palette::ToolButtonPalette);
            const auto panelButtonColor =
                toolButtonThemePalette->color(QPalette::Active, QPalette::AlternateBase);
            drawEllipse(
                *painter, Qt::NoPen,
                widget->windowState().testFlag(Qt::WindowActive)
                    ? panelButtonColor
                    : makeInactiveOrDisabledFromColor(panelButtonColor),
                option->rect);
        } else {
            const QToolButton *toolButton = qobject_cast<const QToolButton *>(widget);
            const auto menuButtonIndicatorSize =
                toolButton != nullptr && toolButton->popupMode() == QToolButton::MenuButtonPopup
                    ? pixelMetric(PM_MenuButtonIndicator, option, widget)
                    : 0;
            auto optionCopy = *option;
            optionCopy.rect.setWidth(option->rect.width() + menuButtonIndicatorSize);
            drawToolButton(*painter, optionCopy);
        }
        break;
    case PE_PanelMenuBar:
        drawMenuBarBarbackground(*painter, *option);
        break;
    case PE_IndicatorCheckBox:
        drawCheckBoxOrRadioButtonIndicator(*painter, *option, true);
        break;
    case PE_IndicatorRadioButton:
        drawCheckBoxOrRadioButtonIndicator(*painter, *option, false);
        break;
    case PE_PanelLineEdit: {
        // HACK: Q(Abstrace)SpinBox contains a QLineEdit inside, which is painted separately from
        // the QSpinBox itself. We will be drawing the whole QSpinBox in the drawComplexControl.
        if (widget != nullptr && qobjectIsInstanceOf<QAbstractSpinBox>(widget->parentWidget()))
            break;

        // HACK: QComboBox in editable mode presents line edit in the center. This line edit is drawn
        // above QComboBox. There are two issues here: QComboBox has different base color than line edit.
        // Second, line edit and QComboBox has the same size - so drawing a line edit overlays a frame border.
        // To fix above, do not draw line edit background.
        if (widget != nullptr && qobjectIsInstanceOf<QComboBox>(widget->parentWidget()))
            break;

        // HACK: while editing Tree View/Widget item - line edit component is presented above it. If the line edit has
        // some opacity in background content below it (item content) is visible. This causes an effect that original
        // text is still visible while editing. To fix it, override background (base) color and use solid color value.
        auto optionCopy = *option;
        if (widget != nullptr && qstrcmp(widget->metaObject()->className(), "QExpandingLineEdit") == 0) {
            optionCopy.palette.setColor(
                QPalette::Base,
                option->palette.color(QPalette::Active, QPalette::AlternateBase));
        }

        drawLineEditBackground(*painter, optionCopy);
        break;
    }
    case PE_PanelButtonBevel:
        if (qStyleOptionIs<QStyleOptionSpinBox>(option)) {
            drawSpinBoxButton(
                *painter, *qstyleoption_cast<const QStyleOptionSpinBox *>(option),
                pixelMetric(PM_SpinBoxFrameWidth, option, widget));
        }
        break;
    case PE_IndicatorSpinDown:
    case PE_IndicatorSpinUp: {
        // HACK: QCommonStyle adjusts the indicator left as follows: copy.rect.adjust(3, 0, -4, 0);
        // However, Ohos requires 4 pixels of horizontal margin and 2 pixels of vertical margin
        QRect indicatorRect = option->rect.adjusted(1, 2, 0, -2);
        constexpr int arrowWidth = 2;
        auto x = indicatorRect.x();
        auto y = indicatorRect.y();
        auto width = indicatorRect.width();
        auto height = indicatorRect.height();
        auto verticalSpacing = 4;
        auto horizontalSpacing = 7;

        qreal x1 = x + horizontalSpacing;
        qreal x2 = x + width * 0.5;
        qreal x3 = x + width - horizontalSpacing;

        qreal y1, y2;
        if (element == PE_IndicatorSpinDown) {
            y1 = y + verticalSpacing;
            y2 = y + height - verticalSpacing;
        } else {
            y1 = y + height - verticalSpacing;
            y2 = y + verticalSpacing;
        }
        auto y3 = y1;

        QPen indicatorPen(option->palette.color(QPalette::Active, QPalette::Text), arrowWidth);
        indicatorPen.setCapStyle(Qt::RoundCap);
        indicatorPen.setJoinStyle(Qt::RoundJoin);

        paintOnStackTop(*painter, [&](auto &p) {
            p.setOpacity(option->state.testFlag(State_Enabled) ? 1.0 : ohos_id_alpha_disabled);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(indicatorPen);
            p.setBrush(Qt::NoBrush);
            p.drawLine(x1, y1, x2, y2);
            p.drawLine(x2, y2, x3, y3);
        });
        break;
    }
    case PE_FrameGroupBox: {
        const auto frame = qstyleoption_cast<const QStyleOptionFrame *>(option);
        if (frame != nullptr) {
            QPen pen;
            pen.setColor(frame->palette.alternateBase().color());
            pen.setStyle(
                frame->features.testFlag(QStyleOptionFrame::FrameFeature::Flat)
                ? Qt::NoPen : Qt::SolidLine);

            painter->setClipRegion(frame->rect);
            drawRoundedRect(
                *painter, pen, option->palette.window(), frame->rect, groupBoxFrameCornerRadius,
                groupBoxFrameCornerRadius);
        }
        break;
    }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        QRect availableRect;
        int maxArrowSize = 0;
        if (qobjectIsInstanceOf<QToolButton>(widget) || qobjectIsInstanceOf<QPushButton>(widget)
            || qobjectIsInstanceOf<QComboBox>(widget)) {
            const auto menuButtonIndicatorSize = pixelMetric(PM_MenuButtonIndicator, option, widget);
            availableRect = QRect(
                widget->size().width() - menuButtonIndicatorSize, 0, menuButtonIndicatorSize, widget->size().height());
            maxArrowSize = std::min(availableRect.width(), availableRect.height()) * 0.5;
        } else {
            availableRect = option->rect;
            maxArrowSize = std::min(availableRect.width(), availableRect.height());
        }

        const auto arrowWidth = 2;
        auto arrowRect = QRect(0, 0, maxArrowSize, maxArrowSize * 0.5);
        arrowRect.moveCenter(availableRect.center());
        arrowRect = adjusted(arrowRect, -arrowWidth * 0.5);

        const std::vector<QPointF> polygonPoints = {
            QPointF(arrowRect.x(), arrowRect.y()),
            QPointF(arrowRect.x() + arrowRect.width() * 0.5, arrowRect.y() + arrowRect.height()),
            QPointF(arrowRect.x() + arrowRect.width(), arrowRect.y())
        };
        const auto isEnabled = option->state.testFlag(QStyle::State_Enabled);

        paintOnStackTop(*painter, [&](auto &p) {
            p.translate(option->rect.center());
            switch (element) {
            default:
            case PE_IndicatorArrowDown:
                break;
            case PE_IndicatorArrowUp:
                p.rotate(180);
                break;
            case PE_IndicatorArrowLeft:
                p.rotate(90);
                break;
            case PE_IndicatorArrowRight:
                p.rotate(-90);
                break;
            }
            p.translate(-option->rect.center());

            p.setOpacity(isEnabled ? 1.0 : ohos_id_alpha_disabled);
            drawPolyline(
                p,
                QPen(
                    option->palette.color(QPalette::Active, QPalette::WindowText),
                    arrowWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin), Qt::NoBrush,
                polygonPoints);
        });
        break;
    }
    case PE_IndicatorButtonDropDown: {
        if (qobjectIsInstanceOf<QToolButton>(widget))
            break;
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
    case PE_PanelItemViewItem:
        if (qStyleOptionIs<QStyleOptionViewItem>(option)) {
            drawPanelItemViewItem(
                *painter, *qstyleoption_cast<const QStyleOptionViewItem *>(option),
                widget != nullptr && qstrcmp(widget->metaObject()->className(), "QComboBoxListView") == 0);
        }
        break;
    case PE_FrameTabWidget:
        if (qStyleOptionIs<QStyleOptionTabWidgetFrame>(option)) {
            QRect rect = option->rect;

            switch (qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)->shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                rect.adjust(0, tabWidgetFrameWidth, 0, 0);
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                rect.adjust(tabWidgetFrameWidth, 0, 0, 0);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                rect.adjust(0, 0, 0, -(tabWidgetFrameWidth * 2));
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                rect.adjust(0, 0, -(tabWidgetFrameWidth * 2), 0);
                break;
            }

            drawRect(
                *painter, QPen(option->palette.color(QPalette::Dark), tabWidgetFrameWidth),
                Qt::NoBrush, rect);
        }
        break;
    case PE_PanelTipLabel:
        if (qStyleOptionIs<QStyleOptionFrame>(option))
            drawToolTip(*painter, *qstyleoption_cast<const QStyleOptionFrame *>(option));
        break;
    default:
        QCommonStyle::drawPrimitive(element, option, painter, widget);
        break;
    }
}

void QOhosStyle::drawControl(
    ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_HeaderSection:
        if (qStyleOptionIs<QStyleOptionHeader>(option))
            drawHeaderSection(*painter, *qstyleoption_cast<const QStyleOptionHeader *>(option));
        break;
    case CE_MenuBarEmptyArea:
        break;
    case CE_MenuItem:
        if (qStyleOptionIs<QStyleOptionMenuItem>(option)) {
            const auto *menuItemStyleOption = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
            drawMenuItem(
                *painter, *menuItemStyleOption, pixelMetric(PM_IndicatorWidth, option, widget),
                pixelMetric(PM_SmallIconSize, option, widget));
            if (menuItemStyleOption->menuItemType == QStyleOptionMenuItem::SubMenu) {
                const auto iconSize = pixelMetric(PM_SmallIconSize, option, widget);
                const auto menuItemRect = menuItemStyleOption->rect;

                QStyleOptionMenuItem menuArrowStyleOption = *menuItemStyleOption;
                menuArrowStyleOption.rect = QRect(
                    menuItemRect.right() - itemViewItemHorizontalMargin - iconSize,
                    menuItemRect.top() + (menuItemRect.height() - iconSize) / 2, iconSize, iconSize);
                QColor arrowColor = menuItemStyleOption->palette.color(QPalette::Active, QPalette::Text);
                menuArrowStyleOption.palette.setColor(
                    QPalette::Active, QPalette::WindowText, arrowColor);
                drawPrimitive(
                    menuArrowStyleOption.direction == Qt::RightToLeft ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight,
                    &menuArrowStyleOption, painter, widget);
            }
        }
        break;
    case CE_MenuTearoff:
        drawMenuTearOff(*painter, *option);
        break;
    case CE_ProgressBarGroove:
        if (qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            drawRoundedRect(
                *painter, Qt::NoPen, option->palette.color(QPalette::Active, QPalette::Mid),
                getProgressBarGrooveRect(*qstyleoption_cast<const QStyleOptionProgressBar *>(option)),
                progressBarCornerRadius, progressBarCornerRadius);
        }
        break;
    case CE_ProgressBarContents:
        if (qstyleoption_cast<const QStyleOptionProgressBar *>(option))
            drawProgressBarContent(*painter, *qstyleoption_cast<const QStyleOptionProgressBar *>(option));
        break;
    case CE_ProgressBarLabel:
        break;
    case CE_Splitter:
        drawSplitter(*painter, *option);
        break;
    case CE_ToolBar:
        drawToolBarbackground(*painter, *option);
        break;
    case CE_ScrollBarSlider:
        if (qStyleOptionIs<QStyleOptionSlider>(option))
            drawScrollBarSlider(*painter, *qstyleoption_cast<const QStyleOptionSlider *>(option));
        break;
    case CE_TabBarTabShape: {
        auto tabOption = qstyleoption_cast<const QStyleOptionTab *>(option);
        if (tabOption != nullptr)
            drawTabBarTabShape(*painter, *tabOption);
        break;
    }
    case CE_TabBarTabLabel: {
        auto tabOption = qstyleoption_cast<const QStyleOptionTab *>(option);
        if (tabOption != nullptr) {
            auto copy = *tabOption;
            if (!option->state.testFlag(State_Selected))
                copy.palette.setCurrentColorGroup(QPalette::Inactive);
            QCommonStyle::drawControl(element, &copy, painter, widget);
        }
        break;
    }
    case CE_PushButtonLabel: {
        if (qStyleOptionIs<QStyleOptionButton>(option)) {
            auto buttonOption = qstyleoption_cast<const QStyleOptionButton *>(option);

            auto buttonOptionCopy = *buttonOption;
            buttonOptionCopy.palette.setColor(QPalette::ButtonText, getPushButtonTextColor(*buttonOption));

            QCommonStyle::drawControl(element, &buttonOptionCopy, painter, widget);
        }
        break;
    }
    case CE_DockWidgetTitle: {
        const auto *dockWidgetOption = qstyleoption_cast<const QStyleOptionDockWidget *>(option);
        if (dockWidgetOption == nullptr)
            break;
        auto dockWidgetOptionCopy = *dockWidgetOption;
        if (!dockWidgetOptionCopy.title.isEmpty()) {
            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, option, widget);
            if (dockWidgetOptionCopy.verticalTitleBar) {
                QRect rect = dockWidgetOptionCopy.rect;
                QRect verticalRect = rect.transposed();
                titleRect = QRect(
                    verticalRect.left() + rect.bottom() - titleRect.bottom(),
                    verticalRect.top() + titleRect.left() - rect.left(),
                    titleRect.height(),
                    titleRect.width());
            }
            dockWidgetOptionCopy.title = painter->fontMetrics().elidedText(
                dockWidgetOptionCopy.title, Qt::ElideRight, titleRect.width());
        }
        QCommonStyle::drawControl(element, &dockWidgetOptionCopy, painter, widget);
        break;
    }
    default:
        QCommonStyle::drawControl(element, option, painter, widget);
    }
}

void QOhosStyle::drawComplexControl(
    ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    switch (control) {
    case CC_Slider: {
        const auto sliderStyleOption = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (sliderStyleOption != nullptr) {
            if (option->subControls.testFlag(SC_SliderGroove)) {
                auto sliderStyleOptionCopy = *sliderStyleOption;
                sliderStyleOptionCopy.rect = subControlRect(control, option, SC_SliderGroove, widget);
                drawSliderGroove(*painter, sliderStyleOptionCopy);
            }
            if (option->subControls.testFlag(SC_SliderHandle)) {
                auto sliderStyleOptionCopy = *sliderStyleOption;
                sliderStyleOptionCopy.rect = subControlRect(control, option, SC_SliderHandle, widget);
                drawSliderThumb(*painter, sliderStyleOptionCopy);
            }
        }
        break;
    }
    case CC_SpinBox:
        if (qStyleOptionIs<QStyleOptionSpinBox>(option)) {
            auto spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
            auto frameRect = subControlRect(control, spinBox, SC_SpinBoxFrame, widget);
            auto frameWidth = pixelMetric(PM_SpinBoxFrameWidth, spinBox, widget);
            drawSpinBoxFrame(*painter, *spinBox, frameRect, frameWidth);

            auto copy = *spinBox;
            copy.frame = false;

            QCommonStyle::drawComplexControl(control, &copy, painter, widget);
        }
        break;
    case CC_ScrollBar: {
        const auto scrollBarStyleOption = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (scrollBarStyleOption != nullptr) {
            if (option->subControls.testFlag(SC_ScrollBarSlider)) {
                auto scrollBarStyleOptionCopy = *scrollBarStyleOption;
                scrollBarStyleOptionCopy.rect = subControlRect(control, option, SC_ScrollBarSlider, widget);
                drawControl(CE_ScrollBarSlider, &scrollBarStyleOptionCopy, painter, widget);
            }
        }
        break;
    }
    case CC_GroupBox: {
        const auto groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
        if (groupBox != nullptr) {
            auto groupBoxCopy = *groupBox;
            groupBoxCopy.textAlignment |= Qt::AlignVCenter;

            const auto colorGroup = option->state.testFlag(QStyle::State_Enabled)
                ? QPalette::Active
                : QPalette::Disabled;
            if (!groupBoxCopy.palette.isBrushSet(colorGroup, QPalette::WindowText))
                groupBoxCopy.textColor = groupBoxCopy.palette.color(QPalette::Text);

            QCommonStyle::drawComplexControl(control, &groupBoxCopy, painter, widget);
        }
        break;
    }
    case CC_ComboBox: {
        if (qStyleOptionIs<QStyleOptionComboBox>(option)) {
            const auto comboBoxStyleOption = qstyleoption_cast<const QStyleOptionComboBox *>(option);
            if (option->subControls.testFlag(SC_ComboBoxFrame)) {
                auto comboBoxStyleOptionCopy = *comboBoxStyleOption;
                comboBoxStyleOptionCopy.rect = subControlRect(control, option, SC_ComboBoxFrame, widget);
                drawComboBoxFrame(*painter, comboBoxStyleOptionCopy);
            }

            if (option->subControls.testFlag(SC_ComboBoxArrow)) {
                auto optionCopy = *option;
                auto comboBoxOptions = static_cast<QStyleOption *>(&optionCopy);
                comboBoxOptions->rect = subControlRect(control, option, SC_ComboBoxArrow, widget);
                drawPrimitive(PE_IndicatorArrowDown, comboBoxOptions, painter, widget);
            }
        } else {
            QCommonStyle::drawComplexControl(control, option, painter, widget);
        }
        break;
    }
    default:
        QCommonStyle::drawComplexControl(control, option, painter, widget);
    }
}

QRect QOhosStyle::subControlRect(
    ComplexControl control, const QStyleOptionComplex *option, SubControl subControl, const QWidget *widget) const
{
    switch (control) {
    case CC_Slider:
        if (qStyleOptionIs<QStyleOptionSlider>(option)) {
            const auto sliderStyleOption = qstyleoption_cast<const QStyleOptionSlider *>(option);
            switch (subControl) {
            case SC_SliderGroove: {
                const auto sliderLengthOffset = pixelMetric(PM_SliderLength, option, widget) * 0.5;
                auto grooveRect = QCommonStyle::subControlRect(control, option, subControl, widget);

                if (sliderStyleOption->orientation == Qt::Horizontal) {
                    grooveRect.adjust(sliderLengthOffset, 0, -sliderLengthOffset, 0);
                    grooveRect.setHeight(sliderGrooveHeight);
                    grooveRect.moveTop(option->rect.center().y() - sliderGrooveHeight * 0.5);
                } else {
                    grooveRect.adjust(0, sliderLengthOffset, 0, -sliderLengthOffset);
                    grooveRect.setWidth(sliderGrooveHeight);
                    grooveRect.moveLeft(option->rect.center().x() - sliderGrooveHeight * 0.5);
                }

                return grooveRect;
            }
            case SC_SliderHandle: {
                auto handleRect = QCommonStyle::subControlRect(control, option, subControl, widget);
                if (sliderStyleOption->orientation == Qt::Horizontal)
                    handleRect.moveTop(option->rect.center().y() - handleRect.height() * 0.5);
                else
                    handleRect.moveLeft(option->rect.center().x() - handleRect.width() * 0.5);
                return handleRect;
            }
            default:
                return QCommonStyle::subControlRect(control, option, subControl, widget);
            }
        } else {
            return QCommonStyle::subControlRect(control, option, subControl, widget);
        }
    case CC_SpinBox:
        if (qStyleOptionIs<QStyleOptionSpinBox>(option)) {
            const auto spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
            auto frameWidth = spinBox->frame ? pixelMetric(PM_SpinBoxFrameWidth, option, widget) : 0;

            switch (subControl) {
            case SC_SpinBoxFrame: {
                auto frameRect = spinBox->rect;
                frameRect.setHeight(spinBoxFrameHeight);
                frameRect.moveCenter(spinBox->rect.center());
                return visualRect(spinBox->direction, spinBox->rect, frameRect);
            }
            case SC_SpinBoxDown:
            case SC_SpinBoxUp: {
                auto frameRect = subControlRect(CC_SpinBox, option, SC_SpinBoxFrame, widget);
                if (spinBox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                auto buttonRect = QRect(
                    frameRect.left() + frameRect.width() - spinBoxButtonBoxWidth,
                    frameRect.top() + (frameRect.height() - spinBoxFrameHeight) * 0.5,
                    spinBoxButtonBoxWidth, spinBoxButtonBoxHeight);
                if (subControl == SC_SpinBoxDown)
                    buttonRect.translate(0, spinBoxButtonBoxHeight);
                return visualRect(spinBox->direction, frameRect, buttonRect);
            }
            case SC_SpinBoxEditField: {
                auto editRect = adjusted(spinBox->rect, -frameWidth);
                if (spinBox->buttonSymbols != QAbstractSpinBox::NoButtons) {
                    editRect.setWidth(spinBox->rect.width() - frameWidth - spinBoxButtonBoxWidth);
                    editRect = visualRect(spinBox->direction, spinBox->rect, editRect);
                }
                return editRect;
            }
            default:
                return QCommonStyle::subControlRect(control, option, subControl, widget);
            }
        } else {
            return QCommonStyle::subControlRect(control, option, subControl, widget);
        }
    case CC_ScrollBar:
        if (qStyleOptionIs<QStyleOptionSlider>(option)) {
            auto scrollBoxStyleOption = qstyleoption_cast<const QStyleOptionSlider *>(option);
            auto defaultUpButtonRect = QCommonStyle::subControlRect(control, option, SC_ScrollBarSubLine, widget);
            auto defaultDownButtonRect = QCommonStyle::subControlRect(control, option, SC_ScrollBarAddLine, widget);
            switch (subControl) {
            case SC_ScrollBarAddLine:
            case SC_ScrollBarSubLine:
                return QRect();
            case SC_ScrollBarSubPage: {
                auto subPageRect = QCommonStyle::subControlRect(control, option, subControl, widget);
                if (scrollBoxStyleOption->orientation == Qt::Vertical)
                    subPageRect.moveTop(subPageRect.top() - defaultDownButtonRect.height());
                else
                    subPageRect.moveLeft(subPageRect.left() - defaultDownButtonRect.width());
                return subPageRect;
            }
            case SC_ScrollBarAddPage: {
                auto addPageRect = QCommonStyle::subControlRect(control, option, subControl, widget);
                if (scrollBoxStyleOption->orientation == Qt::Vertical)
                    addPageRect.moveTop(addPageRect.top() + defaultUpButtonRect.height());
                else
                    addPageRect.moveLeft(addPageRect.left() + defaultUpButtonRect.width());
                return addPageRect;
            }
            case SC_ScrollBarSlider: {
                const bool activeScrollBar =
                    option->state.testFlag(QCommonStyle::State_Enabled)
                    && option->state.testFlag(QCommonStyle::State_MouseOver);
                auto scrollBarExtent = activeScrollBar ? thickScrollBarSize : thinScrollBarSize;

                auto sliderRect = QCommonStyle::subControlRect(control, option, subControl, widget);
                if (scrollBoxStyleOption->orientation == Qt::Vertical) {
                    sliderRect.setWidth(scrollBarExtent);
                    sliderRect.setHeight(sliderRect.height() + defaultUpButtonRect.height() + defaultDownButtonRect.height());
                    sliderRect.moveTop(sliderRect.top() - defaultUpButtonRect.height());
                    sliderRect.moveLeft((option->rect.width() - sliderRect.width()) * 0.5);
                } else {
                    sliderRect.setHeight(scrollBarExtent);
                    sliderRect.setWidth(sliderRect.width() + defaultUpButtonRect.width() + defaultDownButtonRect.width());
                    sliderRect.moveTop((option->rect.height() - sliderRect.height()) * 0.5);
                    sliderRect.moveLeft(sliderRect.left() - defaultUpButtonRect.width());
                }
                return sliderRect;
            }
            default:
                return QCommonStyle::subControlRect(control, option, subControl, widget);
            }
        } else {
            return QCommonStyle::subControlRect(control, option, subControl, widget);
        }
    case CC_GroupBox:
        if (qStyleOptionIs<QStyleOptionGroupBox>(option)) {
            auto groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
            auto labelRect =
                QCommonStyle::subControlRect(control, option, SC_GroupBoxLabel, widget);
            if (groupBox->text.isEmpty()) {
                labelRect.moveTop(groupBox->rect.top() + groupBoxTopPadding);
                labelRect.moveBottom(labelRect.top());
            } else {
                labelRect.moveTop(groupBox->rect.top() + groupBoxTopPadding + groupBoxTitleTextFontSize / 2);
            }

            switch (subControl) {
            case SC_GroupBoxFrame:
                return QCommonStyle::subControlRect(control, option, subControl, widget);
            case SC_GroupBoxLabel: {
                return labelRect;
            }
            case SC_GroupBoxCheckBox: {
                auto checkBoxRect =
                    QCommonStyle::subControlRect(control, option, subControl, widget);
                checkBoxRect.moveTop(labelRect.center().y() - checkBoxRect.height() / 2);
                return checkBoxRect;
            }
            case SC_GroupBoxContents: {
                auto contentsRect =
                    QCommonStyle::subControlRect(control, option, SC_GroupBoxContents, widget);
                contentsRect.setTop(labelRect.bottom() + (groupBox->text.isEmpty() ? 0 : groupBoxTitleToContentSpacing));
                contentsRect.setBottom(groupBox->rect.bottom() - groupBoxBottomPadding);
                contentsRect.adjust(groupBoxHorizontalPadding, 0, -groupBoxHorizontalPadding, 0);
                return contentsRect;
            }
            default:
                return QCommonStyle::subControlRect(control, option, subControl, widget);
            }
        } else {
            return QCommonStyle::subControlRect(control, option, subControl, widget);
        }
    case CC_ComboBox:
        if (qStyleOptionIs<QStyleOptionComboBox>(option)) {
            auto comboBoxStyleOption = qstyleoption_cast<const QStyleOptionComboBox *>(option);
            const auto arrowSize = pixelMetric(PM_MenuButtonIndicator, option, widget);
            switch (subControl) {
            case SC_ComboBoxFrame: {
                return adjusted(comboBoxStyleOption->rect, -comboBoxFocusFrameWidth);;
            }
            case SC_ComboBoxEditField: {
                return comboBoxStyleOption->rect.adjusted(
                    comboBoxFrameLeftRightPadding, comboBoxVerticalMargin,
                    -arrowSize - comboBoxFrameLeftRightPadding, -comboBoxVerticalMargin);
            }
            case SC_ComboBoxArrow: {
                auto arrowIconRect = comboBoxStyleOption->rect;
                arrowIconRect.setWidth(arrowSize);
                arrowIconRect.moveRight(comboBoxStyleOption->rect.width());
                return arrowIconRect;
            }
            default:
                return QCommonStyle::subControlRect(control, option, subControl, widget);
            }
        } else {
            return QCommonStyle::subControlRect(control, option, subControl, widget);
        }
    case CC_TitleBar:
        if (qStyleOptionIs<QStyleOptionTitleBar>(option)) {
            auto titleBarOption = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
            return visualRect(
                titleBarOption->direction, titleBarOption->rect, titleBarSubControlRect(*titleBarOption, subControl));
        } else {
            return QCommonStyle::subControlRect(control, option, subControl, widget);
        }
    default:
        return QCommonStyle::subControlRect(control, option, subControl, widget);
    }
}

QRect QOhosStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    switch (element) {
    case SE_PushButtonContents:
        return option->rect.adjusted(pushButtonHorizontalMargin, 0, -pushButtonHorizontalMargin, 0);
    case SE_ItemViewItemFocusRect:
        return option->rect.adjusted(focusFrameWidth, focusFrameWidth, -focusFrameWidth, -focusFrameWidth);
    case SE_ProgressBarGroove:
    case SE_ProgressBarContents:
        return option->rect;
    case SE_ProgressBarLabel:
        return QRect();
    case SE_LineEditContents: {
        int horizontalMargin =
            widget != nullptr && qobjectIsInstanceOf<QAbstractSpinBox>(widget->parent())
                ? spinBoxEditHorizontalMargin
                : lineEditHorizontalMargin;
        return option->rect.adjusted(
            horizontalMargin, lineEditVerticalMargin, -horizontalMargin, -lineEditVerticalMargin);
    }
    case SE_CheckBoxFocusRect:
    case SE_RadioButtonFocusRect:
        return QRect(
            checkBoxOrRadioButtonFocusStrokeWidth, (option->rect.height() - checkBoxOrRadioButtonHoverSize) * 0.5,
            checkBoxOrRadioButtonHoverSize, checkBoxOrRadioButtonHoverSize);
    case SE_CheckBoxClickRect:
        return subElementRect(SE_CheckBoxIndicator, option, widget)
            | subElementRect(SE_CheckBoxContents, option, widget);
    case SE_RadioButtonClickRect:
        return subElementRect(SE_RadioButtonIndicator, option, widget)
            | subElementRect(SE_RadioButtonContents, option, widget);
    default:
        return QCommonStyle::subElementRect(element, option, widget);;
    }
}

QSize QOhosStyle::sizeFromContents(
    ContentsType contents, const QStyleOption *option, const QSize &contentsSize, const QWidget *widget) const
{
    switch (contents) {
    case CT_ItemViewItem: {
        const auto viewItemStyleOption = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        if (viewItemStyleOption != nullptr) {
            if (isItemViewSeparator(viewItemStyleOption->index)) {
                return QSize(option->rect.width(), itemViewSeparatorHeight);
            } else {
                auto verticalMargin = viewItemStyleOption->features.testFlag(QStyleOptionViewItem::HasDecoration)
                    ? itemViewItemWithDecorationVerticalMargin
                    : itemViewItemVerticalMargin;
                return QCommonStyle::sizeFromContents(contents, option, contentsSize, widget)
                    + QSize(0, 2 * verticalMargin);
            }
        } else {
            return QCommonStyle::sizeFromContents(contents, option, contentsSize, widget);
        }
    }
    case CT_PushButton: {
        auto sizeAlignment = (2 * pushButtonFocusFrameGap) + (2 * pushButtonFocusFrameWidth);
        return contentsSize + QSize(
            (pushButtonHorizontalMargin * 2) + sizeAlignment, (pushButtonVerticalMargin * 2) + sizeAlignment);
    }
    case CT_ComboBox: {
        const auto arrowSize = pixelMetric(PM_MenuButtonIndicator, option, widget);
        auto comboBoxSize = qStyleOptionIs<QStyleOptionComboBox>(option)
            && qstyleoption_cast<const QStyleOptionComboBox *>(option)->editable
            ? contentsSize + QSize(2 * lineEditHorizontalMargin, 0)
            : contentsSize;
        const auto frameWidthCompensation = 2 * comboBoxFocusFrameWidth;
        comboBoxSize.rwidth() += (arrowSize + frameWidthCompensation + 2 * comboBoxFrameLeftRightPadding);
        comboBoxSize.rheight() += (frameWidthCompensation + 2 * comboBoxVerticalMargin + 2 * lineEditVerticalMargin);
        return comboBoxSize;
    }
    case CT_ProgressBar: {
        const auto progressBarStyleOption = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        if (progressBarStyleOption != nullptr) {
            const bool horizontal = progressBarStyleOption->state & QStyle::State_Horizontal;
            if (horizontal)
                return QSize(contentsSize.width(), std::max(contentsSize.height(), progressBarMinHeight));
            else
                return QSize(std::max(contentsSize.height(), progressBarMinHeight), contentsSize.height());
        } else {
            return QCommonStyle::sizeFromContents(contents, option, contentsSize, widget);
        }
    }
    case CT_MenuItem: {
        const auto defaultSize = QCommonStyle::sizeFromContents(contents, option, contentsSize, widget);
        const auto *menuItemStyleOption = qstyleoption_cast<const QStyleOptionMenuItem *>(option);
        if (menuItemStyleOption != nullptr) {
            if (menuItemStyleOption->menuItemType == QStyleOptionMenuItem::Separator) {
                return QSize(defaultSize.width(), itemViewSeparatorHeight);
            } else {
                const auto menuVMargin = pixelMetric(PM_MenuVMargin, option, widget);
                const auto arrowWidth = menuItemStyleOption->menuItemType == QStyleOptionMenuItem::SubMenu
                    ? menuVMargin + pixelMetric(PM_SmallIconSize, option, widget)
                    : 0;
                const auto checkboxWidth = menuItemStyleOption->checkType != QStyleOptionMenuItem::NotCheckable
                    ? menuVMargin + pixelMetric(PM_IndicatorWidth, option, widget)
                    : 0;

                return defaultSize
                    + QSize(2 * itemViewItemHorizontalMargin, 2 * menuVMargin)
                    + QSize(arrowWidth, 0)
                    + QSize(checkboxWidth, 0);
            }
        } else {
            return defaultSize;
        }
    }
    case CT_LineEdit: {
        int horizontalMargin;
        if (widget != nullptr && qobjectIsInstanceOf<QAbstractSpinBox>(widget->parent()))
            horizontalMargin = spinBoxEditHorizontalMargin;
        else
            horizontalMargin = lineEditHorizontalMargin;
        return contentsSize + QSize(horizontalMargin * 2, lineEditVerticalMargin * 2);
    }
    case CT_SpinBox: {
        const auto spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
        if (spinBox != nullptr) {
            int buttonWidth;
            if (spinBox->subControls.testFlag(QStyle::SC_SpinBoxUp)
                && spinBox->subControls.testFlag(QStyle::SC_SpinBoxDown)) {
                buttonWidth = subControlRect(CC_SpinBox, spinBox, SC_SpinBoxDown, widget).width();
            } else {
                buttonWidth = 0;
            }
            const int frameWidth
                = spinBox->frame ? pixelMetric(PM_SpinBoxFrameWidth, option, widget) : 0;

            return QSize(contentsSize.width() + buttonWidth + 2 * frameWidth + spinBoxEditHorizontalMargin * 2,
                std::max(contentsSize.height() + 2 * frameWidth, spinBoxFrameHeight));
        }

        return contentsSize;
    }
    default:
        return QCommonStyle::sizeFromContents(contents, option, contentsSize, widget);
    }
}

int QOhosStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_ButtonMargin:
    case PM_ButtonDefaultIndicator:
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        return 0;
    case PM_IndicatorWidth:
    case PM_ExclusiveIndicatorWidth:
        return checkBoxOrRadioButtonSize;
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorHeight:
        return checkBoxOrRadioButtonSize;
    case PM_FocusFrameHMargin:
        return qStyleOptionIs<QStyleOptionViewItem>(option)
            ? itemViewItemSpacing
            : QCommonStyle::pixelMetric(metric, option, widget);
    case PM_TextCursorWidth:
        return textCursorWidth;
    case PM_DefaultFrameWidth:
        if (qobjectIsInstanceOf<QLineEdit>(widget) || qobjectIsInstanceOf<QPushButton>(widget))
            return 0;
        else
            return QCommonStyle::pixelMetric(metric, option, widget);
    case PM_MenuBarPanelWidth:
        return 0;
    case PM_MenuBarItemSpacing:
        return menuBarItemSpacing;
    case PM_MenuBarVMargin:
        return menuBarVerticalMargin;
    case PM_MenuBarHMargin:
        return menuBarHorizontalMargin;
    case PM_MenuPanelWidth:
        return 0;
    case PM_MenuHMargin:
        return 0;
    case PM_MenuVMargin:
        return menuItemVerticalMargin;
    case PM_ProgressBarChunkWidth:
        return 0;
    case PM_SliderThickness:
        return sliderThickness;
    case PM_SliderLength:
    case PM_SliderControlThickness:
        return sliderThumbSize + 2 * sliderHoverSize;
    case PM_SmallIconSize:
        return qStyleOptionIs<QStyleOptionMenuItem>(option) || qStyleOptionIs<QStyleOptionComboBox>(option)
            ? smallIconSize
            : QCommonStyle::pixelMetric(metric, option, widget);
    case PM_SplitterWidth:
        return splitterWidth;
    case PM_SpinBoxFrameWidth:
        return spinBoxFrameStroke;
    case PM_ScrollBarExtent:
        return maxScrollBarSize;
    case PM_TabBarTabShiftVertical:
    case PM_TabBarBaseOverlap:
        return 0;
    case PM_TabBarTabHSpace:
        return 2 * tabBarTabRadius + 2 * tabBarTabFrameWidth;
    case PM_TitleBarHeight:
        return titleBarHeight;
    case PM_HeaderMargin:
        return headerMargin;
    case PM_HeaderDefaultSectionSizeVertical:
        return headerDefaultSectionSizeVertical;
    case PM_ToolTipLabelFrameWidth:
        return toolTipFrameWidth;
    case PM_MenuButtonIndicator:
        return 2 * QCommonStyle::pixelMetric(metric, option, widget);
    default:
        return QCommonStyle::pixelMetric(metric, option, widget);
    }
}

int QOhosStyle::styleHint(
    StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *hintReturn) const
{
    switch (hint) {
    case SH_Slider_AbsoluteSetButtons:
        return Qt::LeftButton | Qt::MiddleButton;
    case SH_Slider_PageSetButtons:
        return 0;
    case SH_ItemView_ShowDecorationSelected:
        return qobjectIsInstanceOf<QTreeView>(widget) ? 0 : 1;
    case SH_ToolTipLabel_Opacity:
        return toolTipBackgroundAlpha;
    case SH_Menu_MouseTracking:
    case SH_MenuBar_MouseTracking:
        return 1;
    default:
        return QCommonStyle::styleHint(hint, option, widget, hintReturn);
    }
}

void QOhosStyle::polish(QWidget *widget)
{
    QCommonStyle::polish(widget);

    if (isHoverable(widget))
        widget->setAttribute(Qt::WA_Hover);

    if (qobjectIsInstanceOf<QPushButton>(widget)) {
        if (qobject_cast<QPushButton *>(widget)->isDefault()) {
            QPalette palette = widget->palette();
            auto textDisabledColor = palette.color(QPalette::Active, QPalette::Base);
            textDisabledColor.setAlphaF(ohos_id_alpha_disabled);
            palette.setColor(QPalette::All, QPalette::ButtonText, palette.color(QPalette::Active, QPalette::Base));
            palette.setColor(QPalette::Disabled, QPalette::ButtonText, textDisabledColor);
            widget->setPalette(palette);
        }
    } else if (qobjectIsInstanceOf<QScrollBar>(widget)) {
        widget->setAttribute(Qt::WA_NoSystemBackground);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    } else if (qobjectIsInstanceOf<QGroupBox>(widget)) {
        QFont font;
        font.setPointSize(groupBoxTitleTextFontSize);
        font.setBold(true);
        widget->setFont(font);

        auto groupBox = qobject_cast<QGroupBox *>(widget);
        groupBox->setAlignment(Qt::AlignHCenter);
    } else if (qobjectIsInstanceOf<QListView>(widget)) {
        qobject_cast<QListView *>(widget)->viewport()->setAttribute(Qt::WA_Hover);
    } else if (qobjectIsInstanceOf<QComboBox>(widget)) {
        auto comboBox = qobject_cast<QComboBox *>(widget);
        const char *delegateClassName = comboBox->itemDelegate()->metaObject()->className();
        bool isDefaultComboBoxDelegate =
            qstrcmp(delegateClassName, "QComboBoxDelegate") == 0
            || qstrcmp(delegateClassName, "QComboMenuDelegate") == 0;
        if (isDefaultComboBoxDelegate)
            comboBox->setItemDelegate(new QStyledItemDelegate(comboBox->view()));
    } else if (qobjectIsInstanceOf<QMdiSubWindow>(widget)) {
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setAutoFillBackground(false);

        auto palette = widget->palette();
        palette.setColor(QPalette::Highlight, palette.color(QPalette::Active, QPalette::Base));
        palette.setColor(QPalette::Inactive, QPalette::Dark, palette.color(QPalette::Inactive, QPalette::Base));
        palette.setColor(
            QPalette::Inactive, QPalette::Window,
            makeInactiveOrDisabledFromColor(palette.color(QPalette::Active, QPalette::AlternateBase)));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        widget->setPalette(palette);
    }
}

void QOhosStyle::unpolish(QWidget *widget)
{
    if (isHoverable(widget)) {
        widget->setAttribute(Qt::WA_Hover, false);
    } else if (qobjectIsInstanceOf<QScrollBar>(widget)) {
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->setAttribute(Qt::WA_OpaquePaintEvent);
    } else if (qobjectIsInstanceOf<QListView>(widget)) {
        qobject_cast<QListView *>(widget)->viewport()->setAttribute(Qt::WA_Hover, false);
    }
    QCommonStyle::unpolish(widget);
}

QPalette QOhosStyle::standardPalette() const
{
    return *QGuiApplicationPrivate::platformTheme()->palette();
}
