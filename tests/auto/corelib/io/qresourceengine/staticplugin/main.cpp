#include <QObject>

class PluginClass : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.tests.moc" FILE "staticplugin.json")
};

#include "main.moc"
