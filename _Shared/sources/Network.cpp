#include "Network.h"
#include "tools.h"

const int SOCKET_EXC = uniqueExceptionID.generate();

// SocketInit
SocketInit::Init SocketInit::init;  // Si c'est obscur, c'est pas grave. Comme c'est écrit, ça sert à initialiser.

SocketInit::Init::Init()
{
  WSADATA WSAData;
  int error = WSAStartup(MAKEWORD(2, 2), &WSAData);
  if (error)
  {
    std::u16string u16errorStr =
        UTF8_TO_UTF16.from_bytes("Error initializing sockets! Error Code: " + uint_to_str(error, DEC));
    MessageBox(nullptr, (LPCWSTR)u16errorStr.data(), L"Error", MB_ICONERROR | MB_OK);
    exit(-1);
  }
}
SocketInit::Init::~Init()
{
  WSACleanup();
}
// fin SocketInit

// TCP_Socket
TCP_Socket::TCP_Socket(long addr, int port)
{
  init(addr, port);
}
TCP_Socket::~TCP_Socket()
{
  close();
}
void TCP_Socket::init(long addr, int port)
{
  sock = socket(AF_INET, SOCK_STREAM, 0);  // init en mode TCP
  if (INVALID_SOCKET == sock)
    throw SOCKET_EXC;
  sin.sin_addr.s_addr = addr;  // htonl(INADDR_ANY); // inet_addr("127.0.0.1");
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
}
void TCP_Socket::init(SOCKET sock, SOCKADDR_IN sin)
{
  this->sock = sock;
  this->sin = sin;
}
int TCP_Socket::close()
{
  int ret = closesocket(sock);
  sock = INVALID_SOCKET;
  return ret;
}
int TCP_Socket::send(const char* buf, int len)
{  // envoyer le contenu du buffer
  return ::send(sock, buf, len, 0);
}  // SOCKET_ERROR en cas d'erreur, sinon le nbre d'octets envoyés
int TCP_Socket::recv(char* buf, int len)
{  // lire ce qu'on a reçu sur une longueur max len et le mettre dans buf (bloquant)
  return ::recv(sock, buf, len, 0);  // pas les flags
}  // SOCKET_ERROR en cas d'erreur, sinon le nbre de char lus, 0 en cas de déco propre (ou select sans rien avoir reçu)
int TCP_Socket::recv(char* buf, int len, int timeout)
{  // recv avec timeout (non bloquant ou moins bloquant...) (timeout en ms)
  timeval t;
  fd_set fdsock;
  int ret = 0;

  FD_ZERO(&fdsock);
  FD_SET(sock, &fdsock);
  millisec2timeval(timeout, &t);
  if (select(sock + 1, &fdsock, nullptr, nullptr, &t) < 0)
  {  // attend le temps indiqué par timeout
    WSAGetLastError();
    throw SOCKET_EXC;  // exception
  }
  if (FD_ISSET(sock, &fdsock))
  {  // si on a reçu quelque chose sur la socket
    ret = recv(buf, len);  // on utilise recv pour le lire
    if (ret == 0)
      ret = SOCKET_ERROR;
  }
  FD_CLR(sock, &fdsock);
  return ret;
}

// protected:
SOCKADDR_IN* TCP_Socket::getSin()
{
  return &sin;
}
SOCKET* TCP_Socket::getSock()
{
  return &sock;
}
// fin TCP_Socket

// TCP_SockServer
TCP_SockServer::TCP_SockServer(int port, int backlog)
    : TCP_Socket(htonl(INADDR_ANY), port)
{  // INADDR_ANY... En fait on voudra peut-être préciser l'interface à écouter.. on verra
  if (SOCKET_ERROR == bind(*getSock(), (SOCKADDR*)getSin(), sizeof(SOCKADDR_IN)))  // on lie l'adresse locale au socket
    throw SOCKET_EXC;  // générer exception : le port est probablement déjà utilisé
  if (SOCKET_ERROR == listen(backlog))  // on écoute les connexions entrantes
    throw SOCKET_EXC;  // exception
}
TCP_SockServer::~TCP_SockServer()
{
  close();
}
int TCP_SockServer::listen(int backlog)
{  // écoute les connexions entrantes à cette adresse sur le port précisé. backlog indique le nbre max de connexions en attente.
  return ::listen(*getSock(), backlog);
}  // SOCKET_ERROR en cas d'erreur
void TCP_SockServer::accept(TCP_Socket& socket)
{  // accepter une connexion entrante (bloquant)
  SOCKET cs = INVALID_SOCKET;
  SOCKADDR_IN csin;
  socklen_t csinsize = sizeof(SOCKADDR_IN);
  cs = ::accept(*getSock(), (SOCKADDR*)&csin, &csinsize);
  if (cs == INVALID_SOCKET)
    throw SOCKET_EXC;  // exception
  socket.init(cs, csin);
}
bool TCP_SockServer::accept(TCP_Socket& socket, int timeout)
{  // accept avec timeout
  timeval t;
  fd_set fdsock;
  bool ret = false;

  SOCKET sock = *getSock();
  FD_ZERO(&fdsock);
  FD_SET(sock, &fdsock);
  millisec2timeval(timeout, &t);
  if (select(sock + 1, &fdsock, nullptr, nullptr, &t) < 0)
  {  // attend le temps indiqué par timeout
    WSAGetLastError();
    throw SOCKET_EXC;  // exception
  }
  if (FD_ISSET(sock, &fdsock))
  {  // si on a reçu quelque chose sur la socket
    accept(socket);
    ret = true;
  }
  FD_CLR(sock, &fdsock);
  return ret;
}
// fin TCP_SockServer

// TCP_SockClient
TCP_SockClient::TCP_SockClient(long addr, int port) : TCP_Socket(addr, port)
{
  if (SOCKET_ERROR == connect())
    throw SOCKET_EXC;
}
TCP_SockClient::~TCP_SockClient()
{
  close();
}
int TCP_SockClient::close()
{
  shutdown(*getSock(), SD_BOTH);
  int ret = closesocket(*getSock());
  *getSock() = INVALID_SOCKET;
  return ret;
}
int TCP_SockClient::connect(long addr, int port)
{
  init(addr, port);
  return connect();
}
int TCP_SockClient::connect()
{
  return ::connect(*getSock(), (SOCKADDR*)getSin(), sizeof(SOCKADDR_IN));
}  // return -1 en cas d'erreur, sinon 0
// fin

// NetMessage
NetMessage::NetMessage() : buf(MAXSIZE + 1)
{  // + 1 pour pouvoir ajouter un /0 après les données reçues
}
bool NetMessage::build(unsigned int id, const void* data, unsigned int datalen)
{  // construit un message à envoyer
  unsigned int len = sizeof(unsigned int) * 2 + datalen;
  if (len > MAXSIZE)
    return false;
  this->len = len;
  this->messId = id;
  this->data = buf.getData() + sizeof(unsigned int) * 2;
  this->datalen = datalen;
  buf.setLen(len);
  len = htonl(len);  // on convertit de l'endianness host vers big endian
  id = htonl(id);
  buf.write(0, &len, sizeof(unsigned int));
  buf.write(sizeof(unsigned int), &id, sizeof(unsigned int));
  if (data && datalen > 0)
    buf.write(sizeof(unsigned int) * 2, data, datalen);
  return true;
}
bool NetMessage::build(const void* data, unsigned int datalen)
{  // découpe un message reçu
  if (datalen > MAXSIZE || datalen < sizeof(unsigned int) * 2)  // le message est trop long ou incomplet
    return false;
  this->len = *(unsigned int*)data;  // les 4 premiers octects indiquent la longueur du message
  this->len = ntohl(this->len);  // on convertit de big endian vers l'endianness host
  if (datalen < this->len)  // on n'a pas toutes les data du message
    return false;
  buf.write(0, data, this->len);  // on copie dans le buffer recv dans le buffer du message sur la longueur du message (pour extraire juste le premier message)
  buf.setLen(this->len);  // on indique sa longueur au buffer message
  buf.read(sizeof(unsigned int), &(this->messId), sizeof(unsigned int));  // on récupère l'ID du message
  this->messId = ntohl(this->messId);  // on convertit de big endian vers l'endianness host
  this->data = buf.getData() + sizeof(unsigned int) * 2;
  this->datalen = this->len - sizeof(unsigned int) * 2;
  char term = 0;
  buf.write(this->len, &term, sizeof(char));  // on ajoute un '/0' à la fin du message reçu pour éviter les erreurs (buffer overflow..)
  return true;
}
bool NetMessage::build(const Buffer& buffer)
{
  return build(buffer.getData(), buffer.getLen());
}
unsigned int NetMessage::getLen()
{
  return len;
}
unsigned int NetMessage::getID()
{
  return messId;
}
unsigned int NetMessage::getDataLen()
{
  return datalen;
}
void* NetMessage::getData()
{
  return data;
}
Buffer& NetMessage::getBuffer()
{
  return buf;
}
// fin NetMessage

// Guest
Guest::Guest()
{
  login = "";
  password = "";
}
void Guest::setLogin(std::string login)
{
  this->login = login;
}
void Guest::setPassword(std::string password)
{
  this->password = password;
}
void Guest::erasePassword()
{
  password = "";
}
const std::string& Guest::getLogin() const
{
  return login;
}
const std::string& Guest::getPassword() const
{
  return password;
}
// fin
