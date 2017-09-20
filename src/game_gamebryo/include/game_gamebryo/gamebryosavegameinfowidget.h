#ifndef GAMEBRYOSAVEGAMEINFOWIDGET_H
#define GAMEBRYOSAVEGAMEINFOWIDGET_H

#include <QObject>
#include <uibase/isavegameinfowidget.h>

class GamebryoSaveGameInfo;

namespace Ui {
class GamebryoSaveGameInfoWidget;
}

class GamebryoSaveGameInfoWidget : public MOBase::ISaveGameInfoWidget {
    Q_OBJECT

  public:
    GamebryoSaveGameInfoWidget(GamebryoSaveGameInfo const* info, QWidget* parent);
    ~GamebryoSaveGameInfoWidget();

    virtual void setSave(QString const&) override;

  private:
    Ui::GamebryoSaveGameInfoWidget* ui;
    GamebryoSaveGameInfo const* m_Info;
};

#endif // GAMEBRYOSAVEGAMEINFOWIDGET_H
