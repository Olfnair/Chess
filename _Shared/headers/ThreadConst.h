#ifndef THREADCONST_H
#define THREADCONST_H

#include <windows.h>

namespace ThreadMessage
{
enum ThreadMessage
{
  // connection
  CONNECT = (WM_APP + 1),
  CONNECT_FAIL,
  ID_OK,
  ID_INVALID,
  LOST_CONNECT,

  // menus
  //main :
  SOLO,
  MULTI,
  EXIT,
  BACKTOMAIN,

  // jeu
  SELECT,
  CHECKMOVE,

  // serveur
  KICK,
  NEWCLIENT,
  DISCONNECTED,
  NEWMESS
};
};

#endif  // THREADCONST_H
