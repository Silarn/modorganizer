#include "MO/plugincontainer.h"
#include "MO/organizerproxy.h"

#include <MO/Shared/appconfig.h>
#include <QCoreApplication>
#include <QDirIterator>
#include <QMessageBox>
#include <uibase/report.h>

#include <memory>

using namespace MOBase;
using namespace MOShared;

PluginContainer::PluginContainer(OrganizerCore* organizer) : m_Organizer(organizer) {}

void PluginContainer::setUserInterface(IUserInterface* userInterface, QWidget* widget) {
    // FIXME: This.
    for (IPluginProxy* proxy : this->plugins<IPluginProxy>()) {
        proxy->setParentWidget(widget);
    }

    if (userInterface != nullptr) {
        for (IPluginModPage* modPage : this->plugins<IPluginModPage>()) {
            userInterface->registerModPage(modPage);
        }

        for (IPluginTool* tool : this->plugins<IPluginTool>()) {
            userInterface->registerPluginTool(tool);
        }
    }

    m_UserInterface = userInterface;
}

QStringList PluginContainer::pluginFileNames() const {
    QStringList result;
    for (QPluginLoader* loader : m_PluginLoaders) {
        result.append(loader->fileName());
    }
    return result;
}

bool PluginContainer::verifyPlugin(IPlugin* plugin) {
    if (plugin == nullptr) {
        return false;
    } else if (!plugin->init(new OrganizerProxy(m_Organizer, plugin->name()))) {
        qWarning("plugin failed to initialize");
        return false;
    }
    return true;
}

void PluginContainer::registerGame(IPluginGame* game) { m_SupportedGames.insert({game->gameName(), game}); }

bool PluginContainer::registerPlugin(QObject* plugin, const QString& fileName) {
    { // generic treatment for all plugins
        IPlugin* pluginObj = qobject_cast<IPlugin*>(plugin);
        if (!pluginObj) {
            qDebug("not an IPlugin");
            return false;
        }
        plugin->setProperty("filename", fileName);
        m_Organizer->settings().registerPlugin(pluginObj);
    }

    { // diagnosis plugins
        IPluginDiagnose* diagnose = qobject_cast<IPluginDiagnose*>(plugin);
        if (diagnose) {
            this->plugins<IPluginDiagnose>().push_back(diagnose);
            // FIXME: signals
            // m_DiagnosisConnections.push_back(diagnose->onInvalidated([&]() { emit diagnosisUpdate(); }));
        }
    }
    { // mod page plugin
        IPluginModPage* modPage = qobject_cast<IPluginModPage*>(plugin);
        if (verifyPlugin(modPage)) {
            this->plugins<IPluginModPage>().push_back(modPage);
            return true;
        }
    }
    { // game plugin
        IPluginGame* game = qobject_cast<IPluginGame*>(plugin);
        if (verifyPlugin(game)) {
            this->plugins<IPluginGame>().push_back(game);
            registerGame(game);
            return true;
        }
    }
    { // tool plugins
        IPluginTool* tool = qobject_cast<IPluginTool*>(plugin);
        if (verifyPlugin(tool)) {
            this->plugins<IPluginTool>().push_back(tool);
            return true;
        }
    }
    { // installer plugins
        IPluginInstaller* installer = qobject_cast<IPluginInstaller*>(plugin);
        if (verifyPlugin(installer)) {
            this->plugins<IPluginInstaller>().push_back(installer);
            m_Organizer->installationManager()->registerInstaller(installer);
            return true;
        }
    }
    { // preview plugins
        IPluginPreview* preview = qobject_cast<IPluginPreview*>(plugin);
        if (verifyPlugin(preview)) {
            this->plugins<IPluginPreview>().push_back(preview);
            m_PreviewGenerator.registerPlugin(preview);
            return true;
        }
    }
    { // proxy plugins
        IPluginProxy* proxy = qobject_cast<IPluginProxy*>(plugin);
        if (verifyPlugin(proxy)) {
            this->plugins<IPluginProxy>().push_back(proxy);
            QStringList pluginNames =
                proxy->pluginList(QCoreApplication::applicationDirPath() + "/" + ToQString(AppConfig::pluginPath()));
            for (const QString& pluginName : pluginNames) {
                try {
                    QObject* proxiedPlugin = proxy->instantiate(pluginName);
                    if (proxiedPlugin) {
                        if (registerPlugin(proxiedPlugin, pluginName)) {
                            qDebug("loaded plugin \"%s\"", qUtf8Printable(QFileInfo(pluginName).fileName()));
                        } else {
                            qWarning("plugin \"%s\" failed to load. If this plugin is for an older version of MO "
                                     "you have to update it or delete it if no update exists.",
                                     qUtf8Printable(pluginName));
                        }
                    }
                } catch (const std::exception& e) {
                    reportError(QObject::tr("failed to initialize plugin %1: %2").arg(pluginName).arg(e.what()));
                }
            }
            return true;
        }
    }

    { // dummy plugins
        // only initialize these, no processing otherwise
        IPlugin* dummy = qobject_cast<IPlugin*>(plugin);
        if (verifyPlugin(dummy)) {
            this->plugins<IPlugin>().push_back(dummy);
            return true;
        }
    }

    qDebug("no matching plugin interface");

    return false;
}

struct clearPlugins {
    template <typename T>
    void operator()(T& t) const {
        t.clear();
    }
};

void PluginContainer::unloadPlugins() {
    if (m_UserInterface) {
        m_UserInterface->disconnectPlugins();
    }

    // disconnect all slots before unloading plugins so plugins don't have to take care of that
    m_Organizer->disconnectPlugins();
    // TODO: Make sure this works properly. It should be functionally
    // Equalivant to calling clear on all the vectors, but what if the code does bad things like
    // have a reference to the vector itself, now invalidated?
    m_Plugins = {};

    // FIXME: Signals
    // for (const boost::signals2::connection& : m_DiagnosisConnections) {
    //    connection.disconnect();
    //}
    // m_DiagnosisConnections.clear();

    while (!m_PluginLoaders.empty()) {
        std::unique_ptr<QPluginLoader> loader(m_PluginLoaders.back());
        m_PluginLoaders.pop_back();
        if (loader && !loader->unload()) {
            qDebug("failed to unload %s: %s", qUtf8Printable(loader->fileName()),
                   qUtf8Printable(loader->errorString()));
        }
    }
}

IPluginGame* PluginContainer::managedGame(const QString& name) const {
    auto iter = m_SupportedGames.find(name);
    if (iter != m_SupportedGames.end()) {
        return iter->second;
    } else {
        return nullptr;
    }
}

const PreviewGenerator& PluginContainer::previewGenerator() const { return m_PreviewGenerator; }

void PluginContainer::loadPlugins() {
    // Unload existing.
    unloadPlugins();

    // Register plugins
    for (QObject* plugin : QPluginLoader::staticInstances()) {
        registerPlugin(plugin, "");
    }

    QFile loadCheck(qApp->property("dataPath").toString() + "/plugin_loadcheck.tmp");
    if (loadCheck.exists() && loadCheck.open(QIODevice::ReadOnly)) {
        // oh, there was a failed plugin load last time. Find out which plugin was loaded last
        QString fileName;
        while (!loadCheck.atEnd()) {
            fileName = QString::fromUtf8(loadCheck.readLine().constData()).trimmed();
        }
        if (QMessageBox::question(nullptr, QObject::tr("Plugin error"),
                                  QObject::tr("It appears the plugin \"%1\" failed to load last startup and caused MO "
                                              "to crash. Do you want to disable it?\n"
                                              "(Please note: If this is the first time you see this message for this "
                                              "plugin you may want to give it another try. "
                                              "The plugin may be able to recover from the problem)")
                                      .arg(fileName),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
            m_Organizer->settings().addBlacklistPlugin(fileName);
        }
        loadCheck.close();
    }

    loadCheck.open(QIODevice::WriteOnly);

    QString pluginPath = qApp->applicationDirPath() + "/" + ToQString(AppConfig::pluginPath());
    qDebug("looking for plugins in %s", QDir::toNativeSeparators(pluginPath).toUtf8().constData());
    QDirIterator iter(pluginPath, QDir::Files | QDir::NoDotAndDotDot);

    while (iter.hasNext()) {
        iter.next();
        if (m_Organizer->settings().pluginBlacklisted(iter.fileName())) {
            qDebug("plugin \"%s\" blacklisted", qUtf8Printable(iter.fileName()));
            continue;
        }
        loadCheck.write(iter.fileName().toUtf8());
        loadCheck.write("\n");
        loadCheck.flush();
        QString pluginName = iter.filePath();
        if (QLibrary::isLibrary(pluginName)) {
            std::unique_ptr<QPluginLoader> pluginLoader(new QPluginLoader(pluginName, this));
            if (pluginLoader->instance() == nullptr) {
                m_FailedPlugins.push_back(pluginName);
                qCritical("failed to load plugin %s: %s", qUtf8Printable(pluginName),
                          qUtf8Printable(pluginLoader->errorString()));
            } else {
                if (registerPlugin(pluginLoader->instance(), pluginName)) {
                    qDebug("loaded plugin \"%s\"", qUtf8Printable(QFileInfo(pluginName).fileName()));
                    m_PluginLoaders.push_back(pluginLoader.release());
                } else {
                    m_FailedPlugins.push_back(pluginName);
                    qWarning("plugin \"%s\" failed to load", qUtf8Printable(pluginName));
                }
            }
        }
    }

    // remove the load check file on success
    loadCheck.remove();
    this->plugins<IPluginDiagnose>().push_back(this);

    m_Organizer->connectPlugins(this);
}

std::vector<unsigned int> PluginContainer::activeProblems() const {
    std::vector<unsigned int> problems;
    if (m_FailedPlugins.size()) {
        problems.push_back(PROBLEM_PLUGINSNOTLOADED);
    }
    return problems;
}

QString PluginContainer::shortDescription(unsigned int key) const {
    switch (key) {
    case PROBLEM_PLUGINSNOTLOADED: {
        return tr("Some plugins could not be loaded");
    } break;
    default: { return tr("Description missing"); } break;
    }
}

QString PluginContainer::fullDescription(unsigned int key) const {
    switch (key) {
    case PROBLEM_PLUGINSNOTLOADED: {
        QString result = tr("The following plugins could not be loaded. The reason may be missing "
                            "dependencies (i.e. python) or an outdated version:") +
                         "<ul>";
        for (const QString& plugin : m_FailedPlugins) {
            result += "<li>" + plugin + "</li>";
        }
        result += "<ul>";
        return result;
    } break;
    default: { return tr("Description missing"); } break;
    }
}

bool PluginContainer::hasGuidedFix(unsigned int) const { return false; }

void PluginContainer::startGuidedFix(unsigned int) const {}
