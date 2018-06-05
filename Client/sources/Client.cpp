#include "Client.h"

#include "NetConst.h"
#include "ThreadConst.h"
#include "Win32Window.h"
#include "tools.h"

//#include <cstdlib>
//#include <ctime>
//#include <windows.h>
//#include <ws2tcpip.h>

// TaskRecvLoop
TCP_SockClient& TaskRecvLoop::getGuest() {
  return guest;
}
NetMessage& TaskRecvLoop::getMsg() {
  return msg;
}

// private:
void TaskRecvLoop::recvLoop() {
  Buffer buf(NetMessage::MAXSIZE);
  timeval begin;
  timeval end;

  // TODO :
  // - vérifier que le buffer contient bien au mins un message complet : DONE
  // - Anti flood ? (mettre une limite de messages par seconde, sinon on pourrait planter le serveur en le floodant de messages, ce qui ferait exploser la file de messages du thread principal.
  gettimeofday(&begin, nullptr);
  while (!isOver()) {
    gettimeofday(&end, nullptr);
    if (timevalDiff(begin, end) > 5000) { // ttes les 5 secs on essaye d'envoyer un message pour voir si l'autre partie n'a pas perdu la connect
      if(! sendPingMessage()) {
        PostMessage(hwnd, ThreadMessage::LOST_CONNECT, (WPARAM)this, 0);
        halt();
      }
      gettimeofday(&begin, nullptr);
    }
    int len = recvData(buf);
    if (len < 0) { // erreur ou fin de connexion
      PostMessage(hwnd, ThreadMessage::LOST_CONNECT, (WPARAM)this, 0);
      halt();
    } else if (len > 0) {
      buf.setLen(buf.getLen() + len);
      extractMessages(buf);
      if (buf.getLen() >= buf.getSize()) {                       // alors le message est incomplet, mais le buffer est plein, c'est une erreur
        PostMessage(hwnd, ThreadMessage::KICK, (WPARAM)this, 0); // on vire le client/serveur parcequ'il nous envoie des messages invalides
        halt();
      }
    }
  }
  disconnect();
}
bool TaskRecvLoop::sendPingMessage() {
  NetMessage message;
  message.build(NetMsgId::PING);
  std::unique_lock<std::mutex> lock(task_mutex);
  return guest.send(message.getBuffer().getData(), message.getBuffer().getLen()) > 0;
}
int TaskRecvLoop::recvData(Buffer& buf) {
  std::unique_lock<std::mutex> lock(task_mutex);
  return guest.recv(buf.getData() + buf.getLen(), buf.getSize() - buf.getLen(), 50); // on le fait en mode non bloquant pour pouvoir arrêter le thread
}
void TaskRecvLoop::extractMessages(Buffer& buf) {
  // lock msg ? normalement le wait() et signal devraient être suffisants dans ce cas-ci.
  std::unique_lock<std::mutex> lck(task_mutex);
  while (msg.build(buf)) { // tant qu'on a au moins un message complet
    buf.setLen(buf.getLen() - msg.getLen());
    if (buf.getLen() != 0)                                                // si on a lu + qu'un seul message (un message + autre chose, pas sur que ce soit un message complet)
      memmove(buf.getData(), buf.getData() + msg.getLen(), buf.getLen()); // on efface le premier message du buffer (et redécale le reste au début)
    if (!isOver()) {
      lck.unlock();
      PostMessage(hwnd, ThreadMessage::NEWMESS, (WPARAM)this, 0);
      wait(); // on attend que threadMain traite le message
      lck.lock();
    }
  }
}
int TaskRecvLoop::connect() {
  File serverInfo("server.ini");
  std::string ip = serverInfo.readline();
  std::string port = serverInfo.readline();
  serverInfo.close();
  std::unique_lock<std::mutex> lock(task_mutex);
  return guest.connect(inet_addr(ip.c_str()), char_to_uint(port.c_str(), DEC)); // faudrait utiliser inet_pton, mais la flemme... (inet_addr est deprecated)
}
void TaskRecvLoop::disconnect() {
  std::unique_lock<std::mutex> lock(task_mutex);
  guest.close();
}
void TaskRecvLoop::main() {
  int ret;

  try {
    ret = connect(); // vérifier exceptions
  } catch (int) {
    // notifier echec de connection
    PostMessage(hwnd, ThreadMessage::CONNECT_FAIL, 0, 0);
    return;
  }
  if (ret != 0) {
    PostMessage(hwnd, ThreadMessage::CONNECT_FAIL, 0, 0);
    return;
  }

  recvLoop();
}
// finTaskRecvLoop

// TaskIA
Chess::Minimax& TaskIA::getIA() {
  return ia;
}
// private:
void TaskIA::main() {
  Chess::Move move = ia.getBestMove(4); // profondeur
  PostMessage(hwnd, ThreadMessage::CHECKMOVE, move.getFrom(), move.getTo());
}
// fin TaskIA

// ClientMain
const unsigned int ClientMain::CONNECT_TIMER_ID = uniqueTimerID.generate();

ClientMain::ClientMain(const HINSTANCE hInst, const std::string wndClass) : Win32Window(hInst, "messwnd", wndClass, 0, 0, 0, true),
                                                                            connectionStatus(ConnectionStatus::OFFLINE), menuStatus(MenuStatus::MAIN), window(hInst, "Chess", "Class_CHESS", 1024, 768, WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX), databuf(NetMessage::MAXSIZE) {
  threadRecvLoop.setHwnd(getHandle());
  threadIA.setHwnd(getHandle());
  window.setMessWndHandle(getHandle());
  window.show(true);
}

ClientMain::~ClientMain() {
  if (threadIA.hasStarted())
    threadIA.join();
  // arrêt de la boucle de reception
  if (connectionStatus == ConnectionStatus::CONNECTED) {
    send(NetMsgId::SURRENDER); // on prévient son adversaire qu'on se rend
    send(NetMsgId::DISCO);     // on prévient le serveur qu'on se déconnecte pour qu'il ne confonde pas avec une perte de connexion
  }
  if (connectionStatus == ConnectionStatus::CONNECTED || connectionStatus == ConnectionStatus::CONNECTING) {
    threadRecvLoop.halt();
    threadRecvLoop.join();
  }
}

int ClientMain::mainLoop() { // todo : tester les exceptions
  MSG msg;
  ZeroMemory(&msg, sizeof(msg));

  srand((unsigned int)time(nullptr));
  // Boucle de messages principale :
  while (GetMessage(&msg, nullptr, 0U, 0U)) {
    if (!IsDialogMessage(window.getHandle(), &msg)) { // pour pouvoir tab
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

LRESULT ClientMain::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == WM_ACTIVATE)
    InvalidateRect(getHandle(), nullptr, TRUE);
  if (message == WM_TIMER && wParam == CONNECT_TIMER_ID && connectionStatus == ConnectionStatus::CONNECTING) {
    // le serveur ne nous a pas identifié dans les temps
    KillTimer(getHandle(), CONNECT_TIMER_ID);
    threadRecvLoop.halt();
    threadRecvLoop.join();
    connectionStatus = ConnectionStatus::OFFLINE;
    window.setCS(connectionStatus);
    MessageBox(window.getHandle(), L"Le serveur ne répond pas dans les temps !", L"Erreur", MB_ICONERROR);
  } else if (message == ThreadMessage::CONNECT) {
    if (connectionStatus == ConnectionStatus::OFFLINE) {
      //MessageBox(window.getHandle(), "Tentative de connexion en cours", "Info", MB_OK);
      threadRecvLoop.start(); // connexion et écoute des messages
      connectionStatus = ConnectionStatus::CONNECTING;
      window.setCS(connectionStatus);
      SetTimer(getHandle(), CONNECT_TIMER_ID, 5000, nullptr); // on laisse au serveur 5 secondes pour accepter la connection et nous identifier
    } else if (connectionStatus == ConnectionStatus::CONNECTED) {
      send(NetMsgId::DISCO);
      threadRecvLoop.halt();
      threadRecvLoop.join();
      connectionStatus = ConnectionStatus::OFFLINE;
      window.setCS(connectionStatus);
    }
  } else if (message == ThreadMessage::CONNECT_FAIL && connectionStatus == ConnectionStatus::CONNECTING) {
    KillTimer(getHandle(), CONNECT_TIMER_ID);
    threadRecvLoop.join();
    connectionStatus = ConnectionStatus::OFFLINE;
    window.setCS(connectionStatus);
    MessageBox(window.getHandle(), L"Impossible de joindre le serveur de jeu !", L"Erreur", MB_ICONERROR);
  } else if (message == ThreadMessage::ID_OK && connectionStatus == ConnectionStatus::CONNECTING) {
    KillTimer(getHandle(), CONNECT_TIMER_ID);
    connectionStatus = ConnectionStatus::CONNECTED;
    window.setCS(connectionStatus);
    MessageBox(window.getHandle(), L"Connexion établie.", L"Info", MB_OK);
  } else if (message == ThreadMessage::ID_INVALID && connectionStatus == ConnectionStatus::CONNECTING) {
    KillTimer(getHandle(), CONNECT_TIMER_ID);
    connectionStatus = ConnectionStatus::OFFLINE;
    window.setCS(connectionStatus);
    MessageBox(window.getHandle(), L"Erreur lors de l'identification : vérifiez vos login et mot de passe !", L"Erreur", MB_ICONERROR);
  } else if (message == ThreadMessage::LOST_CONNECT && (connectionStatus == ConnectionStatus::CONNECTING || connectionStatus == ConnectionStatus::CONNECTED)) {
    threadRecvLoop.join();
    connectionStatus = ConnectionStatus::OFFLINE;
    window.setCS(connectionStatus);
    MessageBox(window.getHandle(), L"Connexion au serveur perdue.", L"Info", MB_OK);
  } else if (message == ThreadMessage::SOLO && menuStatus == MenuStatus::MAIN) {
    window.gamePanelSetPos(false);
    game.initForPlayers("Joueur", "IA");
    window.gameSetWhitePlayerName(game.getWhitePlayer().getName());
    window.gameSetBlackPlayerName(game.getBlackPlayer().getName());
    window.showGamePanel();
    window.gameSetSide2Move(game.getTrait());
    window.gameSetPlayerSide(game.getPlayerFromName("Joueur").getColor());
    gameOver = false;
    menuStatus = MenuStatus::GAME;

    threadIA.getIA().setColor(game.getPlayerFromName("IA").getColor());
    if (game.getPlayerFromName("IA").getColor() == game.getTrait()) {
      threadIA.getIA().setGame(&game);
      threadIA.start();
    }
  } else if (message == ThreadMessage::MULTI && menuStatus == MenuStatus::MAIN) {
    /*window.gamePanelSetPos(true);
			window.showGamePanel();*/
    if (connectionStatus != ConnectionStatus::CONNECTED)
      MessageBox(window.getHandle(), L"Vous devez vous connecter pour pouvoir accéder au multijoueur !", L"Erreur", MB_ICONERROR);
    else {
      window.gamePanelSetPos(false);
      menuStatus = MenuStatus::GAME_MULTI;
      window.showWaitPanel();
      send(NetMsgId::GAME_RANDOM);
    }
  } else if (message == ThreadMessage::EXIT && menuStatus == MenuStatus::MAIN) {
    PostMessage(window.getHandle(), WM_CLOSE, 0, 0); // window postera un WM_QUIT et arrêtera le main loop
  } else if (message == ThreadMessage::BACKTOMAIN) {
    gameOver = true;
    if (threadIA.hasStarted()) {
      threadIA.join();
    }
    if (menuStatus == MenuStatus::GAME_MULTI)
      send(NetMsgId::SURRENDER);
    menuStatus = MenuStatus::MAIN;
    window.showMainMenu();
  } else if (message == ThreadMessage::SELECT) {
    Chess::Pos pos((int)wParam);
    if (gameOver)
      MessageBox(window.getHandle(), L"La partie est terminée !", L"Info", MB_OK);
    else if (!threadIA.hasStarted() && game.getPlayerFromName("Joueur").getColor() == game.getTrait() && game.validateSelect(pos))
      window.gameAllowSelection();
    else if (game.getPlayerFromName("Joueur").getColor() == game.getTrait())
      MessageBox(window.getHandle(), L"Cette pièce n'est pas à vous !", L"Erreur", MB_ICONERROR);
    else
      MessageBox(window.getHandle(), L"Ce n'est pas à vous de jouer !", L"Erreur", MB_ICONERROR);
  } else if (message == ThreadMessage::CHECKMOVE && (menuStatus == MenuStatus::GAME || menuStatus == MenuStatus::GAME_MULTI)) {
    Chess::Move move(Chess::Pos((int)wParam), Chess::Pos((int)lParam));
    window.gameDisableSelection();
    int ret = game.executeMove(move);

    if (threadIA.hasStarted()) {
      threadIA.join();
    }

    for (int i = 0; i < Chess::X_SIZE * Chess::Y_SIZE; ++i) // on pourrait faire plus fin, mais bon...
      window.gameSetPiece(i, game.getBoard().getPiece(Chess::Pos(i)));
    window.gameSetSide2Move(game.getTrait());
    window.gameInvalidate();

    if (ret == 0 && menuStatus == MenuStatus::GAME_MULTI) {
      int tab[] = {move.getFrom(), move.getTo()};
      send(NetMsgId::MOVE, tab, sizeof(int) * 2);
    }
    if (ret == 1)
      //MessageBox(nullptr, std::string("Erreur executeMove : \nFrom : " + uint_to_str(move.getFrom().getX(), DEC) + " , " + uint_to_str(move.getFrom().getY(), DEC) + "\nTo : " + uint_to_str(move.getTo().getX(), DEC) + " , " + uint_to_str(move.getTo().getY(), DEC)).c_str(), "Erreur", MB_OK);
      MessageBox(window.getHandle(), L"Sélection Invalide !", L"Erreur", MB_ICONERROR);
    else if (ret == 2)
      MessageBox(window.getHandle(), L"Coup Illégal !", L"Erreur", MB_ICONERROR);
    else if (ret == 3)
      MessageBox(window.getHandle(), L"Vous ne pouvez pas roquer !", L"Erreur", MB_ICONERROR);
    else if (ret == 4)
      MessageBox(window.getHandle(), L"Vous ne pouvez pas mettre ou laisser votre roi en échec !", L"Erreur", MB_ICONERROR);
    else if (game.isCheckMate(game.getPlayerFrom(game.getTrait()))) {
      if (game.getPlayerFrom(game.getTrait()).getName().compare("Joueur") == 0)
        MessageBox(window.getHandle(), L"Echec et Mat ! Vous avez perdu.", L"Info", MB_OK);
      else
        MessageBox(window.getHandle(), L"Echec et Mat ! Vous avez gagné.", L"Info", MB_OK);
      gameOver = true;
    } else if (game.isDraw(game.getPlayerFrom(game.getTrait()))) {
      MessageBox(window.getHandle(), L"Partie nulle", L"Info", MB_OK);
      gameOver = true;
    } else if (menuStatus == MenuStatus::GAME && game.getPlayerFromName("IA").getColor() == game.getTrait()) {
      threadIA.getIA().setGame(&game);
      threadIA.start();
    }
  } else if ((message == ThreadMessage::NEWMESS && connectionStatus == ConnectionStatus::CONNECTING) || connectionStatus == ConnectionStatus::CONNECTED) {
    //MessageBox(window.getHandle(), "Client : Message reçu", "info", MB_OK);
    handleNetMsg();
    threadRecvLoop.signal(); // on prévient la boucle de réception qu'elle peut continuer de lire les messages reçus
  }
  return 0;
}

// private:
int ClientMain::send(unsigned int id, void* data, unsigned int datalen) {
  msg.build(id, data, datalen);
  return threadRecvLoop.getGuest().send(msg.getBuffer().getData(), msg.getBuffer().getLen());
}

bool ClientMain::handleNetMsg() {
  int len;
  NetMessage& recvmsg = threadRecvLoop.getMsg();

  if (recvmsg.getID() == NetMsgId::ID && connectionStatus == ConnectionStatus::CONNECTING) { // le serveur nous demande de nous identifier
    len = window.getLogin(databuf.getData(), 25);
    send(NetMsgId::LOGIN, databuf.getData(), len);
    len = window.getPassword(databuf.getData(), 25);
    send(NetMsgId::PWD, databuf.getData(), len);
    send(NetMsgId::ID);
  }

  else if (recvmsg.getID() == NetMsgId::ID_OK && connectionStatus == ConnectionStatus::CONNECTING) {
    // le serveur accepte nos identifiants
    PostMessage(getHandle(), ThreadMessage::ID_OK, 0, 0);
  } else if (recvmsg.getID() == NetMsgId::ID_INVALID && connectionStatus == ConnectionStatus::CONNECTING) {
    // il y a eu un problème lors de l'identification
    PostMessage(getHandle(), ThreadMessage::ID_INVALID, 0, 0);
  } else if (recvmsg.getID() == NetMsgId::GAME_START) {
    int c = *(int*)recvmsg.getData();
    Chess::Player white;
    Chess::Player black;
    white.setColor(Chess::C_WHITE);
    black.setColor(Chess::C_BLACK);
    if (c == (int)Chess::C_WHITE) {
      white.setName("Joueur");
      black.setName("Adversaire");
      window.gameSetWhitePlayerName("Joueur");
      window.gameSetBlackPlayerName("Adversaire");
    } else {
      white.setName("Adversaire");
      black.setName("Joueur");
      window.gameSetWhitePlayerName("Adversaire");
      window.gameSetBlackPlayerName("Joueur");
    }
    game.reset();
    game.setWhitePlayer(white);
    game.setBlackPlayer(black);
    gameOver = false;
    menuStatus = MenuStatus::GAME_MULTI;
    window.showGamePanel();
    window.gameSetSide2Move(game.getTrait());
    window.gameSetPlayerSide(game.getPlayerFromName("Joueur").getColor());
  } else if (recvmsg.getID() == NetMsgId::MOVE) {
    int* tab = (int*)recvmsg.getData();
    PostMessage(getHandle(), ThreadMessage::CHECKMOVE, tab[0], tab[1]);
  } else if (recvmsg.getID() == NetMsgId::SURRENDER && menuStatus == MenuStatus::GAME_MULTI && !gameOver) {
    gameOver = true;
    MessageBox(window.getHandle(), L"Votre adversaire a abandonné", L"Info", MB_OK);
  }
  return true;
}
// fin ClientMain
