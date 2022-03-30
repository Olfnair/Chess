#ifndef CLIENT_H
#define CLIENT_H

#include "Chess.h"
#include "Network.h"
#include "Thread.h"
#include "Win32Window.h"
#include "gui.h"
#include "status.h"

#include <windows.h>

template<typename T> class TaskV2 : public Task<T>
{
 public:
  TaskV2() = default;
  TaskV2(HWND hwnd) : hwnd(hwnd)
  {
  }
  virtual ~TaskV2() = default;
  TaskV2(const TaskV2&) = delete;
  TaskV2(TaskV2&&) = delete;
  void setHwnd(HWND hwnd)
  {
    this->hwnd = hwnd;
  }

 protected:
  HWND hwnd;  // handle vers la file de messages
};

class TaskRecvLoop : public TaskV2<void>
{  // le thread qui accepte les clients
 public:
  TaskRecvLoop() = default;
  virtual ~TaskRecvLoop() = default;
  TaskRecvLoop(const TaskRecvLoop&) = delete;
  TaskRecvLoop(TaskRecvLoop&&) = delete;
  TCP_SockClient& getGuest();
  NetMessage& getMsg();

 private:
  void recvLoop();
  bool sendPingMessage();
  int recvData(Buffer& buf);
  void extractMessages(Buffer& buf);
  int connect();
  void disconnect();
  virtual void main();

  TCP_SockClient guest;
  NetMessage msg;
};

class TaskIA : public TaskV2<void>
{
 public:
  TaskIA() = default;
  virtual ~TaskIA() = default;
  TaskIA(const TaskIA&) = delete;
  TaskIA(TaskIA&&) = delete;
  Chess::Minimax& getIA();

 private:
  virtual void main();

  Chess::Minimax ia;
};

class ClientMain : public Win32Window
{
 public:
  ClientMain(const HINSTANCE hInst, const std::string wndClass);
  virtual ~ClientMain();
  int mainLoop();
  LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

 private:
  int send(unsigned int id, void* data = nullptr, unsigned int datalen = 0);
  bool handleNetMsg();

  static const unsigned int CONNECT_TIMER_ID;

  ConnectionStatus::ConnectionStatus connectionStatus;
  MenuStatus::MenuStatus menuStatus;

  Chess::Game game;
  bool gameOver;

  Thread<TaskIA> threadIA;
  Thread<TaskRecvLoop> threadRecvLoop;

  ChessWin window;
  NetMessage msg;
  Buffer databuf;
};

#endif  // CLIENT_H
