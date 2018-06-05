#ifndef TOOLS_H
#define TOOLS_H

#include <Winsock2.h>
#include <codecvt>
#include <fstream>
#include <locale>
#include <string>

#define FIGURES "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define HEXADEC 16
#define DEC 10
#define OCTAL 8
#define BINARY 2

#define MAX_BASE (strlen(FIGURES))

class UniquidGenerator {
public:
  UniquidGenerator(unsigned int startVal);
  ~UniquidGenerator() = default;
  unsigned int generate();

private:
  unsigned int uniquid;
};

template <class I, class E, class S>
class codecvt : public std::codecvt<I, E, S> {
public:
  ~codecvt() = default;
};

extern UniquidGenerator uniqueExceptionID; // globale
extern UniquidGenerator uniqueTimerID;

extern const int FILE_EXC;

extern std::wstring_convert<codecvt<char16_t, char, std::mbstate_t>, char16_t> UTF8_TO_UTF16;

std::string basex_to_basey(const char* number, const unsigned int BASE_X, const unsigned int BASE_Y);
unsigned int char_to_uint(const char* number, const unsigned int BASE);
std::string uint_to_str(unsigned int number, const unsigned int BASE);

int gettimeofday(timeval* tp, void* tz);
void millisec2timeval(int millisec, timeval* x);
long timevalDiff(const timeval& a, const timeval& b);

class File { // juste pour manipuler rapidement des fichiers de config ... (avec une info par ligne, quick & dirty : sorry)
public:
  File(const char* filename);
  ~File();
  void close();
  std::string readline();

private:
  std::ifstream f;
  bool closed;
};

class Buffer {
public:
  Buffer(unsigned int size);
  ~Buffer();
  Buffer(const Buffer& b);
  Buffer& operator=(const Buffer& b);
  void write(unsigned int i, const void* data, unsigned int len);
  void read(unsigned int i, void* data, unsigned int len) const;
  unsigned int getSize() const;
  unsigned int getLen() const;
  void setLen(unsigned int len);
  char* getData();
  const char* getData() const;
  void print(std::string& str);

private:
  char* data;
  unsigned int size;
  unsigned int length;
};

#endif // TOOLS_H
