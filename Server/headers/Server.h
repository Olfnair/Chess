#ifndef SERVER_H
#define SERVER_H

#include "Chess.h"
#include "Network.h"
#include "Thread.h"
#include "Win32Window.h"
#include "db.h"
#include "status.h"

#include <algorithm>
#include <list>
#include <windows.h>

template<typename Container, typename Element> bool container_erase(Container& c, Element& e)
{
  auto it = std::find(c.begin(), c.end(), e);
  if (it != c.end())
  {
    c.erase(it);
    return true;
  }
  return false;
}

void handleException(int e);

class SharedGame
{
 public:
  SharedGame() = default;
  SharedGame(const SharedGame&) = delete;
  SharedGame(SharedGame&&) = delete;
  ~SharedGame() = default;
  void lock();
  void unlock();

 public:
  Chess::Game* game;

 private:
  std::mutex mutex;
};

class TaskV2 : public Task<void>
{
 public:
  TaskV2() = default;
  TaskV2(HWND hwnd) : hwnd(hwnd)
  {
  }
  virtual ~TaskV2() = default;
  TaskV2(const TaskV2&) = delete;
  TaskV2(TaskV2&&) = delete;
  void setHwnd(HWND hwnd);

 protected:
  HWND hwnd;  // handle vers la file de messages
};

class TaskListen : public TaskV2
{  // le thread qui accepte les clients
 public:
  TaskListen() = default;
  virtual ~TaskListen() = default;
  TaskListen(const TaskListen&) = delete;
  TaskListen(TaskListen&&) = delete;
  TCP_Socket& getGuest();

 private:
  virtual void main();

 private:
  TCP_Socket guest;
};

class TaskGuest : public TaskV2
{  // thread qui écoute les messages envoyés par un client : un par client connecté
 public:
  TaskGuest();
  TaskGuest(HWND hwnd, const TCP_Socket* sock);
  virtual ~TaskGuest() = default;
  TaskGuest(const TaskGuest&) = delete;
  TaskGuest(TaskGuest&&) = delete;
  TCP_Socket& getSock();
  void setSock(const TCP_Socket& sock);
  NetMessage& getMsg();
  Guest* getGuest();
  void setGuest(Guest* guest);
  Thread<TaskGuest>* getOther();
  void setOther(Thread<TaskGuest>* other);
  Chess::Game* getGame();
  void setGame(Chess::Game* game);
  ConnectionStatus::ConnectionStatus getCS();
  void setCS(ConnectionStatus::ConnectionStatus cs);

 private:
  virtual void main();
  void extractMessages(Buffer& buf, NetMessage& msg);
  void recvLoop(TCP_Socket& sock, NetMessage& msg);

 private:
  TCP_Socket sock;
  NetMessage msg;
  ConnectionStatus::ConnectionStatus connectionStatus;
  Guest* guest;
  SharedGame sgame;
  Thread<TaskGuest>* other;  // l'autre joueur
};

class ServerMain : public Win32Window
{
 public:
  ServerMain(const HINSTANCE hInst, const std::string wndClass);
  ~ServerMain();
  int mainLoop();
  LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

 private:
  void close(Thread<TaskGuest>* tg);
  bool deleteGuest(Thread<TaskGuest>* tg);
  bool deleteGame(Thread<TaskGuest>* tg);
  int send(Thread<TaskGuest>* tg, unsigned int id, const void* data = nullptr, unsigned int datalen = 0);
  bool handleMessage(Thread<TaskGuest>* tg);

 private:
  DB_Connector db;
  Thread<TaskListen> threadListen;
  std::list<Thread<TaskGuest>*> threadsGuests;
  std::list<Guest*> guests;  // client connectés
  std::list<Chess::Game*> games;  // parties en cours
  Thread<TaskGuest>* queue;  // le client en attente d'une partie
  Win32Window window;
  NetMessage msg;
};

#endif  // SERVER_H
