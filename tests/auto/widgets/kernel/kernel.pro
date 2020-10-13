TEMPLATE=subdirs
SUBDIRS=\
   qapplication \
   qboxlayout \
   qformlayout \
   qgesturerecognizer \
   qgridlayout \
   qlayout \
   qstackedlayout \
   qtooltip \
   qwidget \
   qwidget_window \
   qwidgetmetatype \
   qwidgetsvariant \
   qwindowcontainer \
   qshortcut \
   qsizepolicy

# QTBUG-87668
android: SUBDIRS -= \
    qwidget

darwin:SUBDIRS -= \
   qgesturerecognizer \

qtConfig(action):SUBDIRS += \
   qaction \
   qactiongroup \
   qwidgetaction

!qtConfig(shortcut): SUBDIRS -= \
   qshortcut
