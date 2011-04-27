TEMPLATE    = subdirs

!contains(QT_CONFIG, no-gui) {
SUBDIRS     = \
            demos_shared \
            demos_deform \
            demos_gradients \
            demos_pathstroke \
            demos_affine \
            demos_composition \
            demos_books \
            demos_interview \
            demos_mainwindow \
            demos_spreadsheet \
            demos_textedit \
            demos_chip \
            demos_embeddeddialogs \
            demos_undo \
            demos_sub-attaq

symbian: SUBDIRS = \
            demos_shared \
            demos_deform \
            demos_pathstroke


wince*:  SUBDIRS = \
            demos_shared \
            demos_deform \
            demos_gradients \
            demos_pathstroke \
            demos_affine \
            demos_composition \
            demos_books \
            demos_interview \
            demos_mainwindow \
            demos_spreadsheet \
            demos_textedit \
            # demos_chip \
            demos_embeddeddialogs \
            demos_undo \
            demos_sub-attaq

contains(QT_CONFIG, opengl):!contains(QT_CONFIG, opengles1):!contains(QT_CONFIG, opengles2):{
SUBDIRS += demos_boxes
}

mac* && !qpa: SUBDIRS += demos_macmainwindow
wince*|symbian|embedded|x11: SUBDIRS += demos_embedded

!contains(QT_EDITION, Console):!cross_compile:!embedded:!wince*:SUBDIRS += demos_arthurplugin

!cross_compile:{
contains(QT_BUILD_PARTS, tools):{
!wince*:SUBDIRS += demos_sqlbrowser demos_qtdemo
wince*:SUBDIRS += demos_sqlbrowser
}
}
contains(QT_CONFIG, phonon):!static:SUBDIRS += demos_mediaplayer
contains(QT_CONFIG, webkit):contains(QT_CONFIG, svg):!symbian:SUBDIRS += demos_browser
contains(QT_CONFIG, declarative):SUBDIRS += demos_declarative
contains(QT_CONFIG, multimedia):!static:SUBDIRS += demos_spectrum

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_DEMOS]
INSTALLS += sources

symbian: include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)

demos_chip.subdir = chip
demos_embeddeddialogs.subdir = embeddeddialogs
demos_embedded.subdir = embedded
# Because of fluidlauncher
demos_embedded.depends = demos_deform demos_pathstroke
demos_shared.subdir = shared
demos_deform.subdir = deform
demos_gradients.subdir = gradients
demos_pathstroke.subdir = pathstroke
demos_affine.subdir = affine
demos_composition.subdir = composition
demos_books.subdir = books
demos_interview.subdir = interview
demos_macmainwindow.subdir = macmainwindow
demos_mainwindow.subdir = mainwindow
demos_spreadsheet.subdir = spreadsheet
demos_textedit.subdir = textedit
demos_arthurplugin.subdir = arthurplugin
demos_sqlbrowser.subdir = sqlbrowser
demos_undo.subdir = undo
demos_qtdemo.subdir = qtdemo
demos_mediaplayer.subdir = qmediaplayer
demos_declarative.subdir = declarative

#mobile demos. Requires QtMobility sources. Not included in demo build
demos_guitartuner.subdir = mobile/guitartuner
demos_qcamera.subdir = mobile/qcamera
demos_qtbubblelevel.subdir = mobile/qtbubblelevel
demos_quickhit.subdir = mobile/quickhit

demos_browser.subdir = browser

demos_boxes.subdir = boxes
demos_sub-attaq.subdir = sub-attaq
demos_spectrum.subdir = spectrum

#CONFIG += ordered
!ordered {
     demos_affine.depends = demos_shared
     demos_deform.depends = demos_shared
     demos_gradients.depends = demos_shared
     demos_composition.depends = demos_shared
     demos_arthurplugin.depends = demos_shared
     demos_pathstroke.depends = demos_shared
}
}
