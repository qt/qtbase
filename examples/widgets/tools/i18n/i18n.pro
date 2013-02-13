QT += widgets

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
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/i18n
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
