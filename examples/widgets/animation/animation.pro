TEMPLATE         = \
                 subdirs
SUBDIRS          += \
                 animatedtiles \
                 appchooser \
                 easing \
                 moveblocks \
                 states \
                 stickman \
                 sub-attaq \

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS animation.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation
INSTALLS += target sources
QT += widgets
