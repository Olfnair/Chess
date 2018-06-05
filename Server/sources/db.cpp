#include "db.h"
#include "tools.h"

#include <string>

const int MYSQL_EXC = uniqueExceptionID.generate();

// DB_Connector
DB_Connector::DB_Connector() {
  if (!(mysql = mysql_init(nullptr)))
    throw MYSQL_EXC;
}
DB_Connector::DB_Connector(const std::string& host, const std::string& user, const std::string& pwd, const std::string& db) {
  if (!(mysql = mysql_init(nullptr)))
    throw MYSQL_EXC;
  connect(host, user, pwd, db);
}
DB_Connector::~DB_Connector() {
  mysql_close(mysql);
}
void DB_Connector::connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& db) {
  mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "option");
  if (!mysql_real_connect(mysql, host.c_str(), user.c_str(), pwd.c_str(), db.c_str(), 0, NULL, 0))
    throw MYSQL_EXC;
}
int DB_Connector::query(const std::string& q) {
  /*char q_escaped[4096 * 2 + 1];
		if (q.size() > 4096)
			return -1;
		mysql_real_escape_string(mysql, q_escaped, q.c_str(), q.size());*/
  return mysql_query(mysql, q.c_str());
}
MYSQL_RES* DB_Connector::getResult() {
  return mysql_store_result(mysql);
}
my_ulonglong DB_Connector::getRowCount() {
  MYSQL_RES* result = getResult();
  my_ulonglong count = mysql_num_rows(result);
  mysql_free_result(result);
  return count;
}
MYSQL* DB_Connector::getObj() {
  return mysql;
}
// fin DB_Connector

// renvoie vrai si login et mdp forment une paire d'identifiants valide
bool DB_CheckLogin(DB_Connector& db, const std::string& login, const std::string& pwd) {
  std::string q = "SELECT * FROM players WHERE login='" + login + "' AND pwd='" + pwd + "'";

  if (db.query(q) != 0) {
    return false;
  }
  return db.getRowCount() == 1;
}
