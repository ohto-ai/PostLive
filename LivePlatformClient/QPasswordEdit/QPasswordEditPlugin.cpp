#include "QPasswordEdit.h"
#include "QPasswordEditPlugin.h"

#include <QtCore/QtPlugin>

QPasswordEditPlugin::QPasswordEditPlugin(QObject *parent)
    : QObject(parent)
{
    initialized = false;
}

void QPasswordEditPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (initialized)
        return;

    initialized = true;
}

bool QPasswordEditPlugin::isInitialized() const
{
    return initialized;
}

QWidget *QPasswordEditPlugin::createWidget(QWidget *parent)
{
    return new QPasswordEdit(parent);
}

QString QPasswordEditPlugin::name() const
{
    return "QPasswordEdit";
}

QString QPasswordEditPlugin::group() const
{
    return "Network";
}

QIcon QPasswordEditPlugin::icon() const
{
    return QIcon();
}

QString QPasswordEditPlugin::toolTip() const
{
    return QString();
}

QString QPasswordEditPlugin::whatsThis() const
{
    return QString();
}

bool QPasswordEditPlugin::isContainer() const
{
    return false;
}

QString QPasswordEditPlugin::domXml() const
{
    return "<widget class=\"QPasswordEdit\" name=\"QPasswordEdit\">\n"
        " <property name=\"geometry\">\n"
        "  <rect>\n"
        "   <x>0</x>\n"
        "   <y>0</y>\n"
        "   <width>100</width>\n"
        "   <height>32</height>\n"
        "  </rect>\n"
        " </property>\n"
        "</widget>\n";
}

QString QPasswordEditPlugin::includeFile() const
{
    return "QPasswordEdit.h";
}
