#include "tools.h"

#include <algorithm>
#include <cstring>
#include <string>

#include <ctime>
#include <sys/timeb.h>

UniquidGenerator uniqueExceptionID(1);
UniquidGenerator uniqueTimerID(1000);

const int FILE_EXC = uniqueExceptionID.generate();

std::wstring_convert<codecvt<char16_t, char, std::mbstate_t>, char16_t> UTF8_TO_UTF16;

UniquidGenerator::UniquidGenerator(unsigned int startVal) : uniquid(startVal) {
}

unsigned int UniquidGenerator::generate() {
  return uniquid++;
}

// du vieux code que j'ai récupéré d'un de mes anciens projets (très vieux) : les noms sont pas très sexys et le code peut-être pas non plus, mais ça fait le job.
// (j'ai quand même refait l'indentation)
std::string basex_to_basey(const char* number, const unsigned int BASE_X, const unsigned int BASE_Y) {
  unsigned int number_base10 = char_to_uint(number, BASE_X);
  std::string nbr_str = uint_to_str(number_base10, BASE_Y);
  return nbr_str;
}

unsigned int char_to_uint(const char* number, const unsigned int BASE) {
  unsigned int exponential = 1; // 1 car BASE^0 = 1
  unsigned int number_converted = 0;

  if (BASE > strlen(FIGURES)) {
    return 0;
  }
  for (int i = (strlen(number) - 1); i >= 0; --i) {
    for (unsigned int i2 = 0; i2 < BASE; ++i2) {
      if (number[i] == FIGURES[i2]) {
        number_converted += i2 * exponential;
        exponential *= BASE;
        break;
      }
    }
  }
  return number_converted;
}

std::string uint_to_str(unsigned int number, const unsigned int BASE) { // itoa() n'est pas standard
  std::string nbr_str = "\0";

  if (BASE > strlen(FIGURES)) {
    return nbr_str;
  } else if (number == 0) {
    return "0";
  }
  while (number > 0) {
    for (unsigned int i = 0; i < BASE; ++i) {
      if (i == (number % BASE)) {
        nbr_str += FIGURES[i];
        break;
      }
    }
    number /= BASE;
  }
  std::reverse(nbr_str.begin(), nbr_str.end());
  return nbr_str;
}

// équivalent de la même fonction Unix
int gettimeofday(timeval* tp, void* tz) {
  struct _timeb timebuffer;
  _ftime(&timebuffer);
  tp->tv_sec = (long)timebuffer.time;
  tp->tv_usec = (long)timebuffer.millitm * 1000;
  return 0;
}

/* transforme un temps en millisec en un timeval */
void millisec2timeval(int millisec, timeval* x) {
  x->tv_sec = millisec / 1000;
  x->tv_usec = (millisec % 1000) * 1000;
}

/* renvoie la différence entre 2 timeval en millisec */
/* positif si a < b */
long timevalDiff(const timeval& a, const timeval& b) {
  long sec = b.tv_sec - a.tv_sec;
  long millisec = b.tv_usec - a.tv_usec;
  return sec * 1000 + millisec / 1000;
}

// File
File::File(const char* filename) {
  closed = true;
  f.open(filename, std::ios::in /*| std::ios::binary*/);
  if (!f)
    throw FILE_EXC;
  closed = false;
}
File::~File() {
  if (!closed)
    close();
}
void File::close() {
  f.close();
  closed = true;
}
std::string File::readline() {
  std::string res = "";
  std::getline(f, res);
  return res;
}
// fin File

// BufferA
Buffer::Buffer(unsigned int size) : size(size), length(0) {
  data = new char[size]; // pour très bien faire, faudrait checker les erreurs d'allocation
}
Buffer::~Buffer() {
  delete[] data;
}
Buffer::Buffer(const Buffer& b) : size(b.size), length(b.length) {
  data = new char[size];
  write(0, b.data, size);
}
Buffer& Buffer::operator=(const Buffer& b) {
  delete[] data;
  this->size = b.size;
  this->length = b.length;
  data = new char[size];
  write(0, b.data, size);
  return *this;
}
void Buffer::write(unsigned int i, const void* data, unsigned int len) {
  std::memcpy(this->data + i, data, len + i <= size - i ? len : size - i);
}
void Buffer::read(unsigned int i, void* data, unsigned int len) const {
  std::memcpy(data, this->data + i, len + i <= size - i ? len : size - i);
}
unsigned int Buffer::getSize() const {
  return size;
}
unsigned int Buffer::getLen() const {
  return length;
}
void Buffer::setLen(unsigned int len) {
  length = len;
}
char* Buffer::getData() {
  return data;
}
const char* Buffer::getData() const {
  return data;
}
void Buffer::print(std::string& str) {
  str = "";
  std::string tmp = "";
  for (unsigned int i = 0; i < length; ++i) {
    tmp = uint_to_str(data[i], HEXADEC);
    if (tmp.size() < 2)
      tmp = "0" + tmp;
    str += tmp;
  }
  str += (char)0;
}
// fin Buffer
