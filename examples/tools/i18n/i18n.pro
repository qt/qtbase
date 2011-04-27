HEADERS       = languagechooser.h \
                mainwindow.h
SOURCES       = languagechooser.cpp \
                main.cpp \
                mainwindow.cpp
RESOURCES    += i18n.qrc
TRANSLATIONS += translations/i18n_ar.ts \
                translations/i18n_cs.ts \
                translations/i18n_de.ts \
                translations/i18n_el.ts \
                translations/i18n_en.ts \
                translations/i18n_eo.ts \
                translations/i18n_fr.ts \
                translations/i18n_it.ts \
                translations/i18n_jp.ts \
                translations/i18n_ko.ts \
                translations/i18n_no.ts \
                translations/i18n_ru.ts \
                translations/i18n_sv.ts \
                translations/i18n_zh.ts

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/i18n
sources.files = $$SOURCES $$HEADERS $$RESOURCES translations i18n.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/i18n
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
