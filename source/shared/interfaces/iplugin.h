#ifndef IPLUGIN_H
#define IPLUGIN_H

#include "../loading/iurltypes.h"

#include <QtPlugin>
#include <QString>
#include <QStringList>

#include <memory>

class IGraphModel;
class IParser;
class QUrl;

class IPluginInstance
{
public:
    virtual ~IPluginInstance() = default;

    virtual std::unique_ptr<IParser> parserForUrlTypeName(const QString& urlTypeName) = 0;
};

class IPlugin : public virtual IUrlTypes
{
public:
    virtual ~IPlugin() = default;

    virtual QStringList identifyUrl(const QUrl& url) const = 0;
    virtual std::unique_ptr<IPluginInstance> createInstance(IGraphModel* graphModel) = 0;

    virtual bool editable() const = 0;
    virtual QString contentQmlPath() const = 0;
};

#define IPluginIID "com.kajeka.IPlugin/" VERSION
Q_DECLARE_INTERFACE(IPlugin, IPluginIID)

#endif // IPLUGIN_H
