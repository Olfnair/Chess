#ifndef EVENT_H
#define EVENT_H

#include <vector>
#include <windows.h>

class EventManager;

class EventHandler {
public:
  EventHandler(EventManager* manager, bool enabled = true);
  virtual ~EventHandler() = default;
  bool enable();
  bool disable();
  virtual LRESULT handle(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) const = 0; // renvoie > 0 si le message a été traité, < 0 si le message ne doit plus être traité, 0 si le message n'a pas été traité

private:
  EventManager* manager;
};

class EventManager {
public:
  EventManager() = default;
  ~EventManager() = default;
  bool addHandler(EventHandler* handler);
  bool remHandler(EventHandler* handler);
  LRESULT handle(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
  std::vector<EventHandler*> handlers;
};

#endif // EVENT_H
