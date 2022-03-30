#ifndef DB_H
#define DB_H

#include "tools.h"

//#include <winsock.h> // winsock2.h dans stdafx.h devrait faire l'affaire
#include <MariaDB/mysql.h>
#include <string>

extern const int MYSQL_EXC;

class DB_Connector
{
 public:
  DB_Connector();
  DB_Connector(const std::string& host, const std::string& user, const std::string& pwd, const std::string& db);
  ~DB_Connector();
  void connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& db);
  int query(const std::string& q);
  MYSQL_RES* getResult();
  my_ulonglong getRowCount();
  MYSQL* getObj();

 private:
  MYSQL* mysql;
};

// requêtes préparées :
bool DB_CheckLogin(DB_Connector& db, const std::string& login, const std::string& pwd);

#endif  // DB_H
