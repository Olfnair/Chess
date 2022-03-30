#ifndef NETWORK_H
#define NETWORK_H

#include "tools.h"

#include <WinSock2.h>
#include <cstdlib>
#include <string>

typedef int socklen_t;

extern const int SOCKET_EXC;

class SocketInit final
{  // juste un gros hack pour initialiser et clean les sockets automatiquement
 private:
  SocketInit() = default;  // juste pour empêcher qu'on instancie la classe : elle ne sert vraiment pas à ça.
  static class Init
  {  // En java on emploierait le bloc static
   public:
    Init();
    ~Init();
  } init;
};

class TCP_Socket
{
 public:
  TCP_Socket() = default;
  TCP_Socket(long addr, int port);
  virtual ~TCP_Socket();
  void init(long addr, int port);
  void init(SOCKET sock, SOCKADDR_IN sin);
  int close();
  int send(const char* buf, int len);
  int recv(char* buf, int len);
  int recv(char* buf, int len, int timeout);

 protected:
  SOCKADDR_IN* getSin();
  SOCKET* getSock();

 private:
  SOCKET sock;  // socket
  SOCKADDR_IN sin;  // contexte d'adressage
};

class TCP_SockServer : public TCP_Socket
{
 public:
  TCP_SockServer(int port, int backlog);
  virtual ~TCP_SockServer();
  int listen(int backlog);
  void accept(TCP_Socket& socket);
  bool accept(TCP_Socket& socket, int timeout);
};

class TCP_SockClient : public TCP_Socket
{
 public:
  TCP_SockClient() = default;
  TCP_SockClient(long addr, int port);
  virtual ~TCP_SockClient();
  int close();
  int connect(long addr, int port);
  int connect();
};

//       Sructure d'un message :
//        4 octects | 4 octets | MAXSIZE - 4 octets
//         longueur | id       | data eventuelles
//
class NetMessage
{
 public:  // htonl & ntohl pour gerer l'endianness des int
  static const int MAXSIZE = 1024;

  NetMessage();
  ~NetMessage() = default;
  bool build(unsigned int id, const void* data = NULL, unsigned int datalen = 0);
  bool build(const void* data, unsigned int datalen);
  bool build(const Buffer& buffer);
  unsigned int getLen();
  unsigned int getID();
  unsigned int getDataLen();
  void* getData();
  Buffer& getBuffer();

 private:
  Buffer buf;
  unsigned int len;
  unsigned int messId;
  void* data;
  unsigned int datalen;
};

class Guest
{
 public:
  Guest();
  ~Guest() = default;
  void setLogin(std::string login);
  void setPassword(std::string password);
  void erasePassword();
  const std::string& getLogin() const;
  const std::string& getPassword() const;

 private:
  std::string login;
  std::string password;
};

#endif  // NETWORK_H
