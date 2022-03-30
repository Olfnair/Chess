#ifndef WIN32WINDOW_H
#define WIN32WINDOW_H

#include <windows.h>
#include <string>
#include <vector>

class Win32Window
{
 public:
  Win32Window(const HINSTANCE hInst, const std::string title, const std::string wndClass, const int width, const int height, const DWORD style, bool msgOnly = false);
  // Win32Window(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
  virtual ~Win32Window();
  const HWND getHandle() const;
  const HINSTANCE getHinstance() const;
  bool resize(const int width, const int height);
  void show(bool s) const;
  virtual int mainLoop();
  virtual LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  static Win32Window* findWindow(HWND hWnd);
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

 private:
  void init(DWORD dwExStyle, const std::string&, const std::string&, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

  static std::vector<Win32Window*> winList;  // employé par le callback pour identifier la fenêtre qui reçoit les évènements
  HINSTANCE hInst;  // instance actuelle
  std::u16string title;  // Le texte de la barre de titre
  std::u16string wndClass;  // le nom de la classe de fenêtre
  HWND hWnd;  // handle de la fenêtre
  int width;  // largeur fenêtre
  int height;  // hauteur fenêtre
  DWORD style;  // styles à appliquer à la fenêtre
};

ATOM MyRegisterClass(const HINSTANCE, const std::u16string&, LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM));

#endif  // WIN32WINDOW_H
