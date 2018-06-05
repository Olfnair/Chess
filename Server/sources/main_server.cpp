// main.cpp : définit le point d'entrée pour l'application.
//

#include "Server.h"

#include <windows.h>

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  AddFontResourceEx(L"data/font/tnm.ttf", FR_PRIVATE, nullptr); // ajout d'un font le temps de l'execution du processus

  ServerMain serverMain(hInstance, "Class_CHESS_SERVER");
  return serverMain.mainLoop();
}
