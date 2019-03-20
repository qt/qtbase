# Generated from sub-attaq.pro.

#####################################################################
## sub-attaq Binary:
#####################################################################

add_qt_executable(sub-attaq
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/sub-attaq"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/sub-attaq"
    SOURCES
        animationmanager.cpp animationmanager.h
        boat.cpp boat.h boat_p.h
        bomb.cpp bomb.h
        graphicsscene.cpp graphicsscene.h
        main.cpp
        mainwindow.cpp mainwindow.h
        pixmapitem.cpp pixmapitem.h
        progressitem.cpp progressitem.h
        qanimationstate.cpp qanimationstate.h
        states.cpp states.h
        submarine.cpp submarine.h submarine_p.h
        textinformationitem.cpp textinformationitem.h
        torpedo.cpp torpedo.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
set_source_files_properties("pics/big/background.png"
    PROPERTIES alias "big/background")
set_source_files_properties("pics/big/boat.png"
    PROPERTIES alias "big/boat")
set_source_files_properties("pics/big/bomb.png"
    PROPERTIES alias "big/bomb")
set_source_files_properties("pics/big/explosion/boat/step1.png"
    PROPERTIES alias "big/explosion/boat/step1")
set_source_files_properties("pics/big/explosion/boat/step2.png"
    PROPERTIES alias "big/explosion/boat/step2")
set_source_files_properties("pics/big/explosion/boat/step3.png"
    PROPERTIES alias "big/explosion/boat/step3")
set_source_files_properties("pics/big/explosion/boat/step4.png"
    PROPERTIES alias "big/explosion/boat/step4")
set_source_files_properties("pics/big/explosion/submarine/step1.png"
    PROPERTIES alias "big/explosion/submarine/step1")
set_source_files_properties("pics/big/explosion/submarine/step2.png"
    PROPERTIES alias "big/explosion/submarine/step2")
set_source_files_properties("pics/big/explosion/submarine/step3.png"
    PROPERTIES alias "big/explosion/submarine/step3")
set_source_files_properties("pics/big/explosion/submarine/step4.png"
    PROPERTIES alias "big/explosion/submarine/step4")
set_source_files_properties("pics/big/submarine.png"
    PROPERTIES alias "big/submarine")
set_source_files_properties("pics/big/surface.png"
    PROPERTIES alias "big/surface")
set_source_files_properties("pics/big/torpedo.png"
    PROPERTIES alias "big/torpedo")
set_source_files_properties("pics/scalable/boat.svg"
    PROPERTIES alias "boat")
set_source_files_properties("pics/scalable/sub-attaq.svg"
    PROPERTIES alias "all")
set_source_files_properties("pics/scalable/submarine.svg"
    PROPERTIES alias "submarine")
set_source_files_properties("pics/scalable/torpedo.svg"
    PROPERTIES alias "torpedo")
set_source_files_properties("pics/small/background.png"
    PROPERTIES alias "small/background")
set_source_files_properties("pics/small/boat.png"
    PROPERTIES alias "small/boat")
set_source_files_properties("pics/small/bomb.png"
    PROPERTIES alias "small/bomb")
set_source_files_properties("pics/small/submarine.png"
    PROPERTIES alias "small/submarine")
set_source_files_properties("pics/small/surface.png"
    PROPERTIES alias "small/surface")
set_source_files_properties("pics/small/torpedo.png"
    PROPERTIES alias "small/torpedo")
set_source_files_properties("pics/welcome/logo-a.png"
    PROPERTIES alias "logo-a")
set_source_files_properties("pics/welcome/logo-a2.png"
    PROPERTIES alias "logo-a2")
set_source_files_properties("pics/welcome/logo-b.png"
    PROPERTIES alias "logo-b")
set_source_files_properties("pics/welcome/logo-dash.png"
    PROPERTIES alias "logo-dash")
set_source_files_properties("pics/welcome/logo-excl.png"
    PROPERTIES alias "logo-excl")
set_source_files_properties("pics/welcome/logo-q.png"
    PROPERTIES alias "logo-q")
set_source_files_properties("pics/welcome/logo-s.png"
    PROPERTIES alias "logo-s")
set_source_files_properties("pics/welcome/logo-t.png"
    PROPERTIES alias "logo-t")
set_source_files_properties("pics/welcome/logo-t2.png"
    PROPERTIES alias "logo-t2")
set_source_files_properties("pics/welcome/logo-u.png"
    PROPERTIES alias "logo-u")
add_qt_resource(sub-attaq "subattaq" PREFIX "/" FILES
    data.xml
    pics/big/background.png
    pics/big/boat.png
    pics/big/bomb.png
    pics/big/explosion/boat/step1.png
    pics/big/explosion/boat/step2.png
    pics/big/explosion/boat/step3.png
    pics/big/explosion/boat/step4.png
    pics/big/explosion/submarine/step1.png
    pics/big/explosion/submarine/step2.png
    pics/big/explosion/submarine/step3.png
    pics/big/explosion/submarine/step4.png
    pics/big/submarine.png
    pics/big/surface.png
    pics/big/torpedo.png
    pics/scalable/boat.svg
    pics/scalable/sub-attaq.svg
    pics/scalable/submarine.svg
    pics/scalable/torpedo.svg
    pics/small/background.png
    pics/small/boat.png
    pics/small/bomb.png
    pics/small/submarine.png
    pics/small/surface.png
    pics/small/torpedo.png
    pics/welcome/logo-a.png
    pics/welcome/logo-a2.png
    pics/welcome/logo-b.png
    pics/welcome/logo-dash.png
    pics/welcome/logo-excl.png
    pics/welcome/logo-q.png
    pics/welcome/logo-s.png
    pics/welcome/logo-t.png
    pics/welcome/logo-t2.png
    pics/welcome/logo-u.png)


## Scopes:
#####################################################################

extend_target(sub-attaq CONDITION TARGET Qt::OpenGL
    LIBRARIES
        Qt::OpenGL
)
