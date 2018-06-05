// main.cpp : définit le point d'entrée pour l'application.
//

#include "Client.h"
#include "gui.h"

#include <windows.h>

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  AddFontResourceEx(L"data/font/TimelessBold.ttf", FR_PRIVATE, NULL); // ajout d'un font le temps de l'execution du processus

  ClientMain clientMain(hInstance, "Class_CHESS");
  return clientMain.mainLoop();
}
