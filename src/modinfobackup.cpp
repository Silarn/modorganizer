#include "modinfobackup.h"

std::vector<ModInfo::EFlag> ModInfoBackup::getFlags() const
{
  std::vector<ModInfo::EFlag> result = ModInfoRegular::getFlags();
  result.insert(result.begin(), ModInfo::FLAG_BACKUP);
  return result;
}


QString ModInfoBackup::getDescription() const
{
  return tr("This is the backup of a mod");
}


ModInfoBackup::ModInfoBackup(PluginContainer *pluginContainer, QString gameName, const QDir &path, MOShared::DirectoryEntry **directoryStructure)
  : ModInfoRegular(pluginContainer, gameName, path, directoryStructure)
{
}
