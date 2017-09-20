#include "game_fallout4/fallout4savegameinfo.h"
#include "game_fallout4/fallout4savegame.h"

#include <game_gamebryo/gamegamebryo.h>

Fallout4SaveGameInfo::Fallout4SaveGameInfo(GameGamebryo const* game) : GamebryoSaveGameInfo(game) {}

Fallout4SaveGameInfo::~Fallout4SaveGameInfo() {}

const MOBase::ISaveGame* Fallout4SaveGameInfo::getSaveGameInfo(const QString& file) const {
    return new Fallout4SaveGame(file, m_Game);
}
