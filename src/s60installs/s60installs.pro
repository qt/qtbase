# Use subdirs template to suppress generation of unnecessary files
TEMPLATE = subdirs

symbian: {
    load(data_caging_paths)

    SUBDIRS=
    # WARNING: Changing TARGET name will break Symbian SISX upgrade functionality
    # DO NOT TOUCH TARGET VARIABLE IF YOU ARE NOT SURE WHAT YOU ARE DOING
    TARGET = "Qt$${QT_LIBINFIX}"

    isEmpty(QT_LIBINFIX) {
        TARGET.UID3 = 0x2001E61C
    } else {
        # Always use experimental UID for infixed configuration to avoid UID clash
        TARGET.UID3 = 0xE001E61C
    }
    VERSION=$${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}

    DESTDIR = $$QMAKE_LIBDIR_QT

    qtlibraries.files = \
        $$QMAKE_LIBDIR_QT/QtCore$${QT_LIBINFIX}.dll \
        $$QMAKE_LIBDIR_QT/QtXml$${QT_LIBINFIX}.dll \
        $$QMAKE_LIBDIR_QT/QtGui$${QT_LIBINFIX}.dll \
        $$QMAKE_LIBDIR_QT/QtNetwork$${QT_LIBINFIX}.dll \
        $$QMAKE_LIBDIR_QT/QtTest$${QT_LIBINFIX}.dll \
        $$QMAKE_LIBDIR_QT/QtSql$${QT_LIBINFIX}.dll

    symbian-abld|symbian-sbsv2 {
        pluginLocations = $${EPOCROOT}epoc32/release/$(PLATFORM)/$(TARGET)
        bearerPluginLocation = $${EPOCROOT}epoc32/release/$(PLATFORM)/$(TARGET)
        bearerStubZ = $${EPOCROOT}$${HW_ZDIR}$${QT_PLUGINS_BASE_DIR}/bearer/qsymbianbearer$${QT_LIBINFIX}.qtplugin
        BLD_INF_RULES.prj_exports += \
            "qsymbianbearer.qtplugin /$${HW_ZDIR}$${QT_PLUGINS_BASE_DIR}/bearer/qsymbianbearer$${QT_LIBINFIX}.qtplugin" \
            "qsymbianbearer.qtplugin /epoc32/winscw/c$${QT_PLUGINS_BASE_DIR}/bearer/qsymbianbearer$${QT_LIBINFIX}.qtplugin"
    } else {
        pluginLocations = $$QT_BUILD_TREE/plugins/s60
        bearerPluginLocation = $$QT_BUILD_TREE/plugins/bearer
        bearerStubZ = $${PWD}/qsymbianbearer.qtplugin
    }

    contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
        qts60plugindeployment = \
            "IF package(0x2003A678) OR package(0x20022E6D)" \
            "   \"$$pluginLocations/qts60plugin_5_0$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qts60plugin_5_0$${QT_LIBINFIX}.dll\"" \
            "   \"$$bearerPluginLocation/qsymbianbearer$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qsymbianbearer$${QT_LIBINFIX}.dll\"" \
            "ELSEIF package(0x1028315F)" \
            "   \"$$pluginLocations/qts60plugin_5_0$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qts60plugin_5_0$${QT_LIBINFIX}.dll\"" \
            "   \"$$bearerPluginLocation/qsymbianbearer$${QT_LIBINFIX}_3_2.dll\" - \"c:\\sys\\bin\\qsymbianbearer$${QT_LIBINFIX}.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$$pluginLocations/qts60plugin_3_2$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qts60plugin_3_2$${QT_LIBINFIX}.dll\"" \
            "   \"$$bearerPluginLocation/qsymbianbearer$${QT_LIBINFIX}_3_2.dll\" - \"c:\\sys\\bin\\qsymbianbearer$${QT_LIBINFIX}.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$$pluginLocations/qts60plugin_3_1$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qts60plugin_3_1$${QT_LIBINFIX}.dll\"" \
            "   \"$$bearerPluginLocation/qsymbianbearer$${QT_LIBINFIX}_3_1.dll\" - \"c:\\sys\\bin\\qsymbianbearer$${QT_LIBINFIX}.dll\"" \
            "ELSE" \
            "   \"$$pluginLocations/qts60plugin_5_0$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qts60plugin_5_0$${QT_LIBINFIX}.dll\"" \
            "   \"$$bearerPluginLocation/qsymbianbearer$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qsymbianbearer$${QT_LIBINFIX}.dll\"" \
            "ENDIF" \
            "   \"$$bearerStubZ\" - \"c:$$replace(QT_PLUGINS_BASE_DIR,/,\\)\\bearer\\qsymbianbearer$${QT_LIBINFIX}.qtplugin\""
    } else {
        # No need to deploy plugins for older platform versions when building on Symbian3 or later
        qts60plugindeployment = \
            "   \"$$pluginLocations/qts60plugin_5_0$${QT_LIBINFIX}.dll\" - \"c:\\sys\\bin\\qts60plugin_5_0$${QT_LIBINFIX}.dll\""

        bearer_plugin.files = $$QT_BUILD_TREE/plugins/bearer/qsymbianbearer$${QT_LIBINFIX}.dll
        bearer_plugin.path = c:$$QT_PLUGINS_BASE_DIR/bearer
        DEPLOYMENT += bearer_plugin
    }

    qtlibraries.pkg_postrules += qts60plugindeployment

    qtlibraries.path = c:/sys/bin

    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"Nokia\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"Nokia, Qt\"" \
        " "


    qtlibraries.pkg_prerules = vendorinfo
    qtlibraries.pkg_prerules += "; Dependencies of Qt libraries"

    # It is expected that Symbian^3 and newer phones will have sufficiently new OpenC already installed
    contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
        qtlibraries.pkg_prerules += "(0x20013851), 1, 5, 1, {\"PIPS Installer\"}"
        contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
            qtlibraries.pkg_prerules += "(0x200110CB), 1, 5, 1, {\"Open C LIBSSL Common\"}"
        }
        contains(CONFIG, stl) {
            qtlibraries.pkg_prerules += "(0x2000F866), 1, 0, 0, {\"Standard C++ Library Common\"}"
        }
    }
    qtlibraries.pkg_prerules += "(0x2002af5f), 0, 5, 0, {\"sqlite3\"}"

    !contains(QT_CONFIG, no-jpeg): imageformats_plugins.files += $$QT_BUILD_TREE/plugins/imageformats/qjpeg$${QT_LIBINFIX}.dll
    !contains(QT_CONFIG, no-gif):  imageformats_plugins.files += $$QT_BUILD_TREE/plugins/imageformats/qgif$${QT_LIBINFIX}.dll
    !contains(QT_CONFIG, no-mng):  imageformats_plugins.files += $$QT_BUILD_TREE/plugins/imageformats/qmng$${QT_LIBINFIX}.dll
    !contains(QT_CONFIG, no-tiff): imageformats_plugins.files += $$QT_BUILD_TREE/plugins/imageformats/qtiff$${QT_LIBINFIX}.dll
    !contains(QT_CONFIG, no-ico):  imageformats_plugins.files += $$QT_BUILD_TREE/plugins/imageformats/qico$${QT_LIBINFIX}.dll
    imageformats_plugins.path = c:$$QT_PLUGINS_BASE_DIR/imageformats

    codecs_plugins.files = $$QT_BUILD_TREE/plugins/codecs/qcncodecs$${QT_LIBINFIX}.dll $$QT_BUILD_TREE/plugins/codecs/qjpcodecs$${QT_LIBINFIX}.dll $$QT_BUILD_TREE/plugins/codecs/qtwcodecs$${QT_LIBINFIX}.dll $$QT_BUILD_TREE/plugins/codecs/qkrcodecs$${QT_LIBINFIX}.dll
    codecs_plugins.path = c:$$QT_PLUGINS_BASE_DIR/codecs

    contains(QT_CONFIG, phonon-backend) {
        phonon_backend_plugins.files += $$QT_BUILD_TREE/plugins/phonon_backend/phonon_mmf$${QT_LIBINFIX}.dll

        phonon_backend_plugins.path = c:$$QT_PLUGINS_BASE_DIR/phonon_backend
        DEPLOYMENT += phonon_backend_plugins
    }

    # Support backup & restore for Qt libraries
    qtbackup.files = backup_registration.xml
    qtbackup.path = c:/private/10202D56/import/packages/$$replace(TARGET.UID3, 0x,)

    DEPLOYMENT += qtlibraries \
                  qtbackup \
                  imageformats_plugins \
                  codecs_plugins \
                  graphicssystems_plugins

    contains(QT_CONFIG, svg): {
       qtlibraries.files += $$QMAKE_LIBDIR_QT/QtSvg$${QT_LIBINFIX}.dll
       imageformats_plugins.files += $$QT_BUILD_TREE/plugins/imageformats/qsvg$${QT_LIBINFIX}.dll
       iconengines_plugins.files = $$QT_BUILD_TREE/plugins/iconengines/qsvgicon$${QT_LIBINFIX}.dll
       iconengines_plugins.path = c:$$QT_PLUGINS_BASE_DIR/iconengines
       DEPLOYMENT += iconengines_plugins
    }

    contains(QT_CONFIG, phonon): {
       qtlibraries.files += $$QMAKE_LIBDIR_QT/phonon$${QT_LIBINFIX}.dll
    }

    contains(QT_CONFIG, script): {
        qtlibraries.files += $$QMAKE_LIBDIR_QT/QtScript$${QT_LIBINFIX}.dll
    }

    contains(QT_CONFIG, xmlpatterns): {
       qtlibraries.files += $$QMAKE_LIBDIR_QT/QtXmlPatterns$${QT_LIBINFIX}.dll
    }

    contains(QT_CONFIG, declarative): {
        qtlibraries.files += $$QMAKE_LIBDIR_QT/QtDeclarative$${QT_LIBINFIX}.dll

        folderlistmodelImport.files = $$QT_BUILD_TREE/imports/Qt/labs/folderlistmodel/qmlfolderlistmodelplugin$${QT_LIBINFIX}.dll
        gesturesImport.files = $$QT_BUILD_TREE/imports/Qt/labs/gestures/qmlgesturesplugin$${QT_LIBINFIX}.dll
        particlesImport.files = $$QT_BUILD_TREE/imports/Qt/labs/particles/qmlparticlesplugin$${QT_LIBINFIX}.dll

        folderlistmodelImport.files += $$QT_SOURCE_TREE/src/imports/folderlistmodel/qmldir
        gesturesImport.files += $$QT_SOURCE_TREE/src/imports/gestures/qmldir
        particlesImport.files += $$QT_SOURCE_TREE/src/imports/particles/qmldir

        folderlistmodelImport.path = c:$$QT_IMPORTS_BASE_DIR/Qt/labs/folderlistmodel
        gesturesImport.path = c:$$QT_IMPORTS_BASE_DIR/Qt/labs/gestures
        particlesImport.path = c:$$QT_IMPORTS_BASE_DIR/Qt/labs/particles

        DEPLOYMENT += folderlistmodelImport gesturesImport particlesImport
    }

    graphicssystems_plugins.path = c:$$QT_PLUGINS_BASE_DIR/graphicssystems
    contains(QT_CONFIG, openvg) {
        qtlibraries.files += $$QMAKE_LIBDIR_QT/QtOpenVG$${QT_LIBINFIX}.dll
        graphicssystems_plugins.files += $$QT_BUILD_TREE/plugins/graphicssystems/qvggraphicssystem$${QT_LIBINFIX}.dll
    }

    contains(QT_CONFIG, opengl) {
        qtlibraries.files += $$QMAKE_LIBDIR_QT/QtOpenGL$${QT_LIBINFIX}.dll
        graphicssystems_plugins.files += $$QT_BUILD_TREE/plugins/graphicssystems/qglgraphicssystem$${QT_LIBINFIX}.dll
    }

    contains(QT_CONFIG, multimedia){
        qtlibraries.files += $$QMAKE_LIBDIR_QT/QtMultimedia$${QT_LIBINFIX}.dll
    }

    BLD_INF_RULES.prj_exports += "qt.iby $$CORE_MW_LAYER_IBY_EXPORT_PATH(qt.iby)"
}
