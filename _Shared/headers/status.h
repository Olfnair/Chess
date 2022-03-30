#ifndef STATUS_H
#define STATUS_H

// statut de la connection
namespace ConnectionStatus
{
enum ConnectionStatus
{
  OFFLINE = 0,
  CONNECTING,
  CONNECTED
};
};

// statut de la partie

// statut des menus
namespace MenuStatus
{
enum MenuStatus
{
  MAIN = 0,
  SOLO,
  MULTI,
  GAME,
  GAME_MULTI
};
};

#endif  // STATUS_H
