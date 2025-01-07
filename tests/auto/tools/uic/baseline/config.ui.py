# -*- coding: utf-8 -*-

# 
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
# 

################################################################################
## Form generated from reading UI file 'config.ui'
##
## Created by: Qt User Interface Compiler version 6.0.0
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QApplication, QCheckBox, QComboBox, QDialog,
    QGridLayout, QGroupBox, QHBoxLayout, QLabel,
    QPushButton, QRadioButton, QSizePolicy, QSlider,
    QSpacerItem, QSpinBox, QVBoxLayout, QWidget)

from gammaview import GammaView

class Ui_Config(object):
    def setupUi(self, Config):
        if not Config.objectName():
            Config.setObjectName(u"Config")
        Config.resize(600, 650)
        Config.setSizeGripEnabled(True)
        self.vboxLayout = QVBoxLayout(Config)
        self.vboxLayout.setSpacing(6)
        self.vboxLayout.setContentsMargins(11, 11, 11, 11)
        self.vboxLayout.setObjectName(u"vboxLayout")
        self.vboxLayout.setContentsMargins(8, 8, 8, 8)
        self.hboxLayout = QHBoxLayout()
        self.hboxLayout.setSpacing(6)
        self.hboxLayout.setObjectName(u"hboxLayout")
        self.hboxLayout.setContentsMargins(0, 0, 0, 0)
        self.ButtonGroup1 = QGroupBox(Config)
        self.ButtonGroup1.setObjectName(u"ButtonGroup1")
        sizePolicy = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.ButtonGroup1.sizePolicy().hasHeightForWidth())
        self.ButtonGroup1.setSizePolicy(sizePolicy)
        self.vboxLayout1 = QVBoxLayout(self.ButtonGroup1)
        self.vboxLayout1.setSpacing(6)
        self.vboxLayout1.setContentsMargins(11, 11, 11, 11)
        self.vboxLayout1.setObjectName(u"vboxLayout1")
        self.vboxLayout1.setContentsMargins(11, 11, 11, 11)
        self.size_176_220 = QRadioButton(self.ButtonGroup1)
        self.size_176_220.setObjectName(u"size_176_220")

        self.vboxLayout1.addWidget(self.size_176_220)

        self.size_240_320 = QRadioButton(self.ButtonGroup1)
        self.size_240_320.setObjectName(u"size_240_320")

        self.vboxLayout1.addWidget(self.size_240_320)

        self.size_320_240 = QRadioButton(self.ButtonGroup1)
        self.size_320_240.setObjectName(u"size_320_240")

        self.vboxLayout1.addWidget(self.size_320_240)

        self.size_640_480 = QRadioButton(self.ButtonGroup1)
        self.size_640_480.setObjectName(u"size_640_480")

        self.vboxLayout1.addWidget(self.size_640_480)

        self.size_800_600 = QRadioButton(self.ButtonGroup1)
        self.size_800_600.setObjectName(u"size_800_600")

        self.vboxLayout1.addWidget(self.size_800_600)

        self.size_1024_768 = QRadioButton(self.ButtonGroup1)
        self.size_1024_768.setObjectName(u"size_1024_768")

        self.vboxLayout1.addWidget(self.size_1024_768)

        self.hboxLayout1 = QHBoxLayout()
        self.hboxLayout1.setSpacing(6)
        self.hboxLayout1.setObjectName(u"hboxLayout1")
        self.hboxLayout1.setContentsMargins(0, 0, 0, 0)
        self.size_custom = QRadioButton(self.ButtonGroup1)
        self.size_custom.setObjectName(u"size_custom")
        sizePolicy1 = QSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.size_custom.sizePolicy().hasHeightForWidth())
        self.size_custom.setSizePolicy(sizePolicy1)

        self.hboxLayout1.addWidget(self.size_custom)

        self.size_width = QSpinBox(self.ButtonGroup1)
        self.size_width.setObjectName(u"size_width")
        self.size_width.setMinimum(1)
        self.size_width.setMaximum(1280)
        self.size_width.setSingleStep(16)
        self.size_width.setValue(400)

        self.hboxLayout1.addWidget(self.size_width)

        self.size_height = QSpinBox(self.ButtonGroup1)
        self.size_height.setObjectName(u"size_height")
        self.size_height.setMinimum(1)
        self.size_height.setMaximum(1024)
        self.size_height.setSingleStep(16)
        self.size_height.setValue(300)

        self.hboxLayout1.addWidget(self.size_height)


        self.vboxLayout1.addLayout(self.hboxLayout1)


        self.hboxLayout.addWidget(self.ButtonGroup1)

        self.ButtonGroup2 = QGroupBox(Config)
        self.ButtonGroup2.setObjectName(u"ButtonGroup2")
        self.vboxLayout2 = QVBoxLayout(self.ButtonGroup2)
        self.vboxLayout2.setSpacing(6)
        self.vboxLayout2.setContentsMargins(11, 11, 11, 11)
        self.vboxLayout2.setObjectName(u"vboxLayout2")
        self.vboxLayout2.setContentsMargins(11, 11, 11, 11)
        self.depth_1 = QRadioButton(self.ButtonGroup2)
        self.depth_1.setObjectName(u"depth_1")

        self.vboxLayout2.addWidget(self.depth_1)

        self.depth_4gray = QRadioButton(self.ButtonGroup2)
        self.depth_4gray.setObjectName(u"depth_4gray")

        self.vboxLayout2.addWidget(self.depth_4gray)

        self.depth_8 = QRadioButton(self.ButtonGroup2)
        self.depth_8.setObjectName(u"depth_8")

        self.vboxLayout2.addWidget(self.depth_8)

        self.depth_12 = QRadioButton(self.ButtonGroup2)
        self.depth_12.setObjectName(u"depth_12")

        self.vboxLayout2.addWidget(self.depth_12)

        self.depth_15 = QRadioButton(self.ButtonGroup2)
        self.depth_15.setObjectName(u"depth_15")

        self.vboxLayout2.addWidget(self.depth_15)

        self.depth_16 = QRadioButton(self.ButtonGroup2)
        self.depth_16.setObjectName(u"depth_16")

        self.vboxLayout2.addWidget(self.depth_16)

        self.depth_18 = QRadioButton(self.ButtonGroup2)
        self.depth_18.setObjectName(u"depth_18")

        self.vboxLayout2.addWidget(self.depth_18)

        self.depth_24 = QRadioButton(self.ButtonGroup2)
        self.depth_24.setObjectName(u"depth_24")

        self.vboxLayout2.addWidget(self.depth_24)

        self.depth_32 = QRadioButton(self.ButtonGroup2)
        self.depth_32.setObjectName(u"depth_32")

        self.vboxLayout2.addWidget(self.depth_32)

        self.depth_32_argb = QRadioButton(self.ButtonGroup2)
        self.depth_32_argb.setObjectName(u"depth_32_argb")

        self.vboxLayout2.addWidget(self.depth_32_argb)


        self.hboxLayout.addWidget(self.ButtonGroup2)


        self.vboxLayout.addLayout(self.hboxLayout)

        self.hboxLayout2 = QHBoxLayout()
        self.hboxLayout2.setSpacing(6)
        self.hboxLayout2.setObjectName(u"hboxLayout2")
        self.hboxLayout2.setContentsMargins(0, 0, 0, 0)
        self.TextLabel1_3 = QLabel(Config)
        self.TextLabel1_3.setObjectName(u"TextLabel1_3")

        self.hboxLayout2.addWidget(self.TextLabel1_3)

        self.skin = QComboBox(Config)
        self.skin.addItem("")
        self.skin.setObjectName(u"skin")
        sizePolicy2 = QSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        sizePolicy2.setHorizontalStretch(0)
        sizePolicy2.setVerticalStretch(0)
        sizePolicy2.setHeightForWidth(self.skin.sizePolicy().hasHeightForWidth())
        self.skin.setSizePolicy(sizePolicy2)

        self.hboxLayout2.addWidget(self.skin)


        self.vboxLayout.addLayout(self.hboxLayout2)

        self.touchScreen = QCheckBox(Config)
        self.touchScreen.setObjectName(u"touchScreen")

        self.vboxLayout.addWidget(self.touchScreen)

        self.lcdScreen = QCheckBox(Config)
        self.lcdScreen.setObjectName(u"lcdScreen")

        self.vboxLayout.addWidget(self.lcdScreen)

        self.spacerItem = QSpacerItem(20, 10, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)

        self.vboxLayout.addItem(self.spacerItem)

        self.TextLabel1 = QLabel(Config)
        self.TextLabel1.setObjectName(u"TextLabel1")
        sizePolicy.setHeightForWidth(self.TextLabel1.sizePolicy().hasHeightForWidth())
        self.TextLabel1.setSizePolicy(sizePolicy)
        self.TextLabel1.setWordWrap(True)

        self.vboxLayout.addWidget(self.TextLabel1)

        self.GroupBox1 = QGroupBox(Config)
        self.GroupBox1.setObjectName(u"GroupBox1")
        self.gridLayout = QGridLayout(self.GroupBox1)
        self.gridLayout.setSpacing(6)
        self.gridLayout.setContentsMargins(11, 11, 11, 11)
        self.gridLayout.setObjectName(u"gridLayout")
        self.gridLayout.setHorizontalSpacing(6)
        self.gridLayout.setVerticalSpacing(6)
        self.gridLayout.setContentsMargins(11, 11, 11, 11)
        self.TextLabel3 = QLabel(self.GroupBox1)
        self.TextLabel3.setObjectName(u"TextLabel3")

        self.gridLayout.addWidget(self.TextLabel3, 6, 0, 1, 1)

        self.bslider = QSlider(self.GroupBox1)
        self.bslider.setObjectName(u"bslider")
        palette = QPalette()
        brush = QBrush(QColor(128, 128, 128, 255))
        brush.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.WindowText, brush)
        brush1 = QBrush(QColor(0, 0, 255, 255))
        brush1.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Button, brush1)
        brush2 = QBrush(QColor(127, 127, 255, 255))
        brush2.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Light, brush2)
        brush3 = QBrush(QColor(38, 38, 255, 255))
        brush3.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Midlight, brush3)
        brush4 = QBrush(QColor(0, 0, 127, 255))
        brush4.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Dark, brush4)
        brush5 = QBrush(QColor(0, 0, 170, 255))
        brush5.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Mid, brush5)
        brush6 = QBrush(QColor(0, 0, 0, 255))
        brush6.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Text, brush6)
        brush7 = QBrush(QColor(255, 255, 255, 255))
        brush7.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.BrightText, brush7)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.ButtonText, brush)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Base, brush7)
        brush8 = QBrush(QColor(220, 220, 220, 255))
        brush8.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Window, brush8)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Shadow, brush6)
        brush9 = QBrush(QColor(10, 95, 137, 255))
        brush9.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Highlight, brush9)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.HighlightedText, brush7)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Link, brush6)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.LinkVisited, brush6)
        brush10 = QBrush(QColor(232, 232, 232, 255))
        brush10.setStyle(Qt.BrushStyle.SolidPattern)
        palette.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.AlternateBase, brush10)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.WindowText, brush)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Button, brush1)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Light, brush2)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Midlight, brush3)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Dark, brush4)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Mid, brush5)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Text, brush6)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.BrightText, brush7)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.ButtonText, brush)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Base, brush7)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Window, brush8)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Shadow, brush6)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Highlight, brush9)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.HighlightedText, brush7)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Link, brush6)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.LinkVisited, brush6)
        palette.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.AlternateBase, brush10)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, brush)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Button, brush1)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Light, brush2)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Midlight, brush3)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Dark, brush4)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Mid, brush5)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, brush6)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.BrightText, brush7)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, brush)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Base, brush7)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Window, brush8)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Shadow, brush6)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Highlight, brush9)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.HighlightedText, brush7)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Link, brush6)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.LinkVisited, brush6)
        palette.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.AlternateBase, brush10)
        self.bslider.setPalette(palette)
        self.bslider.setMaximum(400)
        self.bslider.setValue(100)
        self.bslider.setOrientation(Qt.Horizontal)

        self.gridLayout.addWidget(self.bslider, 6, 1, 1, 1)

        self.blabel = QLabel(self.GroupBox1)
        self.blabel.setObjectName(u"blabel")

        self.gridLayout.addWidget(self.blabel, 6, 2, 1, 1)

        self.TextLabel2 = QLabel(self.GroupBox1)
        self.TextLabel2.setObjectName(u"TextLabel2")

        self.gridLayout.addWidget(self.TextLabel2, 4, 0, 1, 1)

        self.gslider = QSlider(self.GroupBox1)
        self.gslider.setObjectName(u"gslider")
        palette1 = QPalette()
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.WindowText, brush)
        brush11 = QBrush(QColor(0, 255, 0, 255))
        brush11.setStyle(Qt.BrushStyle.SolidPattern)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Button, brush11)
        brush12 = QBrush(QColor(127, 255, 127, 255))
        brush12.setStyle(Qt.BrushStyle.SolidPattern)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Light, brush12)
        brush13 = QBrush(QColor(38, 255, 38, 255))
        brush13.setStyle(Qt.BrushStyle.SolidPattern)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Midlight, brush13)
        brush14 = QBrush(QColor(0, 127, 0, 255))
        brush14.setStyle(Qt.BrushStyle.SolidPattern)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Dark, brush14)
        brush15 = QBrush(QColor(0, 170, 0, 255))
        brush15.setStyle(Qt.BrushStyle.SolidPattern)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Mid, brush15)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Text, brush6)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.BrightText, brush7)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.ButtonText, brush)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Base, brush7)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Window, brush8)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Shadow, brush6)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Highlight, brush9)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.HighlightedText, brush7)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Link, brush6)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.LinkVisited, brush6)
        palette1.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.AlternateBase, brush10)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.WindowText, brush)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Button, brush11)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Light, brush12)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Midlight, brush13)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Dark, brush14)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Mid, brush15)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Text, brush6)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.BrightText, brush7)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.ButtonText, brush)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Base, brush7)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Window, brush8)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Shadow, brush6)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Highlight, brush9)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.HighlightedText, brush7)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Link, brush6)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.LinkVisited, brush6)
        palette1.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.AlternateBase, brush10)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, brush)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Button, brush11)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Light, brush12)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Midlight, brush13)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Dark, brush14)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Mid, brush15)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, brush6)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.BrightText, brush7)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, brush)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Base, brush7)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Window, brush8)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Shadow, brush6)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Highlight, brush9)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.HighlightedText, brush7)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Link, brush6)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.LinkVisited, brush6)
        palette1.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.AlternateBase, brush10)
        self.gslider.setPalette(palette1)
        self.gslider.setMaximum(400)
        self.gslider.setValue(100)
        self.gslider.setOrientation(Qt.Horizontal)

        self.gridLayout.addWidget(self.gslider, 4, 1, 1, 1)

        self.glabel = QLabel(self.GroupBox1)
        self.glabel.setObjectName(u"glabel")

        self.gridLayout.addWidget(self.glabel, 4, 2, 1, 1)

        self.TextLabel7 = QLabel(self.GroupBox1)
        self.TextLabel7.setObjectName(u"TextLabel7")

        self.gridLayout.addWidget(self.TextLabel7, 0, 0, 1, 1)

        self.TextLabel8 = QLabel(self.GroupBox1)
        self.TextLabel8.setObjectName(u"TextLabel8")

        self.gridLayout.addWidget(self.TextLabel8, 0, 2, 1, 1)

        self.gammaslider = QSlider(self.GroupBox1)
        self.gammaslider.setObjectName(u"gammaslider")
        palette2 = QPalette()
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.WindowText, brush)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Button, brush7)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Light, brush7)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Midlight, brush7)
        brush16 = QBrush(QColor(127, 127, 127, 255))
        brush16.setStyle(Qt.BrushStyle.SolidPattern)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Dark, brush16)
        brush17 = QBrush(QColor(170, 170, 170, 255))
        brush17.setStyle(Qt.BrushStyle.SolidPattern)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Mid, brush17)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Text, brush6)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.BrightText, brush7)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.ButtonText, brush)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Base, brush7)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Window, brush8)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Shadow, brush6)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Highlight, brush9)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.HighlightedText, brush7)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Link, brush6)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.LinkVisited, brush6)
        palette2.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.AlternateBase, brush10)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.WindowText, brush)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Button, brush7)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Light, brush7)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Midlight, brush7)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Dark, brush16)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Mid, brush17)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Text, brush6)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.BrightText, brush7)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.ButtonText, brush)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Base, brush7)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Window, brush8)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Shadow, brush6)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Highlight, brush9)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.HighlightedText, brush7)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Link, brush6)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.LinkVisited, brush6)
        palette2.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.AlternateBase, brush10)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, brush)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Button, brush7)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Light, brush7)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Midlight, brush7)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Dark, brush16)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Mid, brush17)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, brush6)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.BrightText, brush7)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, brush)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Base, brush7)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Window, brush8)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Shadow, brush6)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Highlight, brush9)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.HighlightedText, brush7)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Link, brush6)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.LinkVisited, brush6)
        palette2.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.AlternateBase, brush10)
        self.gammaslider.setPalette(palette2)
        self.gammaslider.setMaximum(400)
        self.gammaslider.setValue(100)
        self.gammaslider.setOrientation(Qt.Horizontal)

        self.gridLayout.addWidget(self.gammaslider, 0, 1, 1, 1)

        self.TextLabel1_2 = QLabel(self.GroupBox1)
        self.TextLabel1_2.setObjectName(u"TextLabel1_2")

        self.gridLayout.addWidget(self.TextLabel1_2, 2, 0, 1, 1)

        self.rlabel = QLabel(self.GroupBox1)
        self.rlabel.setObjectName(u"rlabel")

        self.gridLayout.addWidget(self.rlabel, 2, 2, 1, 1)

        self.rslider = QSlider(self.GroupBox1)
        self.rslider.setObjectName(u"rslider")
        palette3 = QPalette()
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.WindowText, brush)
        brush18 = QBrush(QColor(255, 0, 0, 255))
        brush18.setStyle(Qt.BrushStyle.SolidPattern)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Button, brush18)
        brush19 = QBrush(QColor(255, 127, 127, 255))
        brush19.setStyle(Qt.BrushStyle.SolidPattern)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Light, brush19)
        brush20 = QBrush(QColor(255, 38, 38, 255))
        brush20.setStyle(Qt.BrushStyle.SolidPattern)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Midlight, brush20)
        brush21 = QBrush(QColor(127, 0, 0, 255))
        brush21.setStyle(Qt.BrushStyle.SolidPattern)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Dark, brush21)
        brush22 = QBrush(QColor(170, 0, 0, 255))
        brush22.setStyle(Qt.BrushStyle.SolidPattern)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Mid, brush22)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Text, brush6)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.BrightText, brush7)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.ButtonText, brush)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Base, brush7)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Window, brush8)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Shadow, brush6)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Highlight, brush9)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.HighlightedText, brush7)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.Link, brush6)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.LinkVisited, brush6)
        palette3.setBrush(QPalette.ColorGroup.Active, QPalette.ColorRole.AlternateBase, brush10)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.WindowText, brush)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Button, brush18)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Light, brush19)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Midlight, brush20)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Dark, brush21)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Mid, brush22)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Text, brush6)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.BrightText, brush7)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.ButtonText, brush)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Base, brush7)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Window, brush8)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Shadow, brush6)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Highlight, brush9)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.HighlightedText, brush7)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.Link, brush6)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.LinkVisited, brush6)
        palette3.setBrush(QPalette.ColorGroup.Inactive, QPalette.ColorRole.AlternateBase, brush10)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, brush)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Button, brush18)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Light, brush19)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Midlight, brush20)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Dark, brush21)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Mid, brush22)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, brush6)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.BrightText, brush7)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.ButtonText, brush)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Base, brush7)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Window, brush8)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Shadow, brush6)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Highlight, brush9)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.HighlightedText, brush7)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Link, brush6)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.LinkVisited, brush6)
        palette3.setBrush(QPalette.ColorGroup.Disabled, QPalette.ColorRole.AlternateBase, brush10)
        self.rslider.setPalette(palette3)
        self.rslider.setMaximum(400)
        self.rslider.setValue(100)
        self.rslider.setOrientation(Qt.Horizontal)

        self.gridLayout.addWidget(self.rslider, 2, 1, 1, 1)

        self.PushButton3 = QPushButton(self.GroupBox1)
        self.PushButton3.setObjectName(u"PushButton3")

        self.gridLayout.addWidget(self.PushButton3, 8, 0, 1, 3)

        self.MyCustomWidget1 = GammaView(self.GroupBox1)
        self.MyCustomWidget1.setObjectName(u"MyCustomWidget1")

        self.gridLayout.addWidget(self.MyCustomWidget1, 0, 3, 9, 1)


        self.vboxLayout.addWidget(self.GroupBox1)

        self.hboxLayout3 = QHBoxLayout()
        self.hboxLayout3.setSpacing(6)
        self.hboxLayout3.setObjectName(u"hboxLayout3")
        self.hboxLayout3.setContentsMargins(0, 0, 0, 0)
        self.spacerItem1 = QSpacerItem(40, 20, QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Minimum)

        self.hboxLayout3.addItem(self.spacerItem1)

        self.buttonOk = QPushButton(Config)
        self.buttonOk.setObjectName(u"buttonOk")
        self.buttonOk.setAutoDefault(True)

        self.hboxLayout3.addWidget(self.buttonOk)

        self.buttonCancel = QPushButton(Config)
        self.buttonCancel.setObjectName(u"buttonCancel")
        self.buttonCancel.setAutoDefault(True)

        self.hboxLayout3.addWidget(self.buttonCancel)


        self.vboxLayout.addLayout(self.hboxLayout3)


        self.retranslateUi(Config)
        self.size_width.valueChanged.connect(self.size_custom.click)
        self.size_height.valueChanged.connect(self.size_custom.click)

        self.buttonOk.setDefault(True)


        QMetaObject.connectSlotsByName(Config)
    # setupUi

    def retranslateUi(self, Config):
        Config.setWindowTitle(QCoreApplication.translate("Config", u"Configure", None))
        self.ButtonGroup1.setTitle(QCoreApplication.translate("Config", u"Size", None))
        self.size_176_220.setText(QCoreApplication.translate("Config", u"176x220 \"SmartPhone\"", None))
        self.size_240_320.setText(QCoreApplication.translate("Config", u"240x320 \"PDA\"", None))
        self.size_320_240.setText(QCoreApplication.translate("Config", u"320x240 \"TV\" / \"QVGA\"", None))
        self.size_640_480.setText(QCoreApplication.translate("Config", u"640x480 \"VGA\"", None))
        self.size_800_600.setText(QCoreApplication.translate("Config", u"800x600", None))
        self.size_1024_768.setText(QCoreApplication.translate("Config", u"1024x768", None))
        self.size_custom.setText(QCoreApplication.translate("Config", u"Custom", None))
        self.ButtonGroup2.setTitle(QCoreApplication.translate("Config", u"Depth", None))
        self.depth_1.setText(QCoreApplication.translate("Config", u"1 bit monochrome", None))
        self.depth_4gray.setText(QCoreApplication.translate("Config", u"4 bit grayscale", None))
        self.depth_8.setText(QCoreApplication.translate("Config", u"8 bit", None))
        self.depth_12.setText(QCoreApplication.translate("Config", u"12 (16) bit", None))
        self.depth_15.setText(QCoreApplication.translate("Config", u"15 bit", None))
        self.depth_16.setText(QCoreApplication.translate("Config", u"16 bit", None))
        self.depth_18.setText(QCoreApplication.translate("Config", u"18 bit", None))
        self.depth_24.setText(QCoreApplication.translate("Config", u"24 bit", None))
        self.depth_32.setText(QCoreApplication.translate("Config", u"32 bit", None))
        self.depth_32_argb.setText(QCoreApplication.translate("Config", u"32 bit ARGB", None))
        self.TextLabel1_3.setText(QCoreApplication.translate("Config", u"Skin", None))
        self.skin.setItemText(0, QCoreApplication.translate("Config", u"None", None))

        self.touchScreen.setText(QCoreApplication.translate("Config", u"Emulate touch screen (no mouse move)", None))
        self.lcdScreen.setText(QCoreApplication.translate("Config", u"Emulate LCD screen (Only with fixed zoom of 3.0 times magnification)", None))
        self.TextLabel1.setText(QCoreApplication.translate("Config", u"<p>Note that any applications using the virtual framebuffer will be terminated if you change the Size or Depth <i>above</i>. You may freely modify the Gamma <i>below</i>.", None))
        self.GroupBox1.setTitle(QCoreApplication.translate("Config", u"Gamma", None))
        self.TextLabel3.setText(QCoreApplication.translate("Config", u"Blue", None))
        self.blabel.setText(QCoreApplication.translate("Config", u"1.0", None))
        self.TextLabel2.setText(QCoreApplication.translate("Config", u"Green", None))
        self.glabel.setText(QCoreApplication.translate("Config", u"1.0", None))
        self.TextLabel7.setText(QCoreApplication.translate("Config", u"All", None))
        self.TextLabel8.setText(QCoreApplication.translate("Config", u"1.0", None))
        self.TextLabel1_2.setText(QCoreApplication.translate("Config", u"Red", None))
        self.rlabel.setText(QCoreApplication.translate("Config", u"1.0", None))
        self.PushButton3.setText(QCoreApplication.translate("Config", u"Set all to 1.0", None))
        self.buttonOk.setText(QCoreApplication.translate("Config", u"&OK", None))
        self.buttonCancel.setText(QCoreApplication.translate("Config", u"&Cancel", None))
    # retranslateUi

