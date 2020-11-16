#include "QIPAddress.h"
#include "QIPAddressPlugin.h"

#include <QtCore/QtPlugin>

QIPAddressPlugin::QIPAddressPlugin(QObject *parent)
    : QObject(parent)
{
    initialized = false;
}

void QIPAddressPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (initialized)
        return;

    initialized = true;
}

bool QIPAddressPlugin::isInitialized() const
{
    return initialized;
}

QWidget *QIPAddressPlugin::createWidget(QWidget *parent)
{
    return new QIPAddress(parent);
}

QString QIPAddressPlugin::name() const
{
    return "QIPAddress";
}

QString QIPAddressPlugin::group() const
{
    return "Network";
}

QIcon QIPAddressPlugin::icon() const
{
    return QIcon();
}

QString QIPAddressPlugin::toolTip() const
{
    return QString();
}

QString QIPAddressPlugin::whatsThis() const
{
    return QString();
}

bool QIPAddressPlugin::isContainer() const
{
    return false;
}

QString QIPAddressPlugin::domXml() const
{
    return "<widget class=\"QIPAddress\" name=\"QIPAddress\">\n"
        " <property name=\"geometry\">\n"
        "  <rect>\n"
        "   <x>0</x>\n"
        "   <y>0</y>\n"
        "   <width>100</width>\n"
        "   <height>100</height>\n"
        "  </rect>\n"
        " </property>\n"
        "</widget>\n";
}

QString QIPAddressPlugin::includeFile() const
{
    return "QIPAddress.h";
}
