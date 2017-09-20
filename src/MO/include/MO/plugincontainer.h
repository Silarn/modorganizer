#ifndef PLUGINCONTAINER_H
#define PLUGINCONTAINER_H

#include "MO/organizercore.h"
#include "MO/previewgenerator.h"
#include <QFile>
#include <QPluginLoader>
#include <QtPlugin>
#include <map>
#include <tuple>
#include <uibase/iplugindiagnose.h>
#include <uibase/ipluginfilemapper.h>
#include <uibase/iplugingame.h>
#include <uibase/iplugininstaller.h>
#include <uibase/ipluginmodpage.h>
#include <uibase/ipluginproxy.h>
#include <uibase/iplugintool.h>
#include <utility>
#include <vector>

// Manages Plugins.
class PluginContainer : public QObject, public MOBase::IPluginDiagnose {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPluginDiagnose)
  private:
    using PluginMap = std::tuple<std::vector<MOBase::IPlugin*>, std::vector<MOBase::IPluginDiagnose*>,
                                 std::vector<MOBase::IPluginGame*>, std::vector<MOBase::IPluginInstaller*>,
                                 std::vector<MOBase::IPluginModPage*>, std::vector<MOBase::IPluginPreview*>,
                                 std::vector<MOBase::IPluginTool*>, std::vector<MOBase::IPluginProxy*>,
                                 std::vector<MOBase::IPluginFileMapper*>>;

    static const unsigned int PROBLEM_PLUGINSNOTLOADED = 1;

  public:
    PluginContainer(OrganizerCore* organizer);

    void setUserInterface(IUserInterface* userInterface, QWidget* widget);

    // Load plugins.
    // First unloads any already loaded plugins.
    void loadPlugins();
    void unloadPlugins();

    MOBase::IPluginGame* managedGame(const QString& name) const;

    // Return loaded plugins of type T.
    template <typename T>
    std::vector<T*>& plugins() {
        return std::get<std::vector<T*>>(m_Plugins);
    }
    template <typename T>
    const std::vector<T*>& plugins() const {
        return std::get<std::vector<T*>>(m_Plugins);
    }

    const PreviewGenerator& previewGenerator() const;

    QStringList pluginFileNames() const;

  public: // IPluginDiagnose interface
    virtual std::vector<unsigned int> activeProblems() const;
    virtual QString shortDescription(unsigned int key) const;
    virtual QString fullDescription(unsigned int key) const;
    virtual bool hasGuidedFix(unsigned int key) const;
    virtual void startGuidedFix(unsigned int key) const;

  signals:
    void diagnosisUpdate();

  private:
    bool verifyPlugin(MOBase::IPlugin* plugin);
    void registerGame(MOBase::IPluginGame* game);
    // Registers plugins?
    bool registerPlugin(QObject* pluginObj, const QString& fileName);
    bool unregisterPlugin(QObject* pluginObj, const QString& fileName);

    OrganizerCore* m_Organizer;

    IUserInterface* m_UserInterface = nullptr;

    PluginMap m_Plugins;

    std::map<QString, MOBase::IPluginGame*> m_SupportedGames;
    // std::vector<boost::signals2::connection> m_DiagnosisConnections;
    QStringList m_FailedPlugins;
    std::vector<QPluginLoader*> m_PluginLoaders;

    PreviewGenerator m_PreviewGenerator;

    QFile m_PluginsCheck;
};

#endif // PLUGINCONTAINER_H
