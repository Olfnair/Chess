#include "gui.h"

#include "ThreadConst.h"

RECT createRC(LONG left, LONG top, LONG right, LONG bottom) {
  RECT rc = {left, top, right, bottom};
  return rc;
}

bool isInsideRect(int x, int y, RECT rc) {
  return x >= rc.left && y >= rc.top && x < rc.left + rc.right && y < rc.top + rc.bottom;
}

// GdiplusInit
GdiplusInit::Init GdiplusInit::init; // initialisation de Gdiplus avant les autres variables statiques

GdiplusInit::Init::Init() {
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}
GdiplusInit::Init::~Init() {
  Gdiplus::GdiplusShutdown(gdiplusToken);
}
// fin GdiPlusInit

// GuiComponent
// MSDN : The message identifier. Applications can only use the low word; the high word is reserved by the system.
// MSDN : The WM_APP constant is used to distinguish between message values that are reserved for use by the system and values that can be used by an application to send messages within a private window class.
// MSDN : Used to define private messages, usually of the form WM_APP+x, where x is an integer value.
// la constante WM_APP (= 0x8000) Marque donc la délimitation
UniquidGenerator GuiComponent::uniqueWM(WM_APP + 1); // on commence donc à numéroter nos messages à WM_APP + 1
UniquidGenerator GuiComponent::uniqueButtonID(1000); // Identifiants des boutons
UniquidGenerator GuiComponent::uniqueTimerID(1000);

GuiComponent::GuiComponent(bool enabled) : enabled(enabled), window(nullptr), parent(nullptr), hMessWnd(nullptr) {
  rcLoc = createRC(0, 0, 0, 0);
}
GuiComponent::GuiComponent(const Win32Window* window, RECT rcLoc, bool enabled) : enabled(enabled), rcLoc(rcLoc), window(window), parent(nullptr), hMessWnd(nullptr) {
  if (isEnabled())
    enable();
}
GuiComponent::GuiComponent(const GuiComponent* parent, RECT rcLoc, bool enabled) : enabled(enabled), rcLoc(rcLoc), window(nullptr), parent(parent), hMessWnd(nullptr) {
  if (isEnabled())
    enable();
}
bool GuiComponent::attachTo(const Win32Window* window, bool enabled, int x, int y, int dx, int dy) {
  this->window = window;
  setLocation(createRC(x, y, dx, dy));
  if (enabled && !isEnabled())
    enable();
  return true;
}
bool GuiComponent::attachTo(const GuiComponent* parent, bool enabled, int x, int y, int dx, int dy) {
  if (!parent)
    return false;
  this->parent = parent;
  RECT rcParentLoc = parent->getClientLoc();
  return attachTo(parent->getWnd(), enabled, rcParentLoc.left + x, rcParentLoc.top + y, dx, dy);
}
LRESULT GuiComponent::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  return 0;
}
void GuiComponent::enable() {
  invalidate();
  enabled = true;
}
void GuiComponent::disable() {
  invalidate();
  enabled = false;
}
bool GuiComponent::isEnabled() const {
  return enabled;
}
void GuiComponent::setLocation(RECT rcLoc) {
  this->rcLoc = rcLoc;
}
RECT GuiComponent::getLocation() const { // coordonnées relatives au parent
  return rcLoc;
}
RECT GuiComponent::getClientLoc() const { // coordonnées réelles dans la client area
  RECT rcClientLoc = getLocation();
  for (const GuiComponent* parent = this->parent; parent; parent = parent->getParent()) {
    RECT rcParentLoc = parent->getLocation();
    rcClientLoc.left += rcParentLoc.left;
    rcClientLoc.top += rcParentLoc.top;
  }
  return rcClientLoc;
}
void GuiComponent::resize(int x, int y) {
  setLocation(createRC(getLocation().left, getLocation().top, x, y));
}
void GuiComponent::setWnd(Win32Window* window) {
  this->window = window;
}
void GuiComponent::setParent(GuiComponent* parent) {
  this->parent = parent;
}
const Win32Window* GuiComponent::getWnd() const {
  if (!window && parent)
    return parent->getWnd();
  return window;
}
const GuiComponent* GuiComponent::getParent() const {
  return parent;
}
void GuiComponent::invalidate() {
  if (!getWnd())
    return;
  RECT rc = getClientLoc();
  rc = createRC(rc.left, rc.top, rc.left + rc.right, rc.top + rc.bottom);
  InvalidateRect(getWnd()->getHandle(), &rc, false);
}
bool GuiComponent::pointerIsInside(LPARAM lParam) {
  return isInsideRect(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), getClientLoc());
}
void GuiComponent::setMessWndHandle(HWND hwnd) {
  hMessWnd = hwnd;
}
HWND GuiComponent::getMessWndHandle() {
  return hMessWnd;
}
// fin GuiComponent

// GuiPanel
GuiPanel::GuiPanel(bool enabled) : GuiComponent(enabled) {
  init();
}
GuiPanel::GuiPanel(const Win32Window* window, RECT rcLoc, bool enabled) : GuiComponent(window, rcLoc, enabled) {
  init();
}
GuiPanel::GuiPanel(const GuiComponent* parent, RECT rcLoc, bool enabled) : GuiComponent(parent, rcLoc, enabled) {
  init();
}
GuiPanel::~GuiPanel() {
  DeleteObject(hbrush);
}
void GuiPanel::setBackgroundColor(COLORREF color) {
  DeleteObject(hbrush);
  hbrush = CreateSolidBrush(color);
}
bool GuiPanel::setBackgroundTexture(const std::string& u8filename) {
  HBITMAP hbitmap = (HBITMAP)LoadImage(nullptr, (LPCWSTR)UTF8_TO_UTF16.from_bytes(u8filename).data(), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
  if (hbitmap == 0)
    return false;
  DeleteObject(hbrush);
  hbrush = CreatePatternBrush(hbitmap);
  DeleteObject(hbitmap);
  return true;
}
void GuiPanel::setFont(HFONT hfont) {
  this->hfont = hfont;
  invalidate();
}
HFONT GuiPanel::getFont() const {
  return hfont;
}
void GuiPanel::setTextColor(COLORREF textcolor) {
  this->textcolor = textcolor;
}
COLORREF GuiPanel::getTextColor() const {
  return textcolor;
}
void GuiPanel::drawBegin(HDC hdc) {
  if (hfont)
    hOldFont = SelectObject(hdc, hfont);
  oldTextcolor = GetTextColor(hdc);
  SetTextColor(hdc, textcolor);
  SetBkMode(hdc, TRANSPARENT);
  hOldBrush = SelectObject(hdc, hbrush);
}
void GuiPanel::drawEnd(HDC hdc) {
  if (hfont)
    SelectObject(hdc, hOldFont);
  SetTextColor(hdc, oldTextcolor);
  SelectObject(hdc, hOldBrush);
}
bool GuiPanel::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  RECT rc = getClientLoc();
  rc.right += rc.left;
  rc.bottom += rc.top;
  RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 50, 50);
  return true;
}
void GuiPanel::drawCenterText(HDC hdc, const std::u16string& u16text, RECT rcDest) {
  SetTextColor(hdc, textcolor);
  ::DrawText(hdc, (LPCWSTR)u16text.data(), -1, &rcDest, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}
void GuiPanel::drawCenterText(HDC hdc, const std::string& u8text, RECT rcDest) {
  drawCenterText(hdc, UTF8_TO_UTF16.from_bytes(u8text), rcDest);
}
void GuiPanel::drawText(HDC hdc, const std::u16string& u16text, unsigned int x, unsigned int y) {
  SetTextColor(hdc, textcolor);
  LPCWSTR wstr = (LPCWSTR)u16text.data();
  TextOut(hdc, x, y, wstr, wcslen(wstr));
}
void GuiPanel::drawText(HDC hdc, const std::string& u8text, unsigned int x, unsigned int y) {
  drawText(hdc, UTF8_TO_UTF16.from_bytes(u8text), x, y);
}

// private:
void GuiPanel::init() {
  hbrush = CreateSolidBrush(RGB(0, 0, 0));
  textcolor = RGB(0, 0, 0);
  hfont = NULL;
  hOldFont = NULL;
}
// fin GuiPanel

// GuiButton
Gdiplus::Image GuiButton::imButton(L"data/button/button.png");
Gdiplus::Image GuiButton::imButtonClicked(L"data/button/button_clicked2.png");
Gdiplus::Image GuiButton::imButtonHover(L"data/button/button_hover.png");

GuiButton::GuiButton(bool enabled) : GuiPanel(enabled), state(STATE_BUTTON), buttonID(0) {
}
GuiButton::GuiButton(const Win32Window* window, RECT rcLoc, unsigned int buttonID, bool enabled) : GuiPanel(window, rcLoc, enabled), state(STATE_BUTTON), buttonID(buttonID) {
}
GuiButton::GuiButton(const GuiComponent* parent, RECT rcLoc, unsigned int buttonID, bool enabled) : GuiPanel(parent, rcLoc, enabled), state(STATE_BUTTON), buttonID(buttonID) {
}
bool GuiButton::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  drawBegin(hdc);
  RECT rcClientLoc = getClientLoc();
  if (getState() == STATE_CLICKED)
    graphics->DrawImage(&imButtonClicked, Gdiplus::Rect(rcClientLoc.left, rcClientLoc.top, rcClientLoc.right, rcClientLoc.bottom));
  else if (getState() == STATE_HOVER)
    graphics->DrawImage(&imButtonHover, Gdiplus::Rect(rcClientLoc.left, rcClientLoc.top, rcClientLoc.right, rcClientLoc.bottom));
  else
    graphics->DrawImage(&imButton, Gdiplus::Rect(rcClientLoc.left, rcClientLoc.top, rcClientLoc.right, rcClientLoc.bottom));
  if (getState() != STATE_CLICKED)
    setTextColor(RGB(255, 255, 255));
  else
    setTextColor(RGB(200, 200, 200));
  drawCenterText(hdc, text, createRC(rcClientLoc.left, rcClientLoc.top, rcClientLoc.left + rcClientLoc.right, rcClientLoc.top + rcClientLoc.bottom));
  drawEnd(hdc);
  return true;
}
LRESULT GuiButton::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!isEnabled())
    return -1;                                                                                 // on n'est pas en cours d'utilisation, on n'est donc pas censé recevoir d'événement
  else if (message == WM_MOUSEMOVE && pointerIsInside(lParam) && getState() == STATE_BUTTON) { // on survole le bouton et il n'est pas clické ou déjà survolé
    setState(STATE_HOVER);
  } else if (message == WM_MOUSEMOVE && !pointerIsInside(lParam) && getState() != STATE_BUTTON) { // le curseur a quitté le boutton alors qu'il était clické ou survolé
    setState(STATE_BUTTON);
  } else if (message == WM_LBUTTONDOWN && pointerIsInside(lParam) && getState() != STATE_CLICKED) { // on vient de cliquer sur le bouton
    setState(STATE_CLICKED);
  } else if (message == WM_LBUTTONUP && pointerIsInside(lParam) && getState() == STATE_CLICKED) { // le bouton était enfoncé et on a relaché le bouton de la souris en étant dessus (on vient de l'activer)
    setState(STATE_HOVER);
    // envoyer le message de commande (on vient de cliquer sur le bouton)
    if (buttonID != 0)
      PostMessage(getWnd()->getHandle(), WM_COMMAND, (WPARAM)buttonID, 0);
  }
  return 0;
}
void GuiButton::setState(char state) {
  this->state = state;
  invalidate();
}
char GuiButton::getState() {
  return state;
}
void GuiButton::setText(const std::string& text) {
  this->text = UTF8_TO_UTF16.from_bytes(text);
  invalidate();
}
void GuiButton::setID(unsigned int id) {
  buttonID = id;
}
unsigned int GuiButton::getID() {
  return buttonID;
}
// fin GuiButton

// GuiPiece
GuiPiece::GuiPiece(const WCHAR* path, bool enabled, Win32Window* window) : GuiComponent(window, createRC(0, 0, 0, 0), enabled), image(path) {
  setLocation(createRC(0, 0, image.GetWidth(), image.GetHeight()));
}
bool GuiPiece::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  RECT rc = getClientLoc();
  return drawAt(rc.left, rc.top, hdc, graphics);
}
bool GuiPiece::drawAt(int x, int y, HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  RECT rc = getClientLoc();
  graphics->DrawImage(&image, Gdiplus::Rect(x, y, rc.right, rc.bottom));
  return true;
}
Gdiplus::Image* GuiPiece::getImage() {
  return &image;
}
// fin GuiPiece

// GuiSelectedPiece
GuiSelectedPiece::GuiSelectedPiece(bool enabled) : GuiComponent(enabled) {
}
GuiSelectedPiece::GuiSelectedPiece(Win32Window* window, bool enabled) : GuiComponent(window, createRC(0, 0, 0, 0), enabled) {
}
GuiSelectedPiece::GuiSelectedPiece(GuiComponent* parent, bool enabled) : GuiComponent(parent, createRC(0, 0, 0, 0), enabled) {
}
bool GuiSelectedPiece::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  RECT rc = getClientLoc();
  graphics->DrawImage(piece->getImage(), Gdiplus::Rect(rc.left, rc.top, rc.right, rc.bottom));
  return true;
}
void GuiSelectedPiece::setPiece(GuiPiece* piece) {
  this->piece = piece;
}
GuiPiece* GuiSelectedPiece::getPiece() {
  return piece;
}
Chess::Pos GuiSelectedPiece::getFrom() {
  return from;
}
void GuiSelectedPiece::setFrom(int i) {
  from.set(i);
}
// fin GuiSelectedPiece

// GuiChessBoard
// private (static):
GuiPiece GuiChessBoard::whitePawn(L"data/pawn_white.png");
GuiPiece GuiChessBoard::whiteRook(L"data/rook_white.png");
GuiPiece GuiChessBoard::whiteKnight(L"data/knight_white.png");
GuiPiece GuiChessBoard::whiteBishop(L"data/bishop_white.png");
GuiPiece GuiChessBoard::whiteQueen(L"data/queen_white.png");
GuiPiece GuiChessBoard::whiteKing(L"data/king_white.png");
GuiPiece GuiChessBoard::blackPawn(L"data/pawn_black.png");
GuiPiece GuiChessBoard::blackRook(L"data/rook_black.png");
GuiPiece GuiChessBoard::blackKnight(L"data/knight_black.png");
GuiPiece GuiChessBoard::blackBishop(L"data/bishop_black.png");
GuiPiece GuiChessBoard::blackQueen(L"data/queen_black.png");
GuiPiece GuiChessBoard::blackKing(L"data/king_black.png");
Gdiplus::Image GuiChessBoard::board(L"data/board2.jpg");

// private:
bool GuiChessBoard::selectAt(int x, int y) {
  int i = y / PSIZE * BSIZE + x / PSIZE;
  GuiPiece* gpiece = mboard(i);
  if (gpiece) {
    spiece.setFrom(i);
    spiece.setPiece(gpiece);
    spiece.setLocation(createRC(x - PSIZE / 2 + DECAL, y - PSIZE / 2 + DECAL, gpiece->getLocation().right, gpiece->getLocation().bottom));
    return true;
  }
  return false;
}

int GuiChessBoard::translateIndex(int i) {
  return reversed ? BSIZE * BSIZE - 1 - i : i;
}

// public:
GuiChessBoard::GuiChessBoard(bool enabled) : GuiComponent(enabled), mboard(BSIZE, BSIZE), spiece(false) {
  spiece.setParent(this);
}
GuiChessBoard::GuiChessBoard(Win32Window* window, RECT rcLoc, bool enabled) : GuiComponent(window, rcLoc, enabled), mboard(BSIZE, BSIZE), spiece(false) {
  spiece.setParent(this);
}
GuiChessBoard::GuiChessBoard(GuiComponent* parent, RECT rcLoc, bool enabled) : GuiComponent(parent, rcLoc, enabled), mboard(BSIZE, BSIZE), spiece(false) {
  spiece.setParent(this);
}
void GuiChessBoard::enable() { // reset aussi les pieces
  // blancs
  mboard(0, BSIZE - 1) = &whiteRook;
  mboard(1, BSIZE - 1) = &whiteKnight;
  mboard(2, BSIZE - 1) = &whiteBishop;
  mboard(3, BSIZE - 1) = &whiteQueen;
  mboard(4, BSIZE - 1) = &whiteKing;
  mboard(5, BSIZE - 1) = &whiteBishop;
  mboard(6, BSIZE - 1) = &whiteKnight;
  mboard(7, BSIZE - 1) = &whiteRook;
  for (int i = BSIZE * BSIZE - 2 * BSIZE; i < BSIZE * BSIZE - BSIZE; ++i) // toute la 6ème ligne
    mboard(i) = &whitePawn;

  // noirs
  mboard(0, 0) = &blackRook;
  mboard(1, 0) = &blackKnight;
  mboard(2, 0) = &blackBishop;
  mboard(3, 0) = &blackQueen;
  mboard(4, 0) = &blackKing;
  mboard(5, 0) = &blackBishop;
  mboard(6, 0) = &blackKnight;
  mboard(7, 0) = &blackRook;
  for (int i = BSIZE; i < 2 * BSIZE; ++i) // toute la 2ème ligne
    mboard(i) = &blackPawn;

  // vide
  for (int i = BSIZE * 2; i < BSIZE * BSIZE - BSIZE * 2; ++i) // Toutes les lignes sauf les 2 premières et les 2 dernières (soit les 4 du milieu)
    mboard(i) = nullptr;

  spiece.disable(); // aucune pièce n'est sélectionnée au départ
  reversed = false;

  GuiComponent::enable();
}
void GuiChessBoard::disable() {
  GuiComponent::disable();
}
LRESULT GuiChessBoard::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  RECT rcClientLoc = getClientLoc();
  if (!isEnabled())
    return -1; // on n'est pas en cours d'utilisation, on n'est donc pas censé recevoir d'évènement
  // sélection d'une pièce
  else if (message == WM_LBUTTONDOWN && !spiece.isEnabled() && isInsideRect(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), createRC(rcClientLoc.left + DECAL, rcClientLoc.top + DECAL, rcClientLoc.right, rcClientLoc.bottom))) {
    int x = GET_X_LPARAM(lParam) - (rcClientLoc.left + DECAL);
    int y = GET_Y_LPARAM(lParam) - (rcClientLoc.top + DECAL);
    if (selectAt(x, y)) // alors demander l'autorisation de sélectionner cette pièce (peut-être pas notre tour de jouer, pas une pièce à nous, etc) (PostMessage)
      PostMessage(getMessWndHandle(), ThreadMessage::SELECT, translateIndex(y / PSIZE * BSIZE + x / PSIZE), 0);
  }
  // poser une pièce sélectionnée
  else if (message == WM_LBUTTONDOWN && spiece.isEnabled() && isInsideRect(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), createRC(rcClientLoc.left + DECAL, rcClientLoc.top + DECAL, rcClientLoc.right, rcClientLoc.bottom))) {
    int x = GET_X_LPARAM(lParam) - (rcClientLoc.left + DECAL);
    int y = GET_Y_LPARAM(lParam) - (rcClientLoc.top + DECAL);
    // envoyer un message (PostMessage) pour demander si ce coup est legal. mainthread fera ce qu'il faut à la réception du message
    PostMessage(getMessWndHandle(), ThreadMessage::CHECKMOVE, translateIndex(spiece.getFrom()), translateIndex(y / PSIZE * BSIZE + x / PSIZE));
  } else if (message == WM_MOUSEMOVE && pointerIsInside(lParam) && spiece.isEnabled()) {
    RECT rcOld = spiece.getClientLoc();
    RECT rcNew;
    RECT rcOldNew;
    spiece.setLocation(createRC(GET_X_LPARAM(lParam) - rcClientLoc.left - PSIZE / 2, GET_Y_LPARAM(lParam) - rcClientLoc.top - PSIZE / 2, spiece.getLocation().right, spiece.getLocation().bottom));
    rcNew = spiece.getClientLoc();
    rcOld.right += rcOld.left;
    rcOld.bottom += rcOld.top;
    rcNew.right += rcNew.left;
    rcNew.bottom += rcNew.top;
    // on détermine un rectangle contenant l'ancienne et la nouvelle position de la pièce pour ne redessiner que ça mais en une fois.
    rcOldNew = createRC(rcOld.left < rcNew.left ? rcOld.left : rcNew.left, rcOld.top < rcNew.top ? rcOld.top : rcNew.top,
                        rcOld.right > rcNew.right ? rcOld.right : rcNew.right, rcOld.bottom > rcNew.bottom ? rcOld.bottom : rcNew.bottom);
    InvalidateRect(getWnd()->getHandle(), &rcOldNew, false);
  } else if (message == WM_MOUSEMOVE && !pointerIsInside(lParam) && spiece.isEnabled()) { // on est sorti de la board, on annule la sélection et remet la pièce à sa place
    cancelSelection();
  }
  return 0;
}
bool GuiChessBoard::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  GuiPiece* gpiece = nullptr;
  RECT rcClientLoc = getClientLoc();
  graphics->DrawImage(&board, Gdiplus::Rect(rcClientLoc.left, rcClientLoc.top, rcClientLoc.right, rcClientLoc.bottom));
  for (int i = 0; i < BSIZE * BSIZE; ++i)
    if ((gpiece = mboard(i)))
      gpiece->drawAt(rcClientLoc.left + DECAL + (i % BSIZE) * PSIZE, rcClientLoc.top + DECAL + (i / BSIZE) * PSIZE, hdc, graphics);
  if (spiece.isEnabled())
    spiece.draw(hdc, graphics);
  return true;
}
void GuiChessBoard::reverse(bool r) {
  if ((r && !reversed) || (!r && reversed)) { // alors on doit inverser les éléments dans la matrice (premier devient dernier)
    Matrix<GuiPiece*> copyBoard(mboard);
    for (int ci = 0, mi = BSIZE * BSIZE - 1; mi >= 0; ++ci, --mi)
      mboard(mi) = copyBoard(ci);
  }
  reversed = r;
}
void GuiChessBoard::set(int x, int y, GuiPiece* gpiece) {
  set(y * BSIZE + x, gpiece);
}
void GuiChessBoard::set(int i, GuiPiece* gpiece) {
  mboard(translateIndex(i)) = gpiece;
}
void GuiChessBoard::set(int i, const Chess::Piece* piece) {
  if (!piece)
    set(i, (GuiPiece*)nullptr);
  else if (piece->isPawn())
    set(i, (piece->getColor() == Chess::C_WHITE) ? &whitePawn : &blackPawn);
  else if (piece->isRook())
    set(i, (piece->getColor() == Chess::C_WHITE) ? &whiteRook : &blackRook);
  else if (piece->isKnight())
    set(i, (piece->getColor() == Chess::C_WHITE) ? &whiteKnight : &blackKnight);
  else if (piece->isBishop())
    set(i, (piece->getColor() == Chess::C_WHITE) ? &whiteBishop : &blackBishop);
  else if (piece->isQueen())
    set(i, (piece->getColor() == Chess::C_WHITE) ? &whiteQueen : &blackQueen);
  else if (piece->isKing())
    set(i, (piece->getColor() == Chess::C_WHITE) ? &whiteKing : &blackKing);
}
GuiPiece* GuiChessBoard::get(int x, int y) {
  return mboard(x, y);
}
void GuiChessBoard::enableSelection() { // on autorise une sélection (c'est à notre tour de jouer)
  mboard(spiece.getFrom()) = nullptr;   // on enlève la pièce sélectionnée des pièces de la board
  spiece.enable();
  invalidate(); // on redessine tout (on doit effacer la piece sélectionnée et la redessiner là ou elle a été selectionnée)
}
void GuiChessBoard::disableSelection() {
  spiece.disable();
  spiece.invalidate();
}
void GuiChessBoard::cancelSelection() {
  disableSelection();
  mboard(spiece.getFrom()) = spiece.getPiece(); // on remet la pièce sélectionnée à sa place sur la board
  invalidate();                                 // on redessine l'échiquier
}
// fin GuiChessBoard

// GuiColorInfo
GuiColorInfo::GuiColorInfo(bool enabled) : GuiPanel(enabled), timer(0), color(Chess::C_WHITE), side2move(false) {
}
GuiColorInfo::GuiColorInfo(Win32Window* window, bool enabled) : GuiPanel(window, createRC(0, 0, 0, 0), enabled), timer(0), color(Chess::C_WHITE), side2move(false) {
}
GuiColorInfo::GuiColorInfo(GuiComponent* parent, bool enabled) : GuiPanel(parent, createRC(0, 0, 0, 0), enabled), timer(0), color(Chess::C_WHITE), side2move(false) {
}
bool GuiColorInfo::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  RECT rcClient = getClientLoc();
  if (!isEnabled() || !graphics)
    return false;
  drawBegin(hdc);
  GuiPanel::draw(hdc, graphics);
  if (color == Chess::C_BLACK)
    setTextColor(RGB(255, 255, 255));
  else
    setTextColor(RGB(0, 0, 0));
  std::string u8str = ((color == Chess::C_WHITE) ? u8"Blancs : " : u8"Noirs : ") + playerName;
  std::u16string u16str = UTF8_TO_UTF16.from_bytes(u8str);
  drawText(hdc, u16str, rcClient.left + 20, rcClient.top + 10);
  // afficher timer
  if (side2move)
    drawText(hdc, "A le trait (c'est son tour)", rcClient.left + 20, rcClient.top + 30);
  drawEnd(hdc);
  return true;
}
bool GuiColorInfo::setColor(Chess::Color color) {
  bool ret = false;
  if (color == Chess::C_BLACK) {
    this->color = color;
    ret = setBackgroundTexture("data/textures/texture03.bmp");
  }

  else if (color == Chess::C_WHITE) {
    this->color = color;
    ret = setBackgroundTexture("data/textures/texture04.bmp");
  }
  return ret;
}
Chess::Color GuiColorInfo::getColor() const {
  return color;
}
void GuiColorInfo::setPlayerName(std::string playerName) {
  this->playerName = playerName;
}
const std::string& GuiColorInfo::getPlayerName() const {
  return playerName;
}
void GuiColorInfo::setTimer(unsigned int timer) {
  this->timer = timer;
}
unsigned int GuiColorInfo::getTimer() const {
  return timer;
}
void GuiColorInfo::setSide2Move(bool val) {
  side2move = val;
  invalidate();
}
bool GuiColorInfo::isSideToMove() {
  return side2move;
}
// fin GuiColorInfo

// GuiWaitPanel
GuiWaitPanel::GuiWaitPanel(bool enabled) : GuiPanel(enabled) {
  init();
}
GuiWaitPanel::GuiWaitPanel(Win32Window* window, bool enabled) : GuiPanel(window, createRC(0, 0, 0, 0), enabled) {
  init();
}
GuiWaitPanel::GuiWaitPanel(GuiComponent* parent, bool enabled) : GuiPanel(parent, createRC(0, 0, 0, 0), enabled) {
  init();
}
bool GuiWaitPanel::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  RECT rcClientLoc = getClientLoc();
  drawCenterText(hdc, "En attente d'un autre joueur...", createRC(rcClientLoc.left, rcClientLoc.top, rcClientLoc.left + rcClientLoc.right, rcClientLoc.top + rcClientLoc.bottom));
  backButton.draw(hdc, graphics);
  return true;
}
LRESULT GuiWaitPanel::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!isEnabled())
    return -1;
  else if (message == WM_COMMAND && wParam == (WPARAM)backID)
    PostMessage(getMessWndHandle(), ThreadMessage::BACKTOMAIN, 0, 0);

  backButton.eventListener(hWnd, message, wParam, lParam);
  return 0;
}
void GuiWaitPanel::setFont(HFONT hfont) {
  GuiPanel::setFont(hfont);
  backButton.setFont(getFont());
}
void GuiWaitPanel::setMessWndHandle(HWND hwnd) {
  GuiPanel::setMessWndHandle(hwnd);
  backButton.setMessWndHandle(hwnd);
}

// private:
void GuiWaitPanel::init() {
  backID = GuiComponent::uniqueButtonID.generate();
  backButton.setText("Menu Principal");
  backButton.setID(backID);
  backButton.attachTo(this, true, 15, 100, 200, 40);
}
// fin GuiWaitPanel

// GuiGamePanel
GuiGamePanel::GuiGamePanel(bool enabled) : GuiPanel(enabled), board(false) {
  init();
}
GuiGamePanel::GuiGamePanel(Win32Window* window, bool enabled) : GuiPanel(window, createRC(0, 0, 0, 0), enabled) {
  init();
}
GuiGamePanel::GuiGamePanel(GuiComponent* parent, bool enabled) : GuiPanel(parent, createRC(0, 0, 0, 0), enabled) {
  init();
}
bool GuiGamePanel::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  infoWhite.draw(hdc, graphics);
  infoBlack.draw(hdc, graphics);
  backButton.draw(hdc, graphics);
  board.draw(hdc, graphics);
  return true;
}
LRESULT GuiGamePanel::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!isEnabled())
    return -1;
  else if (message == WM_COMMAND && wParam == (WPARAM)backID)
    PostMessage(getMessWndHandle(), ThreadMessage::BACKTOMAIN, 0, 0);

  infoWhite.eventListener(hWnd, message, wParam, lParam);
  infoBlack.eventListener(hWnd, message, wParam, lParam);
  board.eventListener(hWnd, message, wParam, lParam);
  backButton.eventListener(hWnd, message, wParam, lParam);
  return 0;
}
void GuiGamePanel::setSide2Move(Chess::Color color) {
  bool side2move = (color == Chess::C_BLACK);
  infoBlack.setSide2Move(side2move);
  infoWhite.setSide2Move(!side2move);
}
void GuiGamePanel::setPlayerSide(Chess::Color c) {
  playerSide = c;
  reversed = (c == Chess::C_BLACK);
  reverse(reversed);
  invalidate();
}
void GuiGamePanel::reverse(bool r) {
  reversed = r;
  RECT rcWhite = infoWhite.getLocation();
  RECT rcBlack = infoBlack.getLocation();
  if (reversed) {
    infoWhite.setLocation(createRC(rcWhite.left, 0, rcWhite.right, rcWhite.bottom));
    infoBlack.setLocation(createRC(rcBlack.left, 592, rcBlack.right, rcBlack.bottom));
  } else {
    infoWhite.setLocation(createRC(rcWhite.left, 592, rcWhite.right, rcWhite.bottom));
    infoBlack.setLocation(createRC(rcBlack.left, 0, rcBlack.right, rcBlack.bottom));
  }
  board.reverse(reversed);
}
void GuiGamePanel::setBlackPlayerName(std::string name) {
  infoBlack.setPlayerName(name);
}
void GuiGamePanel::setWhitePlayerName(std::string name) {
  infoWhite.setPlayerName(name);
}
void GuiGamePanel::setPiece(int i, Chess::Piece* piece) {
  board.set(i, piece);
}
void GuiGamePanel::enableSelection() {
  board.enableSelection();
}
void GuiGamePanel::disableSelection() {
  board.disableSelection();
}
void GuiGamePanel::reset() {
  resetBoard();
  setSide2Move(Chess::C_WHITE);
}
void GuiGamePanel::setFont(HFONT hfont) {
  GuiPanel::setFont(hfont);
  infoWhite.setFont(getFont());
  infoBlack.setFont(getFont());
  backButton.setFont(getFont());
}
void GuiGamePanel::setMessWndHandle(HWND hwnd) {
  GuiPanel::setMessWndHandle(hwnd);
  board.setMessWndHandle(hwnd);
  infoWhite.setMessWndHandle(hwnd);
  infoBlack.setMessWndHandle(hwnd);
  backButton.setMessWndHandle(hwnd);
}

// private:
void GuiGamePanel::init() {
  reversed = false;
  backID = GuiComponent::uniqueButtonID.generate();
  infoWhite.setColor(Chess::C_WHITE);
  infoBlack.setColor(Chess::C_BLACK);
  backButton.setText("Menu Principal");
  backButton.setID(backID);
  infoBlack.attachTo(this, true, 15, 0, 482, 60);
  infoWhite.attachTo(this, true, 15, 592, 482, 60);
  board.attachTo(this, true, 0, 70, 512, 512);
  backButton.attachTo(this, true, 512 + 25, 602, 200, 40);
  setSide2Move(Chess::C_WHITE);
  setPlayerSide(Chess::C_WHITE);
}
void GuiGamePanel::resetBoard() {
  board.disable();
  board.enable();
}
// fin GuiGamePanel

// GuiMenu
GuiMenu::GuiMenu(bool enabled) : GuiPanel(enabled) {
}
GuiMenu::GuiMenu(const Win32Window* window, RECT rcLoc, bool enabled) : GuiPanel(window, rcLoc, enabled) {
}
GuiMenu::GuiMenu(const GuiComponent* parent, RECT rcLoc, bool enabled) : GuiPanel(parent, rcLoc, enabled) {
}
bool GuiMenu::addButton(std::string text, unsigned int buttonID) {
  buttons.push_back(GuiButton(this, createRC(0, 0, 0, 0), buttonID, true));
  buttons.back().setText(text);
  buttons.back().setFont(getFont());
  invalidate();
  return true;
}
bool GuiMenu::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  RECT rcLoc = getLocation();
  setLocation(createRC(rcLoc.left, rcLoc.top, rcLoc.right, YMARGIN + buttons.size() * (HEIGHT + YMARGIN)));
  drawBegin(hdc);
  GuiPanel::draw(hdc, graphics);
  drawEnd(hdc);

  int y = YMARGIN;
  RECT rcDim = getLocation();
  if (rcDim.right <= 2 * XMARGIN)
    return false;
  for(std::size_t i = 0; i < buttons.size() && rcDim.bottom >= y + HEIGHT + YMARGIN; ++i) {
    buttons[i].setLocation(createRC(XMARGIN, y, rcDim.right - 2 * XMARGIN, HEIGHT));
    buttons[i].draw(hdc, graphics);
    y += HEIGHT + YMARGIN;
  }
  return true;
}
LRESULT GuiMenu::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!isEnabled())
    return -1;
  for (auto& button : buttons) {
    button.eventListener(hWnd, message, wParam, lParam);
  }
  return 0;
}
void GuiMenu::setFont(HFONT hfont) {
  GuiPanel::setFont(hfont);
  for (auto& button : buttons) {
    button.setFont(getFont());
  }
}
void GuiMenu::setMessWndHandle(HWND hwnd) {
  GuiPanel::setMessWndHandle(hwnd);
  for (auto& button : buttons) {
    button.setMessWndHandle(hwnd);
  }
}
// fin GuiMenu

// GuiMainMenu
GuiMainMenu::GuiMainMenu(bool enabled) : GuiMenu(enabled) {
  init();
}
GuiMainMenu::GuiMainMenu(const Win32Window* window, RECT rcLoc, bool enabled) : GuiMenu(window, rcLoc, enabled) {
  init();
}
GuiMainMenu::GuiMainMenu(const GuiComponent* parent, RECT rcLoc, bool enabled) : GuiMenu(parent, rcLoc, enabled) {
  init();
}
LRESULT GuiMainMenu::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!isEnabled())
    return -1;
  else if (message == WM_COMMAND && wParam == (WPARAM)soloID) {
    PostMessage(getMessWndHandle(), ThreadMessage::SOLO, 0, 0);
  } else if (message == WM_COMMAND && wParam == (WPARAM)multiID) {
    PostMessage(getMessWndHandle(), ThreadMessage::MULTI, 0, 0);
  } else if (message == WM_COMMAND && wParam == (WPARAM)exitID) {
    PostMessage(getMessWndHandle(), ThreadMessage::EXIT, 0, 0);
  }
  return GuiMenu::eventListener(hWnd, message, wParam, lParam);
}

// private:
void GuiMainMenu::init() {
  setBackgroundColor(RGB(0, 0, 0));
  soloID = GuiComponent::uniqueButtonID.generate();
  multiID = GuiComponent::uniqueButtonID.generate();
  exitID = GuiComponent::uniqueButtonID.generate();
  addButton("SOLO", soloID);
  addButton("MULTIJOUEUR", multiID);
  addButton("QUITTER", exitID);
}
// fin GuiMainMenu

// GuiMainPanel
GuiMainPanel::GuiMainPanel(Win32Window* window, RECT rcLoc, bool enabled) : GuiPanel(window, rcLoc, enabled) {
}
GuiMainPanel::GuiMainPanel(GuiComponent* parent, RECT rcLoc, bool enabled) : GuiPanel(parent, rcLoc, enabled) {
}
bool GuiMainPanel::attachTo(const Win32Window* window, bool enabled, int x, int y, int dx, int dy) {
  bool ret = GuiComponent::attachTo(window, enabled, x, y, dx, dy);
  init();
  return ret;
}
bool GuiMainPanel::attachTo(const GuiComponent* parent, bool enabled, int x, int y, int dx, int dy) {
  bool ret = GuiComponent::attachTo(parent, enabled, x, y, dx, dy);
  init();
  return ret;
}
void GuiMainPanel::enable() {
  ShowWindow(hLogin, true);
  ShowWindow(hPwd, true);
  GuiComponent::enable();
}
void GuiMainPanel::disable() {
  ShowWindow(hLogin, false);
  ShowWindow(hPwd, false);
  GuiComponent::disable();
}
bool GuiMainPanel::draw(HDC hdc, Gdiplus::Graphics* graphics) {
  if (!isEnabled() || !graphics)
    return false;
  drawBegin(hdc);
  GuiPanel::draw(hdc, graphics);
  drawText(hdc, "Login :", 15, 15);
  drawText(hdc, "Mot de passe :", 280, 15);
  drawText(hdc, strCS.c_str(), 750, 15);
  drawEnd(hdc);

  connectButton.draw(hdc, graphics);
  return true;
}
LRESULT GuiMainPanel::eventListener(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (!isEnabled())
    return -1;
  else if (message == WM_COMMAND && wParam == connectID)
    PostMessage(getMessWndHandle(), ThreadMessage::CONNECT, 0, 0);
  return connectButton.eventListener(hWnd, message, wParam, lParam);
}
int GuiMainPanel::getLogin(char* str, unsigned int size) {
  char16_t buffer[256];
  GetWindowText(hLogin, (LPWSTR)buffer, sizeof(buffer));
  std::string login = UTF8_TO_UTF16.to_bytes(buffer);
  int outputsize = login.size() > size ? size : login.size();
  std::memcpy(str, login.data(), outputsize);
  return outputsize;
}
int GuiMainPanel::getPwd(char* str, unsigned int size) {
  char16_t buffer[256];
  GetWindowText(hPwd, (LPWSTR)buffer, sizeof(buffer));
  std::string pwd = UTF8_TO_UTF16.to_bytes(buffer);
  int outputsize = pwd.size() > size ? size : pwd.size();
  std::memcpy(str, pwd.data(), outputsize);
  return outputsize;
}
void GuiMainPanel::setLocation(RECT rcLoc) {
  GuiComponent::setLocation(rcLoc);
  MoveWindow(hLogin, getClientLoc().left + 75, getClientLoc().left + 15, 180, 22, true);
  MoveWindow(hPwd, getClientLoc().left + 390, getClientLoc().left + 15, 180, 22, true);
}
void GuiMainPanel::setCS(ConnectionStatus::ConnectionStatus cs) {
  this->cs = cs;
  if (cs == ConnectionStatus::OFFLINE) {
    connectButton.setText("Connexion");
    strCS = "Statut : Déconnecté";
  } else if (cs == ConnectionStatus::CONNECTING) {
    connectButton.setText("Connexion");
    strCS = "Statut : Connexion en cours...";
  } else if (cs == ConnectionStatus::CONNECTED) {
    connectButton.setText("Déconnexion");
    strCS = "Statut : Connecté";
  }
  invalidate();
}
void GuiMainPanel::setFont(HFONT hfont) {
  GuiPanel::setFont(hfont);
  connectButton.setFont(getFont());
}

// private:
void GuiMainPanel::init() {
  hLogin = CreateWindowEx(WS_EX_CLIENTEDGE, L"edit", nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, getWnd()->getHandle(), reinterpret_cast<HMENU>(uniqueButtonID.generate()), nullptr, nullptr);
  hPwd = CreateWindowEx(WS_EX_CLIENTEDGE, L"edit", nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_PASSWORD, 0, 0, 0, 0, getWnd()->getHandle(), reinterpret_cast<HMENU>(uniqueButtonID.generate()), nullptr, nullptr);
  MoveWindow(hLogin, getClientLoc().left + 75, getClientLoc().left + 15, 180, 22, true);
  MoveWindow(hPwd, getClientLoc().left + 390, getClientLoc().left + 15, 180, 22, true);
  connectID = GuiComponent::uniqueButtonID.generate();
  connectButton.attachTo(this, true, 600, 10, 125, 30);
  connectButton.setID(connectID);
  setCS(ConnectionStatus::OFFLINE);
}
// fin GuiMainPanel

// ChessWin
ChessWin::ChessWin(const HINSTANCE hInst, const std::string title, const std::string wndClass, const int width, const int height, const DWORD style) : Win32Window(hInst, title, wndClass, width, height, style), hbrush(nullptr), hMessWnd(nullptr) {
  RECT rcClient;
  GetClientRect(getHandle(), &rcClient);
  hbrush = CreateSolidBrush(RGB(255, 255, 255)); // au cas où on trouve pas la texture
  setBackgroundTexture("data/textures/texture02.bmp");
  hfont = CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Timeless");
  wndProc(getHandle(), WM_CREATE, 0, 0);
  wndProc(getHandle(), WM_SETFONT, (WPARAM)hfont, 0);
  mainPanel.attachTo(this, true, 0, 0, rcClient.right, 50);
  mainPanel.setBackgroundTexture("data/textures/texture01.bmp");
  mainPanel.setFont(hfont);
  mainPanel.setTextColor(RGB(225, 225, 225));
  waitPanel.attachTo(this, true, (rcClient.right - 230) / 2, 100, 230, 100);
  waitPanel.setFont(hfont);
  waitPanel.setTextColor(RGB(225, 225, 225));
  mainMenu.attachTo(this, false, 80, 150, rcClient.right - 80 * 2, 0);
  mainMenu.setBackgroundTexture("data/textures/texture05.bmp");
  mainMenu.setFont(hfont);
  gamePanel.attachTo(this, false, 15, 65, 512, 512 + 200);
  gamePanel.setFont(hfont);
  showMainMenu();
}
ChessWin::~ChessWin() {
  DeleteObject(hbrush);
  DeleteObject(hfont);
}
LRESULT ChessWin::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == WM_PAINT) {
    PAINTSTRUCT ps;
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    int width = rcClient.right;      /* - Client_Rect.left;*/
    int height = rcClient.bottom;    /* + Client_Rect.left;*/
    HDC hdc = BeginPaint(hWnd, &ps); // récupère le DC // buffer d'affichage

    HDC memHdc = CreateCompatibleDC(hdc);                           // backbuffer (on dessine ici et on copie dans le buffer d'affichage pour éviter d'avoir du clignotement : technique du double buffering)
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height); // crée une copie du dc en mémoire
    SelectObject(memHdc, memBitmap);                                // associe la copie à memHdc

    // Et on dessine sur memHdc
    FillRect(memHdc, &rcClient, hbrush); // background

    Gdiplus::Graphics graphics(memHdc);
    // dessins des composants :
    mainPanel.draw(memHdc, &graphics);
    gamePanel.draw(memHdc, &graphics);
    mainMenu.draw(memHdc, &graphics);
    waitPanel.draw(memHdc, &graphics);

    BitBlt(hdc, 0, 0, width, height, memHdc, 0, 0, SRCCOPY); // copie hdcMem dans hdc, le DC de la fenêtre
    DeleteObject(memBitmap);
    DeleteDC(memHdc);
    DeleteDC(hdc);
    EndPaint(hWnd, &ps);
  } else {
    mainPanel.eventListener(hWnd, message, wParam, lParam);
    gamePanel.eventListener(hWnd, message, wParam, lParam);
    mainMenu.eventListener(hWnd, message, wParam, lParam);
    waitPanel.eventListener(hWnd, message, wParam, lParam);
  }

  return Win32Window::wndProc(hWnd, message, wParam, lParam);
}
HFONT ChessWin::getFont() {
  return hfont;
}
bool ChessWin::setBackgroundTexture(const std::string& u8filename) {
  HBITMAP hbitmap = (HBITMAP)LoadImage(nullptr, (LPCWSTR)UTF8_TO_UTF16.from_bytes(u8filename).data(), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
  if (hbitmap == nullptr)
    return false;
  if (hbrush)
    DeleteObject(hbrush);
  hbrush = CreatePatternBrush(hbitmap);
  DeleteObject(hbitmap);
  return true;
}
void ChessWin::setMessWndHandle(HWND hwnd) {
  hMessWnd = hwnd;
  mainMenu.setMessWndHandle(hMessWnd);
  gamePanel.setMessWndHandle(hMessWnd);
  mainPanel.setMessWndHandle(hMessWnd);
  waitPanel.setMessWndHandle(hMessWnd);
}
void ChessWin::showGamePanel() {
  waitPanel.disable();
  mainMenu.disable();
  gamePanel.reset();
  gamePanel.enable();
  SendMessage(getHandle(), WM_MOUSEMOVE, 0, 0);
}
void ChessWin::showMainMenu() {
  waitPanel.disable();
  gamePanel.disable();
  mainMenu.enable();
  SendMessage(getHandle(), WM_MOUSEMOVE, 0, 0);
}
void ChessWin::showWaitPanel() {
  mainMenu.disable();
  gamePanel.disable();
  waitPanel.enable();
  SendMessage(getHandle(), WM_MOUSEMOVE, 0, 0);
}
void ChessWin::gameInvalidate() {
  gamePanel.invalidate();
}
void ChessWin::gameSetPlayerSide(Chess::Color c) {
  gamePanel.setPlayerSide(c);
}
void ChessWin::gameSetPiece(int i, Chess::Piece* piece) {
  gamePanel.setPiece(i, piece);
}
void ChessWin::gameSetWhitePlayerName(std::string name) {
  gamePanel.setWhitePlayerName(name);
}
void ChessWin::gameSetBlackPlayerName(std::string name) {
  gamePanel.setBlackPlayerName(name);
}
void ChessWin::gameSetSide2Move(const Chess::Color color) {
  gamePanel.setSide2Move(color);
}
void ChessWin::gameAllowSelection() {
  gamePanel.enableSelection();
}
void ChessWin::gameDisableSelection() {
  gamePanel.disableSelection();
}
void ChessWin::setCS(ConnectionStatus::ConnectionStatus cs) {
  mainPanel.setCS(cs);
}
int ChessWin::getLogin(char* str, int size) {
  return mainPanel.getLogin(str, size);
}
int ChessWin::getPassword(char* str, int size) {
  return mainPanel.getPwd(str, size);
}
void ChessWin::gamePanelSetPos(bool multi) { // en multi on le décale sur le coté, en solo on le centre
  if (multi)
    gamePanel.setLocation(createRC(15, 65, 512 + 250, 512 + 200));
  else {
    RECT rcClient;
    GetClientRect(getHandle(), &rcClient);
    gamePanel.setLocation(createRC((rcClient.right - (512 + 220)) / 2, 65, 512 + 250, 512 + 200));
  }
}
// fin
