#include "Event.h"

#include <algorithm>
#include <windows.h>

// EventHandler :
EventHandler::EventHandler(EventManager* manager, bool enabled) : manager(manager) {
  if (enabled)
    enable();
}

bool EventHandler::enable() {
  if (!manager)
    return false;
  return manager->addHandler(this);
}

bool EventHandler::disable() {
  if (!manager)
    return false;
  return manager->remHandler(this);
}

// EventManager :
bool EventManager::addHandler(EventHandler* handler) {
  handlers.push_back(handler);
  return true;
}

bool EventManager::remHandler(EventHandler* handler) {
  auto it = std::remove(handlers.begin(), handlers.end(), handler); // déplace les éléments à supprimer après handlers.end(), it étant le premier
  if (it == handlers.end()) {                                       // handler n'a pas été trouvé : il n'y à rien à supprimer
    return false;
  }
  handlers.erase(it, handlers.end());
  return true;
}

LRESULT EventManager::handle(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT g_ret = 0;
  LRESULT ret = 1;
  for (auto handler : handlers) {
    ret = handler->handle(hWnd, message, wParam, lParam);
    if (g_ret == 0 && ret != 0)
      g_ret = ret;
  }
  return g_ret;
}
