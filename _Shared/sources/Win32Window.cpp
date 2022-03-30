#include "tools.h"

#include "Win32Window.h"

#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

std::vector<Win32Window*> Win32Window::winList;

Win32Window::Win32Window(const HINSTANCE hInst, const std::string title, const std::string wndClass, const int width, const int height, const DWORD style, bool msgOnly) {
  if (msgOnly)
    init(0, wndClass, title, style, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInst, nullptr);
  else
    init(0, wndClass, title, style, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInst, nullptr);
}

// Win32Window::Win32Window(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
// 	init(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
// }

Win32Window::~Win32Window() {
  auto windowIt = std::find(winList.begin(), winList.end(), this);
  if(windowIt != winList.end()) {
    winList.erase(windowIt);
  }
}

const HWND Win32Window::getHandle() const {
  return hWnd;
}

const HINSTANCE Win32Window::getHinstance() const {
  return hInst;
}

bool Win32Window::resize(const int width, const int height) {
  return MoveWindow(hWnd, 0, 0, width, height, true) != FALSE;
  // MoveWindow() return un BOOL, qui est en fait un entier, donc on fait " != FALSE "
  //   pour avoir une expression logique (bool), ça évite de faire crier le compilo...
  // Il est un peu sensible...
  // (Soit disant, ça nuirait aux performances si on ne le fait pas...
  //   C'est vrai que je comptais appeler cette méthode 100 fois par seconde !)
}

void Win32Window::show(bool s) const {
  ShowWindow(hWnd, s);
  UpdateWindow(hWnd);
}

int Win32Window::mainLoop() {
  MSG msg;
  ZeroMemory(&msg, sizeof(msg));

  // Boucle de messages principale :
  while (GetMessage(&msg, NULL, 0U, 0U)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

LRESULT Win32Window::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_SIZE:
    if (wParam == SIZE_RESTORED) {
      width = LOWORD(lParam);
      height = HIWORD(lParam);
    }
    break;
  case WM_COMMAND:
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    break;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

Win32Window* Win32Window::findWindow(HWND hWnd) {
  auto windowIt = std::find_if(winList.begin(), winList.end(), [hWnd](const Win32Window* window) {
    return window->getHandle() == hWnd;
  });
  return windowIt != winList.end() ? *windowIt : nullptr;
}

LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  Win32Window* win;

  if (!(win = findWindow(hWnd))) // cherche le callback de la fenêtre
    return DefWindowProc(hWnd, message, wParam, lParam);

  return win->wndProc(hWnd, message, wParam, lParam);
}

// private
void Win32Window::init(DWORD dwExStyle, const std::string& classname, const std::string& windowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
  WNDCLASSEX wcex;
  this->hInst = hInstance;
  this->title = UTF8_TO_UTF16.from_bytes(windowName);
  this->wndClass = UTF8_TO_UTF16.from_bytes(classname);
  this->width = nWidth;
  this->height = nHeight;
  this->style = dwStyle;

  // TODO : générer des exceptions en cas d'erreur
  if (!GetClassInfoEx(hInst, (LPCWSTR)this->wndClass.data(), &wcex)) // si on a pas déjà enregistré une classe de fenêtre de ce nom
    MyRegisterClass(hInst, this->wndClass, Win32Window::WndProc);    // on l'enregistre
  winList.push_back(this);                                           // très important d'ajouter la fenêtre dans la liste avant de la créer (sinon le système envoie des messages alors que la liste est vide)
  // A noter this->wndProc() ne recevra pas le message WM_CREATE généré pendant CreateWindow() comme hWnd n'aura pas encore la bonne valeur et qu'on ne trouvera donc pas la fenêtre sur base de son HWND dans winList.

  hWnd = CreateWindowEx(dwExStyle, (LPCWSTR)this->wndClass.data(), (LPCWSTR)this->title.data(), dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

//
//  FONCTION : MyRegisterClass()
//
//  BUT : inscrit la classe de fenêtre.
//
//  COMMENTAIRES :
//
//    Cette fonction et son utilisation sont nécessaires uniquement si vous souhaitez que ce code
//    soit compatible avec les systèmes Win32 avant la fonction 'RegisterClassEx'
//    qui a été ajoutée à Windows 95. Il est important d'appeler cette fonction
//    afin que l'application dispose des petites icônes correctes qui lui sont
//    associées.
//
ATOM MyRegisterClass(const HINSTANCE hInstance, const std::u16string& wndClass, LRESULT(CALLBACK* WndProc)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)) {
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101)); // 101 hardcodé : c'est la valeur de IDI_CHESS défini dans Resource.h
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = (LPCWSTR)wndClass.data();
  wcex.hIconSm = 0;
  return RegisterClassEx(&wcex);
}
