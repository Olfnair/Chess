#include "Server.h"

#include "NetConst.h"
#include "ThreadConst.h"
#include "tools.h"

void handleException(int e)
{
  if (e == FILE_EXC)
    MessageBox(nullptr, L"File exception! File not found?", L"Error", MB_ICONERROR | MB_OK);
  else if (e == SOCKET_EXC)
  {
    std::string error = "Socket exception! Error Code : " + uint_to_str(WSAGetLastError(), DEC);
    std::u16string u16error = UTF8_TO_UTF16.from_bytes(error);
    MessageBox(nullptr, (LPCWSTR)u16error.data(), L"Error", MB_ICONERROR | MB_OK);
  }
  else if (e == MYSQL_EXC)
    MessageBox(nullptr, L"Error connecting to database!", L"Error", MB_ICONERROR | MB_OK);
  else
    MessageBox(nullptr, L"Unknow exception!", L"Error", MB_ICONERROR | MB_OK);
  exit(-1);
}

// SharedGame
void SharedGame::lock()
{
  mutex.lock();
}
void SharedGame::unlock()
{
  mutex.unlock();
}
// fin SharedGame

// TaskV2
void TaskV2::setHwnd(HWND hwnd)
{
  this->hwnd = hwnd;
}
// fin TaskV2

// TaskListen
TCP_Socket& TaskListen::getGuest()
{
  return guest;
}
// private:
void TaskListen::main()
{
  try
  {
    File serverInfo("server.ini");
    TCP_SockServer sockServer(char_to_uint(serverInfo.readline().c_str(), DEC), 5);
    serverInfo.close();
    while (!isOver())
    {
      if (sockServer.accept(guest, 50))
      {  // si on accepte un client dans les 50 ms :
        PostMessage(hwnd, ThreadMessage::NEWCLIENT, 0, 0);
        wait();  // on attend que mainThread nous dise de continuer (il va devoir manipuler TCP_Socket guest)
      }
    }
  }
  catch (int e)
  {
    handleException(e);
  }
}
// fin TaskListen

// TaskGuest
TaskGuest::TaskGuest()
    : connectionStatus(ConnectionStatus::OFFLINE), guest(nullptr)
{
}
TaskGuest::TaskGuest(HWND hwnd, const TCP_Socket* sock)
    : TaskV2(hwnd), sock(*sock), connectionStatus(ConnectionStatus::OFFLINE), guest(nullptr)
{
}
TCP_Socket& TaskGuest::getSock()
{
  return sock;
}
void TaskGuest::setSock(const TCP_Socket& sock)
{
  this->sock = sock;
}
NetMessage& TaskGuest::getMsg()
{
  return msg;
}
Guest* TaskGuest::getGuest()
{
  return guest;
}
void TaskGuest::setGuest(Guest* guest)
{
  this->guest = guest;
}
Thread<TaskGuest>* TaskGuest::getOther()
{
  return other;
}
void TaskGuest::setOther(Thread<TaskGuest>* other)
{
  this->other = other;
}
Chess::Game* TaskGuest::getGame()
{
  Chess::Game* g;
  sgame.lock();
  g = sgame.game;
  sgame.unlock();
  return g;
}
void TaskGuest::setGame(Chess::Game* game)
{
  sgame.lock();
  sgame.game = game;
  sgame.unlock();
}
ConnectionStatus::ConnectionStatus TaskGuest::getCS()
{
  return connectionStatus;
}
void TaskGuest::setCS(ConnectionStatus::ConnectionStatus cs)
{
  connectionStatus = cs;
}

// private:
void TaskGuest::main()
{
  recvLoop(sock, msg);
}
void TaskGuest::extractMessages(Buffer& buf, NetMessage& msg)
{
  std::unique_lock<std::mutex> lck(task_mutex);
  while (msg.build(buf))
  {  // tant qu'on a au moins un message complet
    buf.setLen(buf.getLen() - msg.getLen());
    if (buf.getLen() != 0)  // si on a lu + qu'un seul message (un message + autre chose, pas sur que ce soit un message complet)
      memmove(buf.getData(), buf.getData() + msg.getLen(), buf.getLen());  // on efface le premier message du buffer (et redécale le reste au début)
    if (!isOver())
    {
      lck.unlock();
      PostMessage(hwnd, ThreadMessage::NEWMESS, (WPARAM)this, 0);
      wait();  // on attend que threadMain traite le message
      lck.lock();
    }
  }
}
void TaskGuest::recvLoop(TCP_Socket& sock, NetMessage& msg)
{
  Buffer buf(NetMessage::MAXSIZE);
  timeval begin;
  timeval end;

  // TODO :
  // - vérifier que le buffer contient bien au mins un message complet : DONE
  // - Anti flood ? (mettre une limite de messages par seconde, sinon on pourrait planter le serveur en le floodant de messages, ce qui ferait exploser la file de messages du thread principal.
  gettimeofday(&begin, NULL);
  while (!isOver())
  {
    gettimeofday(&end, NULL);
    if (timevalDiff(begin, end) > 5000)
    {  // ttes les 5 secs on essaye d'envoyer un message pour voir si l'autre partie n'a pas perdu la connect
      msg.build(NetMsgId::PING);
      if (sock.send(msg.getBuffer().getData(), msg.getBuffer().getLen()) <= 0)
      {  // erreur ou fin de connexion
        PostMessage(hwnd, ThreadMessage::LOST_CONNECT, (WPARAM)this, 0);
        halt();
      }
      gettimeofday(&begin, NULL);
    }
    int len = sock.recv(buf.getData() + buf.getLen(), buf.getSize() - buf.getLen(), 50);  // on le fait en mode non bloquant pour pouvoir arrêter le thread
    if (len < 0)
    {  // erreur ou fin de connexion
      PostMessage(hwnd, ThreadMessage::LOST_CONNECT, (WPARAM)this, 0);
      halt();
    }
    else if (len > 0)
    {
      buf.setLen(buf.getLen() + len);
      extractMessages(buf, msg);
      if (buf.getLen() >= buf.getSize())
      {  // alors le message est incomplet, mais le buffer est plein, c'est une erreur
        PostMessage(hwnd, ThreadMessage::KICK, (WPARAM)this, 0);  // on vire le client/serveur parcequ'il nous envoie des messages invalides
        halt();
      }
    }
  }
  sock.close();
}
// fin TaskGuest

// ServerMain
ServerMain::ServerMain(const HINSTANCE hInst, const std::string wndClass)
    : Win32Window(hInst, "messwnd", wndClass, 0, 0, 0, true), queue(nullptr), window(hInst, "Chess", "Class_CHESS_SERVER", 400, 300, WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
{
  window.show(true);
}
ServerMain::~ServerMain()
{
  threadListen.halt();
  threadListen.signal();

  // on demande aux threads clients de s'arrêter
  for (auto tg: threadsGuests)
  {
    tg->halt();
    tg->signal();
  }

  //on les join dans une boucle à part parce qu'ils peuvent prendre un peu de temps (50 ms chacun) pour s'arrêter, autant demander aux autres de s'arrêter aussi pdt ce temps là, plutot que de les attendre un par un
  for (auto tg: threadsGuests)
  {
    tg->join();
    delete tg;
  }

  threadListen.join();

  for (auto guest: guests)
  {
    delete guest;
  }

  for (auto game: games)
  {
    delete game;
  }
}
int ServerMain::mainLoop()
{  // todo : tester les exceptions
  MSG msg;
  ZeroMemory(&msg, sizeof(msg));

  srand((unsigned int)time(nullptr));

  File dbInfo("db.ini");
  std::string host = dbInfo.readline();
  std::string user = dbInfo.readline();
  std::string pwd = dbInfo.readline();
  std::string dbName = dbInfo.readline();
  dbInfo.close();

  try
  {
    db.connect(host.c_str(), user.c_str(), pwd.c_str(), dbName.c_str());
  }
  catch (int e)
  {
    handleException(e);
  }

  threadListen.setHwnd(getHandle());
  threadListen.start();

  // Boucle de messages principale :
  while (GetMessage(&msg, nullptr, 0U, 0U))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}
LRESULT ServerMain::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == ThreadMessage::NEWCLIENT)
  {
    Thread<TaskGuest>* tg = new Thread<TaskGuest>(getHandle(), &threadListen.getGuest());
    //Thread<TaskGuest>* tg = new Thread<TaskGuest>();
    //tg->task.setHwnd(getHandle());
    //tg->task.setSock(threadListen.task.getGuest());
    Guest* guest = new Guest();
    threadsGuests.push_back(tg);
    guests.push_back(guest);
    tg->setGuest(guest);
    tg->setCS(ConnectionStatus::CONNECTING);  // le client est en train de se connecter (il doit encore s'identifier)
    tg->start();
    threadListen.signal();
    //MessageBox(nullptr, L"nouveau client", L"Info", MB_OK);
    send(tg, NetMsgId::ID);  // on demande au client de s'identifier
    // TODO : mise en place du timer (temps max pour s'identifier)
  }
  else if (message == ThreadMessage::NEWMESS)
  {
    //MessageBox(window.getHandle(), "Serveur : message reçu", "info", MB_OK);
    Thread<TaskGuest>* tg = (Thread<TaskGuest>*)wParam;
    if (!(tg->getCS() == ConnectionStatus::CONNECTED || tg->getCS() == ConnectionStatus::CONNECTING))
    {  // on vérifie qu'on doit bien écouter les messages de ce client
      return 0;
    }
    handleMessage(tg);
    tg->signal();
  }
  else if (message == ThreadMessage::ID_OK)
  {
    Thread<TaskGuest>* tg = (Thread<TaskGuest>*)wParam;
    // TODO : vérifier que cet utilisateur n'est pas déjà connecté... DONE (ailleurs)
    // ou qu'il a une partie en cours (si on arrive jusque là)
    tg->setCS(ConnectionStatus::CONNECTED);  // on change le statut de connexion pour pouvoir recevoir immédiatement d'éventuels messages et éviter de les jeter par erreur
    send(tg, NetMsgId::ID_OK);
  }
  else if (message == ThreadMessage::ID_INVALID)
  {
    Thread<TaskGuest>* tg = (Thread<TaskGuest>*)wParam;
    tg->setCS(ConnectionStatus::OFFLINE);  // les messages envoyés par ce client seront ignorés (de tte façon on va le kick, mais bon...)
    tg->halt();  // on arrête le thread ici pour éviter de recevoir par erreur un message de perte de connection
    send(tg, NetMsgId::ID_INVALID);
    PostMessage(getHandle(), ThreadMessage::KICK, wParam, 0);  // on le vire
  }
  else if (message == ThreadMessage::DISCONNECTED)
  {
    PostMessage(getHandle(), ThreadMessage::KICK, wParam, 0);
    //MessageBox(nullptr, L"Un client s'est déconnecté.", L"info", MB_OK);
  }
  else if (message == ThreadMessage::LOST_CONNECT)
  {
    PostMessage(getHandle(), ThreadMessage::KICK, wParam, 0);
    //MessageBox(nullptr, L"Un client a perdu la connection.", L"info", MB_OK);
  }
  else if (message == ThreadMessage::KICK)
  {
    Thread<TaskGuest>* tg = (Thread<TaskGuest>*)wParam;
    close(tg);
  }

  return 0;
}

// private:
void ServerMain::close(Thread<TaskGuest>* tg)
{
  tg->halt();
  tg->signal();  // on ne sait jamais qu'il soit justement sur son wait()
  tg->join();
  deleteGame(tg);
  deleteGuest(tg);
  if (tg == queue)
    queue = nullptr;  // reset de la queue;
  container_erase(threadsGuests, tg);
  delete tg;
}
bool ServerMain::deleteGuest(Thread<TaskGuest>* tg)
{
  bool ret = false;
  Guest* guest = tg->getGuest();
  if (guest && (ret = container_erase(guests, guest)))
    delete guest;
  return ret;
}
bool ServerMain::deleteGame(Thread<TaskGuest>* tg)
{
  bool ret = false;
  Chess::Game* game = tg->getGame();
  if (game && (ret = container_erase(games, game)))
  {
    //send(tg, NetMsgId::SURRENDER);
    delete game;
    tg->setGame(nullptr);
    Thread<TaskGuest>* other = tg->getOther();

    if (other &&
        std::find(threadsGuests.begin(), threadsGuests.end(), other) !=
            threadsGuests.end())
    {  // si other existe encore
      //send(other, NetMsgId::SURRENDER);
      other->setGame(nullptr);  // on doit lui indiquer que le jeu a été supprimé
    }
  }
  return ret;
}
int ServerMain::send(Thread<TaskGuest>* tg, unsigned int id, const void* data, unsigned int datalen)
{
  msg.build(id, data, datalen);
  //MessageBox(nullptr, L"Serveur : envoyer message ?", L"info", MB_OK);
  return tg->getSock().send(msg.getBuffer().getData(), msg.getBuffer().getLen());
}
bool ServerMain::handleMessage(Thread<TaskGuest>* tg)
{
  if (tg->getMsg().getID() == NetMsgId::ID && tg->getCS() == ConnectionStatus::CONNECTING)
  {  // le client demande qu'on vérifie ses identifiants
    // vérifier login et mdp
    if (DB_CheckLogin(db, tg->getGuest()->getLogin(), tg->getGuest()->getPassword()))  // mettre les requêtes db dans un thread à part ?
      PostMessage(getHandle(), ThreadMessage::ID_OK, (WPARAM)tg, 0);
    else
      PostMessage(getHandle(), ThreadMessage::ID_INVALID, (WPARAM)tg, 0);
  }
  else if (tg->getMsg().getID() == NetMsgId::LOGIN && tg->getCS() == ConnectionStatus::CONNECTING)
  {  // le client envoie son login
    bool notConnected = true;
    // on vérifie que le client n'est pas déjà identifié
    for (auto guest: guests)
    {
      if (guest->getLogin().compare((char*)tg->getMsg().getData()) == 0)
      {
        PostMessage(getHandle(), ThreadMessage::ID_INVALID, (WPARAM)tg, 0);
        notConnected = false;
        break;
      }
    }
    if (notConnected)  // si le guest n'est pas encore connecté on continue
      tg->getGuest()->setLogin((char*)tg->getMsg().getData());
  }
  else if (tg->getMsg().getID() == NetMsgId::PWD && tg->getCS() == ConnectionStatus::CONNECTING)
  {  // le client envoie son mdp
    tg->getGuest()->setPassword((char*)tg->getMsg().getData());
  }
  else if (tg->getMsg().getID() == NetMsgId::DISCO && tg->getCS() == ConnectionStatus::CONNECTED)
  {  // le client annonce qu'il va se déconnecter
    tg->setCS(ConnectionStatus::OFFLINE);  // on le prend au mot et on ne l'écoute plus
    tg->halt();  // on arrête le thread ici pour éviter de recevoir par erreur un message de perte de connection
    PostMessage(getHandle(), ThreadMessage::DISCONNECTED, (WPARAM)tg, 0);
  }
  else if (tg->getMsg().getID() == NetMsgId::GAME_RANDOM && tg->getCS() == ConnectionStatus::CONNECTED)
  {  // demande de partie
    if (!queue)
      queue = tg;
    else
    {
      Chess::Game* game = new Chess::Game();
      games.push_back(game);
      tg->setOther(queue);
      tg->setGame(game);
      queue->setOther(tg);
      queue->setGame(game);
      Chess::Player white =
          game->initForPlayers(queue->getGuest()->getLogin(), tg->getGuest()->getLogin());
      int w = Chess::C_WHITE;
      int b = Chess::C_BLACK;
      if (white.getName().compare(queue->getGuest()->getLogin()) == 0)
      {
        send(queue, NetMsgId::GAME_START, &w, sizeof(int));
        send(tg, NetMsgId::GAME_START, &b, sizeof(int));
      }
      else
      {
        send(tg, NetMsgId::GAME_START, &w, sizeof(int));
        send(queue, NetMsgId::GAME_START, &b, sizeof(int));
      }
      queue = nullptr;
    }
  }
  else if (tg->getMsg().getID() == NetMsgId::MOVE && tg->getCS() == ConnectionStatus::CONNECTED)
  {
    Chess::Game* game = tg->getGame();
    Thread<TaskGuest>* other = tg->getOther();
    if (!game || tg->getMsg().getDataLen() != 2 * sizeof(int))
    {
      return false;  // drop
    }
    int* tab = (int*)tg->getMsg().getData();
    /*int from = tab[0];
			int to = tab[1];*/
    Chess::Move move(tab[0], tab[1]);
    if (game->executeMove(move) != 0)
    {
      send(tg, NetMsgId::MOVE_INVALID);
      return false;
    }
    send(other, NetMsgId::MOVE, tab, 2 * sizeof(int));

    if (game->isCheckMate(game->getPlayerFrom(game->getTrait())) || game->isDraw(game->getPlayerFrom(game->getTrait())))
    {
      deleteGame(tg);
      tg->setOther(nullptr);
      other->setOther(nullptr);
    }
  }
  else if (tg->getMsg().getID() == NetMsgId::SURRENDER && tg->getCS() == ConnectionStatus::CONNECTED)
  {
    if (tg == queue)
      queue = nullptr;
    if (tg->getGame())
    {
      Thread<TaskGuest>* other = tg->getOther();
      send(other, NetMsgId::SURRENDER);
      deleteGame(tg);
      tg->setOther(nullptr);
      other->setOther(nullptr);
    }
  }

  return true;
}
// fin ServerMain
