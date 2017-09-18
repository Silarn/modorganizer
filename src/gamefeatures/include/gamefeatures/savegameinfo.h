#ifndef SAVEGAMEINFO_H
#define SAVEGAMEINFO_H

namespace MOBase { class ISaveGame; }
namespace MOBase { class ISaveGameInfoWidget; }

#include <QMap>

class QString;
class QStringList;
class QWidget;

/** Feature to get hold of stuff to do with save games */
class SaveGameInfo
{
public:
  virtual ~SaveGameInfo() {}

  /** Get the information about the supplied save game */
  virtual MOBase::ISaveGame const *getSaveGameInfo(QString const &file) const = 0;

  typedef QStringList ProvidingModules;
  typedef QMap<QString, ProvidingModules> MissingAssets;

  /** Get items missing from save
   *
   * @returns a collection of missing assets and the modules that can supply those
   * assets.
   *
   * Note that in the situation where 'module' and 'asset' are indistinguishable,
   * both still have to be supplied.
   */
  virtual MissingAssets getMissingAssets(QString const &file) const = 0;

  /** Get a widget to display over the save game list.
   *
   * @returns a QT Widget.
   *
   * @param parent - parent widget
   *
   * It is permitted to return a null pointer to indicate you don't have a
   * nice visual way of displaying save game contents
   */
  virtual MOBase::ISaveGameInfoWidget *getSaveGameWidget(QWidget *parent = 0) const = 0;
};

#endif // SAVEGAMEINFO_H

