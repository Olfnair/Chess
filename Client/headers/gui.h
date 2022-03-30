#ifndef GUI_H
#define GUI_H

#include "Chess.h"
#include "Matrix.h"
#include "Win32Window.h"
#include "status.h"

#include <objidl.h>
#include <windows.h>
#include <windowsx.h>
#include <winerror.h>
#include <gdiplus.h>
#include <string>
#include <vector>

RECT createRC(LONG left, LONG top, LONG right, LONG bottom);
bool isInsideRect(int x, int y, RECT rc);

class GdiplusInit final
{  // juste un gros hack pour initialiser gdi+ avant les autres variables statiques (ça permet d'avoir des Images préchargées comme membres statiques dans les classes)
 private:
  GdiplusInit() = default;  // Pas instanciable
  static class Init
  {  // En java on emploierait le bloc static
   public:
    Init();
    ~Init();

   private:
    ULONG_PTR gdiplusToken;
  } init;
};

class GuiComponent
{
 public:
  GuiComponent(bool enabled = true);
  GuiComponent(const Win32Window* window, RECT rcLoc = createRC(0, 0, 0, 0), bool enabled = true);
  GuiComponent(const GuiComponent* parent, RECT rcLoc = createRC(0, 0, 0, 0), bool enabled = true);
  virtual ~GuiComponent() = default;
  bool attachTo(const Win32Window* window, bool enabled = true, int x = 0, int y = 0, int dx = 0, int dy = 0);
  bool attachTo(const GuiComponent* parent, bool enabled = true, int x = 0, int y = 0, int dx = 0, int dy = 0);
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) = 0;
  virtual void enable();
  virtual void disable();
  bool isEnabled() const;
  virtual void setLocation(RECT rcLoc);
  RECT getLocation() const;  // coordonnées relatives au parent
  RECT getClientLoc() const;  // coordonnées réelles dans la client area
  void resize(int x, int y);
  void setWnd(Win32Window* window);
  void setParent(GuiComponent* parent);
  const Win32Window* getWnd() const;
  const GuiComponent* getParent() const;
  virtual void invalidate();
  virtual bool pointerIsInside(LPARAM lParam);
  void setMessWndHandle(HWND hwnd);
  HWND getMessWndHandle();

 protected:
  static UniquidGenerator uniqueWM;
  static UniquidGenerator uniqueButtonID;
  static UniquidGenerator uniqueTimerID;

 private:
  bool enabled;
  RECT rcLoc;  // indique position et dimensions du composant : la position est relative à celle du composant parent, ou à la fenêtre si pas de parent
  const Win32Window* window;
  const GuiComponent* parent;
  HWND hMessWnd;
};

class GuiPanel : public GuiComponent
{
 public:
  GuiPanel(bool enabled = true);
  GuiPanel(const Win32Window* window, RECT rcLoc, bool enabled = true);
  GuiPanel(const GuiComponent* parent, RECT rcLoc, bool enabled = true);
  virtual ~GuiPanel();
  void setBackgroundColor(COLORREF color);
  bool setBackgroundTexture(const std::string& u8filename);
  void setFont(HFONT hfont);
  HFONT getFont() const;
  void setTextColor(COLORREF textcolor);
  COLORREF getTextColor() const;
  void drawBegin(HDC hdc);
  void drawEnd(HDC hdc);
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  void drawCenterText(HDC hdc, const std::u16string& u16text, RECT rcDest);
  void drawCenterText(HDC hdc, const std::string& u8text, RECT rcDest);
  void drawText(HDC hdc, const std::u16string& u16text, unsigned int x, unsigned int y);
  void drawText(HDC hdc, const std::string& u8text, unsigned int x, unsigned int y);

 private:
  void init();

  HBRUSH hbrush;
  HGDIOBJ hOldBrush;
  HFONT hfont;
  HGDIOBJ hOldFont;
  COLORREF textcolor;
  COLORREF oldTextcolor;
};

class GuiButton : public GuiPanel
{
 public:
  GuiButton(bool enabled = true);
  GuiButton(const Win32Window* window, RECT rcLoc, unsigned int buttonID = 0, bool enabled = true);
  GuiButton(const GuiComponent* parent, RECT rcLoc, unsigned int buttonID = 0, bool enabled = true);
  virtual ~GuiButton() = default;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
  void setState(char state);
  char getState();
  void setText(const std::string& text);
  void setID(unsigned int id);
  unsigned int getID();

 private:
  char state;
  unsigned int buttonID;
  std::u16string text;  // UTF 16 (wchar, car l'API WIN32 emploie des WCHAR en UTF 16 pour le texte)

  static const char STATE_BUTTON = 0;
  static const char STATE_CLICKED = 1;
  static const char STATE_HOVER = 2;
  static Gdiplus::Image imButton;
  static Gdiplus::Image imButtonClicked;
  static Gdiplus::Image imButtonHover;
};

class GuiPiece : public GuiComponent
{
 public:
  GuiPiece(const WCHAR* path, bool enabled = true, Win32Window* window = nullptr);
  virtual ~GuiPiece() = default;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  bool drawAt(int x, int y, HDC hdc, Gdiplus::Graphics* graphics = nullptr);
  Gdiplus::Image* getImage();

 private:
  Gdiplus::Image image;
};

class GuiSelectedPiece : public GuiComponent
{
 public:
  GuiSelectedPiece(bool enabled = false);
  GuiSelectedPiece(Win32Window* window, bool enabled = false);
  GuiSelectedPiece(GuiComponent* parent, bool enabled = false);
  virtual ~GuiSelectedPiece() = default;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics) override;
  void setPiece(GuiPiece* piece);
  GuiPiece* getPiece();
  Chess::Pos getFrom();
  void setFrom(int i);

 private:
  GuiPiece* piece;
  Chess::Pos from;
};

class GuiChessBoard : public GuiComponent
{
 public:
  GuiChessBoard(bool enabled = false);
  GuiChessBoard(Win32Window* window, RECT rcLoc, bool enabled = false);
  GuiChessBoard(GuiComponent* parent, RECT rcLoc, bool enabled = false);
  virtual ~GuiChessBoard() = default;
  virtual void enable() override;
  virtual void disable() override;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  void reverse(bool r);
  void set(int x, int y, GuiPiece* gpiece);
  void set(int i, GuiPiece* gpiece);
  void set(int i, const Chess::Piece* piece);
  GuiPiece* get(int x, int y);
  void enableSelection();
  void disableSelection();
  void cancelSelection();

 private:
  static const int BSIZE = 8;  // taille de la board
  static const int DECAL = 56;  // taille en pixels du rebord (il faut décaler quand on draw les pieces)
  static const int PSIZE = 50;  // largeur/hauteur en pixel d'une piece

  static GuiPiece whitePawn;
  static GuiPiece whiteRook;
  static GuiPiece whiteKnight;
  static GuiPiece whiteBishop;
  static GuiPiece whiteQueen;
  static GuiPiece whiteKing;
  static GuiPiece blackPawn;
  static GuiPiece blackRook;
  static GuiPiece blackKnight;
  static GuiPiece blackBishop;
  static GuiPiece blackQueen;
  static GuiPiece blackKing;
  static Gdiplus::Image board;

  Matrix<GuiPiece*> mboard;
  GuiSelectedPiece spiece;
  bool reversed;

  bool selectAt(int x, int y);
  int translateIndex(int i);
};

class GuiColorInfo : public GuiPanel
{
 public:
  GuiColorInfo(bool enabled = true);
  GuiColorInfo(Win32Window* window, bool enabled = true);
  GuiColorInfo(GuiComponent* parent, bool enabled = true);
  virtual ~GuiColorInfo() = default;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  bool setColor(Chess::Color color);
  Chess::Color getColor() const;
  void setPlayerName(std::string playerName);
  const std::string& getPlayerName() const;
  void setTimer(unsigned int timer);
  unsigned int getTimer() const;
  void setSide2Move(bool val);
  bool isSideToMove();

 private:
  unsigned int timer;
  Chess::Color color;
  std::string playerName;
  bool side2move;
};

class GuiWaitPanel : public GuiPanel
{
 public:
  GuiWaitPanel(bool enabled = false);
  GuiWaitPanel(Win32Window* window, bool enabled = false);
  GuiWaitPanel(GuiComponent* parent, bool enabled = false);
  virtual ~GuiWaitPanel() = default;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
  void setFont(HFONT hfont);
  void setMessWndHandle(HWND hwnd);

 private:
  void init();

  GuiButton backButton;
  unsigned int backID;
};

class GuiGamePanel : public GuiPanel
{
 public:
  GuiGamePanel(bool enabled = false);
  GuiGamePanel(Win32Window* window, bool enabled = false);
  GuiGamePanel(GuiComponent* parent, bool enabled = false);
  virtual ~GuiGamePanel() = default;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
  void setSide2Move(Chess::Color color);
  void setPlayerSide(Chess::Color c);
  void reverse(bool r);
  void setBlackPlayerName(std::string name);
  void setWhitePlayerName(std::string name);
  void setPiece(int i, Chess::Piece* piece);
  void enableSelection();
  void disableSelection();
  void reset();
  void setFont(HFONT hfont);
  void setMessWndHandle(HWND hwnd);

 private:
  void init();
  void resetBoard();

  GuiChessBoard board;
  GuiColorInfo infoWhite;
  GuiColorInfo infoBlack;
  GuiButton backButton;

  Chess::Color playerSide;
  bool reversed;

  unsigned int backID;
};

class GuiMenu : public GuiPanel
{
 public:
  GuiMenu(bool enabled = false);
  GuiMenu(const Win32Window* window, RECT rcLoc, bool enabled = true);
  GuiMenu(const GuiComponent* parent, RECT rcLoc, bool enabled = true);
  virtual ~GuiMenu() = default;
  bool addButton(std::string text, unsigned int buttonID);
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
  void setFont(HFONT hfont);
  void setMessWndHandle(HWND hwnd);

 private:
  std::vector<GuiButton> buttons;

  static const int XMARGIN = 100;
  static const int YMARGIN = 50;
  static const int HEIGHT = 80;
};

class GuiMainMenu : public GuiMenu
{
 public:
  GuiMainMenu(bool enabled = false);
  GuiMainMenu(const Win32Window* window, RECT rcLoc, bool enabled = true);
  GuiMainMenu(const GuiComponent* parent, RECT rcLoc, bool enabled = true);
  virtual ~GuiMainMenu() = default;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

 private:
  void init();

  unsigned int soloID;
  unsigned int multiID;
  unsigned int exitID;
};

class GuiMainPanel : public GuiPanel
{
 public:
  GuiMainPanel() = default;
  GuiMainPanel(Win32Window* window, RECT rcLoc, bool enabled = false);
  GuiMainPanel(GuiComponent* parent, RECT rcLoc, bool enabled = false);
  virtual ~GuiMainPanel() = default;
  bool attachTo(const Win32Window* window, bool enabled = true, int x = 0, int y = 0, int dx = 0, int dy = 0);
  bool attachTo(const GuiComponent* parent, bool enabled = true, int x = 0, int y = 0, int dx = 0, int dy = 0);
  virtual void enable() override;
  virtual void disable() override;
  virtual bool draw(HDC hdc, Gdiplus::Graphics* graphics = nullptr) override;
  virtual LRESULT eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
  int getLogin(char* str, unsigned int size);
  int getPwd(char* str, unsigned int size);
  virtual void setLocation(RECT rcLoc) override;
  void setCS(ConnectionStatus::ConnectionStatus cs);
  void setFont(HFONT hfont);

 private:
  void init();

  HWND hLogin;
  HWND hPwd;
  GuiButton connectButton;
  unsigned int connectID;
  ConnectionStatus::ConnectionStatus cs;
  std::string strCS;
};

class ChessWin : public Win32Window
{
 public:
  ChessWin(const HINSTANCE hInst, const std::string title, const std::string wndClass, const int width, const int height, const DWORD style);
  virtual ~ChessWin();
  LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  HFONT getFont();
  bool setBackgroundTexture(const std::string& u8filename);
  void setMessWndHandle(HWND hwnd);
  void showGamePanel();
  void showMainMenu();
  void showWaitPanel();
  void gameInvalidate();
  void gameSetPlayerSide(Chess::Color c);
  void gameSetPiece(int i, Chess::Piece* piece);
  void gameSetWhitePlayerName(std::string name);
  void gameSetBlackPlayerName(std::string name);
  void gameSetSide2Move(const Chess::Color color);
  void gameAllowSelection();
  void gameDisableSelection();
  void setCS(ConnectionStatus::ConnectionStatus cs);
  int getLogin(char* str, int size);
  int getPassword(char* str, int size);
  void gamePanelSetPos(bool multi);

 private:
  GuiMainPanel mainPanel;
  GuiWaitPanel waitPanel;
  GuiGamePanel gamePanel;
  GuiMainMenu mainMenu;
  HFONT hfont;
  HBRUSH hbrush;
  HWND hMessWnd;

  Chess::Game game;
};

#endif  // GUI_H
