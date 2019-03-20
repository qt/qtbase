# Generated from i18n.pro.

#####################################################################
## i18n Binary:
#####################################################################

add_qt_executable(i18n
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/i18n"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/i18n"
    SOURCES
        languagechooser.cpp languagechooser.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(i18n "i18n" FILES
    translations/i18n_ar.qm
    translations/i18n_cs.qm
    translations/i18n_de.qm
    translations/i18n_el.qm
    translations/i18n_en.qm
    translations/i18n_eo.qm
    translations/i18n_fr.qm
    translations/i18n_it.qm
    translations/i18n_jp.qm
    translations/i18n_ko.qm
    translations/i18n_no.qm
    translations/i18n_ru.qm
    translations/i18n_sv.qm
    translations/i18n_zh.qm)


#### Keys ignored in scope 1:.:i18n.pro:<NONE>:
# TRANSLATIONS = "translations/i18n_ar.ts" "translations/i18n_cs.ts" "translations/i18n_de.ts" "translations/i18n_el.ts" "translations/i18n_en.ts" "translations/i18n_eo.ts" "translations/i18n_fr.ts" "translations/i18n_it.ts" "translations/i18n_jp.ts" "translations/i18n_ko.ts" "translations/i18n_no.ts" "translations/i18n_ru.ts" "translations/i18n_sv.ts" "translations/i18n_zh.ts"
