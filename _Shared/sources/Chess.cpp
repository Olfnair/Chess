#include "Chess.h"

#include <cmath>
#include <string>
#include <algorithm>

UniquidGenerator Chess::excIDGenerator(1);

const int Chess::EXC_POS_INVALID = Chess::excIDGenerator.generate();

const Chess::Move Chess::Minimax::NO_MOVE(0, 0);

// Pos :
Chess::Pos::Pos() : i(0) {
}

Chess::Pos::Pos(const int i) : i(i) {
}

Chess::Pos::Pos(const int x, const int y) {
  try {
    set(x, y);
  } catch (int e) {
    throw e;
  }
}

Chess::Pos::~Pos() {
}

void Chess::Pos::set(const int i) {
  // pas de vérification de i, on veut que ce soit rapide
  this->i = i;
}

void Chess::Pos::set(const int x, const int y) {
  if (x >= X_SIZE || y >= Y_SIZE || x < 0 || y < 0)
    throw EXC_POS_INVALID; // position invalide sur un plateau d'échecs
  set(y * X_SIZE + x);
}

bool Chess::Pos::build(Pos from, const int dx, const int dy) {
  if (from.getX() + dx >= X_SIZE || from.getY() + dy >= Y_SIZE || from.getX() + dx < 0 || from.getY() + dy < 0)
    return false;
  set(from + dy * X_SIZE + dx);
  return true;
}

int Chess::Pos::getX() const {
  return i % X_SIZE;
}

int Chess::Pos::getY() const {
  return i / X_SIZE;
}

Chess::Pos::operator int() const {
  return i;
}

bool Chess::Pos::operator==(const Chess::Pos& p) const {
  return (int(*this) == int(p));
}

bool Chess::Pos::operator!=(const Chess::Pos& p) const {
  return (int(*this) != int(p));
}

bool Chess::Pos::isLegal() const {
  return i >= 0 && i < Chess::X_SIZE * Y_SIZE;
}

// Move :
Chess::Move::Move() : from(0), to(0) {
}

Chess::Move::Move(const Pos from, const Pos to) : from(from), to(to) {
  dx = to.getX() - from.getX();
  dy = to.getY() - from.getY();
}

Chess::Move::~Move() {
}

void Chess::Move::setFrom(const Chess::Pos from) {
  this->from = from;
}

void Chess::Move::setTo(const Chess::Pos to) {
  this->to = to;
}

void Chess::Move::set(const Chess::Pos from, const Chess::Pos to) {
  setFrom(from);
  setTo(to);
  dx = to.getX() - from.getX();
  dy = to.getY() - from.getY();
}

bool Chess::Move::build(Pos from, const int dx, const int dy) {
  Pos to;
  if (!to.build(from, dx, dy))
    return false;
  set(from, to);
  return true;
}

const Chess::Pos& Chess::Move::getFrom() const {
  return from;
}

const Chess::Pos& Chess::Move::getTo() const {
  return to;
}

bool Chess::Move::incrStraight() { // fait avancer la position from d'une case vers la position to (en ligne la plus droite possible)
  if (from == to)
    return false;
  int x = (dx > 0 ? 1 : (-1));
  int y = (dy > 0 ? X_SIZE : (-(X_SIZE)));
  if (dx != 0) {
    from.set(from + x);
    dx -= x;
  }
  if (dy != 0) {
    from.set(from + y);
    dy -= (y > 0 ? 1 : (-1));
  }
  return true;
}

int Chess::Move::getDX() const {
  return dx;
}

int Chess::Move::getDY() const {
  return dy;
}

void Chess::Move::reverse() {
  Pos tmp = to;
  to = from;
  from = tmp;
  dx = -dx;
  dy = -dy;
}

bool Chess::Move::isMove(const Move& move) { // vérifie qu'il y a bien eu déplacement
  return move.getFrom() != move.getTo();
}

// les suivantes renvoie tjrs true pr dx == 0 && dy == 0 (une pièce est sur la même diagonale qu'elle même etc...)
bool Chess::Move::isDiag(const Move& move) { // déplacement en diagonale
  return abs(move.getDX()) == abs(move.getDY());
}

bool Chess::Move::isLine(const Move& move) { // déplacement sur une ligne
  return move.getDY() == 0;
}

bool Chess::Move::isCol(const Move& move) { // déplacement sur une colone
  return move.getDX() == 0;
}

bool Chess::Move::isStraight(const Move& move) { // déplacement en ligne droite
  return isLine(move) || isCol(move);
}

bool Chess::Move::isAdj(const Move& move) { // déplacement sur une case adjacente
  return (abs(move.getDX()) <= 1 && abs(move.getDY()) <= 1);
}

// rêgles de base pour les pièces
bool Chess::Move::isKing(const Move& move) { // roi
  return isAdj(move);
}

bool Chess::Move::isKingCastle(const Move& move) { // roi roque
  return isLine(move) && abs(move.getDX()) == 2;
}

bool Chess::Move::isQueen(const Move& move) { // reine
  return isDiag(move) || isStraight(move);
}

bool Chess::Move::isRook(const Move& move) { // tour
  return isStraight(move);
}

bool Chess::Move::isBishop(const Move& move) { // fou
  return isDiag(move);
}

bool Chess::Move::isKnight(const Move& move) { // cavalier
  return (abs(move.getDX()) == 1 && abs(move.getDY()) == 2) || (abs(move.getDX()) == 2 && abs(move.getDY()) == 1);
}

bool Chess::Move::isPawn(const Move& move, const Color c) { // pion déplacement standard
  // les pions blancs montent, les noirs descendent (==> pion blanc x diminue, pion noir x augmente)
  return isCol(move) && move.getDY() == 1 - (c << 1);
}

bool Chess::Move::isPawnFirst(const Move& move, const Color c) { // pion, premier déplacement
  // les pions blancs montent, les noirs descendent (==> pion blanc x diminue, pion noir x augmente)
  return isPawn(move, c) || (isCol(move) && move.getDY() == 2 - (c << 2));
}

bool Chess::Move::isPawnTake(const Move& move, const Color c) { // prise du pion (.. déterminer si c'est en passant ou pas)
  return move.getDY() == 1 - (c << 1) && isDiag(move);
}

bool Chess::Move::isPawnEnPassant(const Move& move, const Color c) { // suceptible d'être une prise en passant (ça peut aussi etre une prise normale)
  return move.getFrom().getY() == ((c == C_WHITE) ? 3 : (X_SIZE - 1) - 3) && isPawnTake(move, c);
}

bool Chess::Move::operator==(const Chess::Move& m) const {
  return getFrom() == m.getFrom() && getTo() == m.getTo();
}

bool Chess::Move::operator!=(const Chess::Move& m) const {
  return !operator==(m); // = return ! ((*this) == m);
}

// PVLine
Chess::PVLine::PVLine() : depth(0) {
}

Chess::PVLine::~PVLine() {
}

void Chess::PVLine::set(int depth, Move move) {
  if (this->depth >= depth)
    ; // NOP
  else
    this->depth = depth + 1;
  line[depth] = move;
}

Chess::Move Chess::PVLine::get(int depth) const {
  return line[depth];
}

void Chess::PVLine::reset() {
  depth = 0;
}

int Chess::PVLine::length() const {
  return depth;
}

void Chess::PVLine::concat(PVLine& pv) {
  memcpy(&line + depth * sizeof(Move), &pv.line, pv.length() * sizeof(Move));
  depth = depth + pv.length();
}

// EvaluatedMove
Chess::EvaluatedMove::EvaluatedMove() {
}

Chess::EvaluatedMove::EvaluatedMove(const Move& move, int value) : move(move), value(value) {
}

Chess::EvaluatedMove::~EvaluatedMove() {
}

const Chess::Move& Chess::EvaluatedMove::getMove() const {
  return move;
}

int Chess::EvaluatedMove::getValue() const {
  return value;
}

void Chess::EvaluatedMove::setValue(int value) {
  this->value = value;
}

void Chess::EvaluatedMove::setPVLine(PVLine pvLine) {
  pvLine2Play = pvLine;
}

const Chess::PVLine& Chess::EvaluatedMove::getPVLine() const {
  return pvLine2Play;
}

// MoveList
Chess::MoveList::MoveList() : len(0) {
}

Chess::MoveList::~MoveList() {
}

bool Chess::MoveList::add(const Move move) {
  /*if (len >= MAXLEN) // ne devrait jamais arriver avec une taille de 256, le max théorique possible étant de l'ordre de 218
		return false;*/
  list[len++] = move;
  return true;
}

Chess::Move Chess::MoveList::get(int x) const {
  return list[x];
}

int Chess::MoveList::length() const {
  return len;
}

void Chess::MoveList::reset() {
  len = 0;
}

// Piece :
Chess::Piece::Piece(Board& board, Color c, Pos pos) : Piece(0, 0, board, c, pos) {
}

Chess::Piece::Piece(int type, int value, Board& board, Color c, Pos pos) : type(type), value(value), board(board), color(c), moveCount(0) {
  setPos(pos);
  resetMoveCount();
}

Chess::Piece::~Piece() {
}

bool Chess::Piece::isLegal(const Move& move) const { // vérifie qu'un mouvement est valide pour la pièce
  return Move::isMove(move) && (isKnight() || getBoard().isRightClean(move)) && !getBoard().isDestAlly(move);
}

// virtual
bool Chess::Piece::isRook() const {
  return type & IS_ROOK;
}

bool Chess::Piece::isKnight() const {
  return type & IS_KNIGHT;
}

bool Chess::Piece::isBishop() const {
  return type & IS_BISHOP;
}

bool Chess::Piece::isQueen() const {
  return type & IS_QUEEN;
}

bool Chess::Piece::isKing() const {
  return type & IS_KING;
}

bool Chess::Piece::isPawn() const {
  return type & IS_PAWN;
}

int Chess::Piece::evaluate() const {
  return value;
}

bool Chess::Piece::threatens(const Piece& piece) const {
  return threatens(piece.getPos());
}

bool Chess::Piece::targets(const Piece& piece) const {
  return threatens(piece) && Piece::isLegal(moveTo(piece));
}

bool Chess::Piece::targets(const Pos& pos) const {
  return threatens(pos) && Piece::isLegal(moveTo(pos));
}

void Chess::Piece::kill() {
  board.remPiece(this, pos);
  alive = false;
}

void Chess::Piece::resurrect() {
  board.setPiece(this, pos);
  alive = true;
}

bool Chess::Piece::isAlive() {
  return alive;
}

bool Chess::Piece::moved() {
  return getMoveCount() > 0;
}

Chess::Piece* Chess::Piece::getPiece(const Pos& pos) const {
  return board(pos);
}

const Chess::Board& Chess::Piece::getBoard() const {
  return board;
}

Chess::Board& Chess::Piece::getBoard() {
  return board;
}

void Chess::Piece::setBoard(Board& board) {
  this->board = board;
}

const Chess::Pos& Chess::Piece::getPos() const {
  return pos;
}

void Chess::Piece::setPos(Pos pos) {
  this->pos = pos;
}

unsigned int Chess::Piece::getMoveCount() const {
  return moveCount;
}

bool Chess::Piece::getColor() const {
  return color;
}

void Chess::Piece::setColor(Color c) {
  color = c;
}

bool Chess::Piece::isAlly(const Piece* piece) const {
  return isAlly(this, piece);
}

bool Chess::Piece::isFoe(const Piece* piece) const {
  return isFoe(this, piece);
}

// static
bool Chess::Piece::isEmpty(const Piece* piece) {
  return piece == NULL;
}

// static
bool Chess::Piece::isAlly(const Piece* a, const Piece* b) {
  return !isEmpty(a) && !isEmpty(b) && a->getColor() == b->getColor();
}

// static
bool Chess::Piece::isFoe(const Piece* a, const Piece* b) {
  return !isEmpty(a) && !isEmpty(b) && !isAlly(a, b);
}

void Chess::Piece::resetMoveCount() {
  moveCount = 0;
}

void Chess::Piece::incrMoveCount() {
  moveCount++;
}

void Chess::Piece::decrMoveCount() {
  moveCount--;
}

Chess::Move Chess::Piece::moveTo(const Pos& pos) const {
  Move move(getPos(), pos);
  return move;
}

Chess::Move Chess::Piece::moveTo(const Piece& piece) const {
  return moveTo(piece.getPos());
}

// RookLister
Chess::RookLister::RookLister(int type, int value, Board& board, Color c, Pos pos) : Piece(type, value, board, c, pos) {
}

Chess::RookLister::~RookLister() {
}

unsigned int Chess::RookLister::listLegalMoves(MoveList& moveList) const {
  Move move;
  int count = 0;

  // monter
  move.set(getPos(), getPos());
  for (int dx = 0, dy = 1; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); ++dy) {
    moveList.add(move);
    ++count;
  }

  // descendre
  move.set(getPos(), getPos());
  for (int dx = 0, dy = -1; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); --dy) {
    moveList.add(move);
    ++count;
  }

  // droite
  move.set(getPos(), getPos());
  for (int dx = 1, dy = 0; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); ++dx) {
    moveList.add(move);
    ++count;
  }

  // gauche
  move.set(getPos(), getPos());
  for (int dx = -1, dy = 0; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); --dx) {
    moveList.add(move);
    ++count;
  }

  return count;
}

// BishopLister
Chess::BishopLister::BishopLister(int type, int value, Board& board, Color c, Pos pos) : Piece(type, value, board, c, pos) {
}

Chess::BishopLister::~BishopLister() {
}

unsigned int Chess::BishopLister::listLegalMoves(MoveList& moveList) const {
  Move move;
  int count = 0;

  // monter à droite
  move.set(getPos(), getPos()); // on crée un premier mouvement du fou vers lui même
  // si le fou rencontre une pièce, il ne peut pas aller plus loin sur cette diagonale
  // donc avant de créer le prochain mouvement on vérifie si on a pas rencontré un ennemi (faux au début, parce qu'on se rencontre soi même et qu'on est allié)
  // et après avoir créé le mouvement, il faut vérifier que sa destination n'est pas un allié
  // si une de ces conditions est vraie on s'arrête
  for (int dx = 1, dy = 1; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); ++dx, ++dy) {
    moveList.add(move);
    ++count;
  }

  // descendre à droite
  move.set(getPos(), getPos());
  for (int dx = 1, dy = -1; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); ++dx, --dy) {
    moveList.add(move);
    ++count;
  }

  // monter à gauche
  move.set(getPos(), getPos());
  for (int dx = -1, dy = 1; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); --dx, ++dy) {
    moveList.add(move);
    ++count;
  }

  // descendre à gauche
  move.set(getPos(), getPos());
  for (int dx = -1, dy = -1; !getBoard().isDestFoe(move) && move.build(getPos(), dx, dy) && !getBoard().isDestAlly(move); --dx, --dy) {
    moveList.add(move);
    ++count;
  }
  return count;
}

// King :
Chess::King::King(Board& board, Color c, Pos pos) : Piece(IS_KING, 0, board, c, pos) {
}

Chess::King::~King() {
}

bool Chess::King::isLegal(const Move& move) const {
  return (Move::isKing(move) || ((Move::isKingCastle(move) && canCastle(move) && Piece::isLegal(move))));
}

bool Chess::King::threatens(const Pos& pos) const {
  return Move::isKing(moveTo(pos)); // si un déplacement vers la pièce est valide, c'est qu'on la menace
}

bool Chess::King::canCastle(const Move& move) const {
  Piece* rook = getBoard().getRookCastle(move);
  Move rookMove;
  if (rook)
    rookMove = moveTo(*rook);
  // on vérifie que le roi et la tour n'ont pas bougé et que les cases entre eux sont vides
  return getMoveCount() == 0 && rook && rook->isRook() && rook->getColor() == getColor() && rook->getMoveCount() == 0 && getBoard().isRightClean(rookMove);
  // on devra encore vérifier que ni le roi, ni la tour ne sont en échec (la tour, parce qu'on ne peut pas mettre son roi en échec)
}

unsigned int Chess::King::listLegalMoves(MoveList& moveList) const {
  Move move;
  int count = 0;

  // haut
  if (move.build(getPos(), 0, 1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // haut droite
  if (move.build(getPos(), 1, 1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // haut gauche
  if (move.build(getPos(), -1, 1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // droite
  if (move.build(getPos(), 1, 0) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // gauche
  if (move.build(getPos(), -1, 0) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // bas
  if (move.build(getPos(), 0, -1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // bas droite
  if (move.build(getPos(), 1, -1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // bas gauche
  if (move.build(getPos(), -1, -1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // petit roque (droite)
  if (move.build(getPos(), 2, 0) && Move::isKingCastle(move) && canCastle(move) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // grand roque (gauche)
  if (move.build(getPos(), -2, 0) && Move::isKingCastle(move) && canCastle(move) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  return count;
}

Chess::Piece* Chess::King::getCopy() const {
  return new King(*this);
}

// Queen :
Chess::Queen::Queen(Board& board, Color c, Pos pos) : Piece(IS_QUEEN, 100, board, c, pos), RookLister(IS_QUEEN, 100, board, c, pos), BishopLister(IS_QUEEN, 100, board, c, pos) {
}

Chess::Queen::~Queen() {
}

bool Chess::Queen::isLegal(const Move& move) const {
  return Move::isQueen(move) && Piece::isLegal(move);
}

bool Chess::Queen::threatens(const Pos& pos) const {
  return Move::isQueen(moveTo(pos)); // si un déplacement vers la pièce est valide, c'est qu'on la menace
}

unsigned int Chess::Queen::listLegalMoves(MoveList& moveList) const {
  int count = 0;

  count += BishopLister::listLegalMoves(moveList);
  count += RookLister::listLegalMoves(moveList);
  return count;
}

Chess::Piece* Chess::Queen::getCopy() const {
  return new Queen(*this);
}

// Rook :
Chess::Rook::Rook(Board& board, Color c, Pos pos) : Piece(IS_ROOK, 50, board, c, pos), RookLister(IS_ROOK, 50, board, c, pos) {
}

Chess::Rook::~Rook() {
}

bool Chess::Rook::isLegal(const Move& move) const {
  return Move::isRook(move) && Piece::isLegal(move);
}

bool Chess::Rook::threatens(const Pos& pos) const {
  return Move::isRook(moveTo(pos)); // si un déplacement vers la pièce est valide, c'est qu'on la menace
}

Chess::Piece* Chess::Rook::getCopy() const {
  return new Rook(*this);
}

// Bishop :
Chess::Bishop::Bishop(Board& board, Color c, Pos pos) : Piece(IS_BISHOP, 30, board, c, pos), BishopLister(IS_BISHOP, 30, board, c, pos) {
}

Chess::Bishop::~Bishop() {
}

bool Chess::Bishop::isLegal(const Move& move) const {
  return Move::isBishop(move) && Piece::isLegal(move);
}

bool Chess::Bishop::threatens(const Pos& pos) const {
  return Move::isBishop(moveTo(pos)); // si un déplacement vers la pièce est valide, c'est qu'on la menace
}

Chess::Piece* Chess::Bishop::getCopy() const {
  return new Bishop(*this);
}

// Knight :
Chess::Knight::Knight(Board& board, Color c, Pos pos) : Piece(IS_KNIGHT, 30, board, c, pos) {
}

Chess::Knight::~Knight() {
}

bool Chess::Knight::isLegal(const Move& move) const {
  return Move::isKnight(move) && Piece::isLegal(move);
}

bool Chess::Knight::threatens(const Pos& pos) const {
  return Move::isKnight(moveTo(pos)); // si un déplacement vers la pièce est valide, c'est qu'on la menace
}

unsigned int Chess::Knight::listLegalMoves(MoveList& moveList) const {
  Move move;
  int count = 0;

  if (move.build(getPos(), 2, 1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), 2, -1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), -2, 1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), -2, -1) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), 1, 2) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), 1, -2) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), -1, 2) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  if (move.build(getPos(), -1, -2) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  return count;
}

Chess::Piece* Chess::Knight::getCopy() const {
  return new Knight(*this);
}

// Pawn :
Chess::Pawn::Pawn(Board& board, Color c, Pos pos) : Piece(IS_PAWN, 10, board, c, pos) {
}

Chess::Pawn::~Pawn() {
}

bool Chess::Pawn::isEnPassant(const Move& move) const {
  // (on vérifie aussi que la destination est une case vide, sinon ça peut être une prise, mais pas en passant)
  if (!Move::isPawnEnPassant(move, getColor()) || getPiece(move.getTo())) // Est-ce que ce mouvement peut correspondre à une prise en passant ?
    return false;
  // si oui :
  int y = ((getColor() == C_WHITE) ? X_SIZE : (-(X_SIZE)));                                                                    // (détermine si on doit avancer ou reculer d'une ligne pour chercher la piece, en fonction de la couleur du pion)
  Piece* piece = getPiece(move.getTo() + y);                                                                                   // on va aller voir si une pièce se trouve derrière la destination de notre pion
  return isFoe(piece) && piece->isPawn() && piece->getPos() == getBoard().getLastMove().getTo() && piece->getMoveCount() == 1; // si on trouve un pion adverse qui a avancé de 2 cases au coup d'avant, c'est une prise en passant
}

bool Chess::Pawn::isLegal(const Move& move) const {
  return ((Move::isPawn(move, getColor()) && getBoard().isDestEmpty(move))         // déplacement normal
          || (getMoveCount() == 0 && Move::isPawnFirst(move, getColor()))          // avance de 2 cases au premier mouvement
          || (isFoe(getPiece(move.getTo())) && Move::isPawnTake(move, getColor())) // prend une piece adverse
          || (isEnPassant(move)))                                                  // prise d'un pion adverse "en passant"
         && Piece::isLegal(move);                                                  // vérifs de base (obstacles etc)
}

bool Chess::Pawn::threatens(const Pos& pos) const {
  return Move::isPawnTake(moveTo(pos), getColor()); // si un déplacement vers la pièce est valide, c'est qu'on la menace
}

unsigned int Chess::Pawn::listLegalMoves(MoveList& moveList) const {
  Move move;
  int count = 0;
  int yforward = (getColor() == Chess::C_BLACK) ? 1 : (-1); // avancer d'une ligne

  // avancer d'un case;
  if (move.build(getPos(), 0, yforward) && getBoard().isDestEmpty(move) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // avancer de 2 cases
  if (move.build(getPos(), 0, 2 * yforward) && getMoveCount() == 0 && getBoard().isDestEmpty(move) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // prise à droite (en passant ou pas)
  if (move.build(getPos(), 1, yforward) && ((isFoe(getPiece(move.getTo())) && Move::isPawnTake(move, getColor())) || isEnPassant(move)) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }

  // prise à gauche (en passant ou pas)
  if (move.build(getPos(), -1, yforward) && ((isFoe(getPiece(move.getTo())) && Move::isPawnTake(move, getColor())) || isEnPassant(move)) && Piece::isLegal(move)) {
    moveList.add(move);
    ++count;
  }
  return count;
}

Chess::Piece* Chess::Pawn::getCopy() const {
  return new Pawn(*this);
}

// Board :
Chess::Board::Board() : Matrix(X_SIZE, Y_SIZE) {
}

Chess::Board::~Board() {
}

bool Chess::Board::setPiece(Piece* piece, const Pos pos) {
  // TODO : générer exceptions ?
  try {
    (*this)(pos) = piece; // ne lance pas d'exception
  } catch (int) {
    return false; // pour bien faire il faudrait relancer l'exception
  }
  if (piece)
    piece->setPos(pos);
  return true;
}

bool Chess::Board::remPiece(const Piece* piece, const Pos pos) {
  // TODO : générer exceptions ?
  if (getPiece(pos) != piece)
    return false;
  return setPiece(nullptr, pos);
}

Chess::Piece* Chess::Board::getPiece(const Pos& pos) const { // on emploie plutôt operator()(int i) héritée de Matrix, déplacer en private ?
  // TODO : générer exceptions ?
  try {
    return (*this)(pos); // ne lance pas d'exception
  } catch (int) {
    return nullptr; // pour bien faire il faudrait relancer l'exception
  }
}

bool Chess::Board::executeMove(const Move& move) {
  Piece* piece = getPiece(move.getFrom());
  if (!piece || !setPiece(piece, move.getTo()))
    return false;
  setPiece(nullptr, move.getFrom());
  return true;
}

void Chess::Board::setLastMove(const Move& move) {
  lastMove = move;
}

const Chess::Move& Chess::Board::getLastMove() const {
  return lastMove;
}

bool Chess::Board::isRightClean(Move move) const { // vrai si aucune piece n'occupe les cases strictement comprises entre from et to (en ligne droite)
  /*if(! isValidMove(move)) // s'assurer en amont que ce soit le cas
		return false;*/
  while (move.incrStraight() && move.getFrom() != move.getTo()) {
    if (getPiece(move.getFrom()) != NULL)
      return false;
  }
  return true;
}

bool Chess::Board::isEmpty(const Pos& pos) const {
  return Piece::isEmpty(getPiece(pos));
}

bool Chess::Board::isDestAlly(const Move& move) const {
  return Piece::isAlly(getPiece(move.getFrom()), getPiece(move.getTo()));
}

bool Chess::Board::isDestEmpty(const Move& move) const {
  return Piece::isEmpty(getPiece(move.getTo()));
}

bool Chess::Board::isDestFoe(const Move& move) const {
  return !isDestEmpty(move) && !isDestAlly(move);
}

bool Chess::Board::isValidMove(const Move& move) {
  return ((move.getFrom() != move.getTo())                           // le mouvement n'est pas nul
          && move.getFrom() >= 0 && move.getFrom() < X_SIZE * Y_SIZE // pos de départ valable
          && move.getTo() >= 0 && move.getTo() < X_SIZE * Y_SIZE);   // pos d'arrivée valable
}

Chess::Piece* Chess::Board::getRookCastle(const Move& move) const { // renvoie la piece à la position ou devrait se trouver la tour pour un mouvement correspondant à un roque
  Piece* rook = NULL;
  if (!Move::isKingCastle(move))
    return NULL;
  if (move.getDX() < 0) // grand roque (vers la gauche)
    rook = getPiece(Pos(0, move.getFrom().getY()));
  else // petit roque
    rook = getPiece(Pos(Chess::X_SIZE - 1, move.getFrom().getY()));
  return rook;
}

// MoveInfo
Chess::MoveInfo::MoveInfo() : to(NULL), other(NULL), move(0, 0), otherMove(0, 0) {
}

Chess::MoveInfo::~MoveInfo() {
  // on a créé des copies des pieces, il faut les supprimer
  if (to)
    delete to;
  if (other)
    delete other;
}

void Chess::MoveInfo::setTo(Piece* piece) {
  if (!piece)
    return;
  to = piece->getCopy();
}

void Chess::MoveInfo::setOther(Piece* piece) {
  if (!piece)
    return;
  other = piece->getCopy();
}

void Chess::MoveInfo::setMove(const Move& move) {
  this->move = move;
}

void Chess::MoveInfo::setOtherMove(const Move& move) {
  this->otherMove = move;
}

Chess::Piece* Chess::MoveInfo::getTo() {
  return to;
}

Chess::Piece* Chess::MoveInfo::getOther() {
  return other;
}

Chess::Move& Chess::MoveInfo::getMove() {
  return move;
}

Chess::Move& Chess::MoveInfo::getOtherMove() {
  return otherMove;
}

bool Chess::MoveInfo::isThereOtherMove() {
  return !(otherMove.getFrom() == Pos(0) && otherMove.getTo() == Pos(0));
}

void Chess::MoveInfo::reset() {
  // on a créé des copies des pieces, il faut les supprimer
  if (to)
    delete to;
  if (other)
    delete other;
  move.set(0, 0);
  otherMove.set(0, 0);
  to = NULL;
  other = NULL;
}

// Player :
Chess::Player::Player() {
}

Chess::Player::Player(const std::string name, const Chess::Color color) : name(name), color(color) {
}

Chess::Player::~Player() {
}

void Chess::Player::setColor(const Color color) {
  this->color = color;
}

Chess::Color Chess::Player::getColor() const {
  return color;
}

void Chess::Player::setName(const std::string name) {
  this->name = name;
}

const std::string& Chess::Player::getName() const {
  return name;
}

// Game :
Chess::Game::Game(Player& white, Player& black) : white(white), black(black) {
}

Chess::Game::~Game() {
  clean();
}

Chess::Board& Chess::Game::getBoard() {
  return board;
}

Chess::Color Chess::Game::getTrait() const {
  return trait;
}

void Chess::Game::setTrait(const Color c) {
  trait = c;
}

void Chess::Game::setWhitePlayer(const Player& player) {
  this->white = player;
}

void Chess::Game::setBlackPlayer(const Player& player) {
  this->black = player;
}

const Chess::Player& Chess::Game::getWhitePlayer() const {
  return white;
}

const Chess::Player& Chess::Game::getBlackPlayer() const {
  return black;
}

const Chess::Player& Chess::Game::initForPlayers(const std::string nameA, const std::string nameB) { // initialise les joueurs avec ces 2 noms, renvoie une reférence vers le joueur blanc
  reset();
  if (rand() & 0x0001) { // on décide aléatoirement de qui est blanc, qui est noir
    white.setName(nameA);
    black.setName(nameB);
  } else {
    white.setName(nameB);
    black.setName(nameA);
  }
  white.setColor(Chess::C_WHITE);
  black.setColor(Chess::C_BLACK);
  return white;
}

const Chess::Player& Chess::Game::getPlayerFromName(const std::string name) const { // on suppose que name est valide
  if (white.getName().compare(name) == 0)
    return white;
  return black;
}

const Chess::Player& Chess::Game::getPlayerFrom(const Color c) const {
  if (c == Chess::C_WHITE)
    return white;
  return black;
}

bool Chess::Game::isCheck(const Player& player, const Pos& pos) { // vrai si au moins une piece adverse menace cette case
  for(Piece* piece : (player.getColor() == Chess::C_WHITE) ? blackPieces : whitePieces) {
    if(piece->targets(pos))
      return true;
  }
  return false;
}

unsigned int Chess::Game::listLegalMoves(const Player& player, MoveList& moveList) {
  MoveInfo info;
  MoveList tmpList;

  for(Piece* piece : (player.getColor() == Chess::C_WHITE) ? whitePieces : blackPieces) {
    piece->listLegalMoves(tmpList);
  }

  //moveList.reset(); // on vide le tableau
  // on doit encore supprimer les coups qui mettent le roi en échec (ou le roque non autorisé)
  // (en fait on recopie les coups valables)
  const King* PLAYER_KING = ((player.getColor() == Chess::C_WHITE) ? whiteKing : blackKing);
  const Pos PLAYER_KING_POS = PLAYER_KING->getPos();
  const bool IS_CHECK = isCheck(player, PLAYER_KING_POS);
  for (int i = 0; i < tmpList.length(); ++i) {
    const Move move = tmpList.get(i);
    if (!isCastle(move) || isLegalCastle(move)) {
      const Pos FROM_POS = move.getFrom();
      if (IS_CHECK || FROM_POS.getX() == PLAYER_KING_POS.getX() || FROM_POS.getY() == PLAYER_KING_POS.getY() || Move::isDiag(Move(FROM_POS, PLAYER_KING_POS))) {
        simulateMove(move, info);
        if (!isCheck(player, PLAYER_KING->getPos())) {
          moveList.add(move); // on ajoute uniquement les coups légaux
        }
        cancelMove(info);
      } else {
        moveList.add(move); // on ajoute uniquement les coups légaux
      }
    }
  }
  return moveList.length();
}

/*inline unsigned int Chess::Game::listLegalMoves(const Player & player, MoveList & moveList) {
	MoveInfo info;
	MoveList tmpList;
	
	for(Piece* piece : (player.getColor() == Chess::C_WHITE) ? whitePieces : blackPieces) {
    piece->listLegalMoves(tmpList);
  }

	// on doit encore supprimer les coups qui mettent le roi en échec (ou le roque non autorisé)
	// (en fait on recopie les coups valables)
	//tmpList = moveList; // copie du tableau de la liste des coups
	moveList.reset(); // on vide le tableau
	for (int i = 0; i < tmpList.length(); ++i) {
		if (!(isCastle(tmpList.get(i)) && !isLegalCastle(tmpList.get(i)))) {
			simulateMove(tmpList.get(i), info);
			if (!isCheck(player, ((player.getColor() == Chess::C_WHITE) ? whiteKing->getPos() : blackKing->getPos())))
				moveList.add(tmpList.get(i)); // on ajoute uniquement les coups légaux
			cancelMove(info);
		}
	}
	return moveList.length();
}*/

bool Chess::Game::canMakeLegalMove(const Player& player) {
  MoveList moveList;

  listLegalMoves(player, moveList);
  return moveList.length() > 0;
}

bool Chess::Game::isCheckMate(const Player& player) { // vrai si en echec et ne peut pas faire de mouvement valide (ou vérifier ça plus "intelligemment" ?)
  return isCheck(player, ((player.getColor() == Chess::C_WHITE) ? whiteKing->getPos() : blackKing->getPos())) && !canMakeLegalMove(player);
  /* Comment ne plus être en echec
	si une seule piece met le roi en echec :
		- prendre la piece qui met le roi en echec
		- bloquer la piece qui met le roi en echec si ce n'est pas un cheval ou un pion
		- bouger le roi

	si plusieurs pieces mettent le roi en echec :
		- bouger le roi
	*/
}

bool Chess::Game::isDraw(const Player& player) { // vrai si partie nulle
  // les règles de parties nulles aux echecs sont assez complexes. On simplifie donc ici en ne considérant que le pat et la règle des 50 coups, qu'on a changée en règle des 30
  return !canMakeLegalMove(player) || notake >= 30;
}

bool Chess::Game::hasTrait(const Player& player) {
  return player.getColor() == trait;
}

bool Chess::Game::validateSelect(const Pos pos) {
  Piece* pieceFrom = board.getPiece(pos);
  return pieceFrom && pieceFrom->getColor() == trait;
}

bool Chess::Game::validateSelect(const Move& move) {
  return validateSelect(move.getFrom());
}

bool Chess::Game::isEnPassant(const Move& move) {
  Piece* fromPiece = board.getPiece(move.getFrom());
  return fromPiece->isPawn() && Move::isPawnEnPassant(move, fromPiece->getColor());
}

bool Chess::Game::isCastle(const Move& move) {
  return board.getPiece(move.getFrom())->isKing() && Move::isKingCastle(move);
}

bool Chess::Game::isLegalCastle(const Move& move) { // pour vérifier qu'un roque est autorisé (suppose qu'on lui passe un mouvement de roque vérifié)
  Piece* king = board.getPiece(move.getFrom());
  const Player& player = getPlayerFrom(king->getColor());

  // on vérifie qu'aucune des cases traversées par le roi n'est en échec : départ, traversée, arrivée
  return !(isCheck(player, king->getPos()) || isCheck(player, king->getPos() + move.getDX() / 2) || isCheck(player, move.getTo()));
}

bool Chess::Game::isLegal(const Move& move) {
  return board.getPiece(move.getFrom())->isLegal(move);
}

// vrai si on prend une pièce, sinon faux
bool Chess::Game::simulateMove(const Move& move, MoveInfo& moveInfo) {
  Piece* pieceFrom = board.getPiece(move.getFrom());
  Piece* pieceTo = board.getPiece(move.getTo());
  bool take = false;

  // pour simuler un coup et pouvoir ensuite l'annuler,
  // on doit mémoriser le mouvement effectué par la pièce
  moveInfo.reset();
  moveInfo.setMove(move);
  // on doit faire une copie de la piece de destination s'il y en a une
  moveInfo.setTo(pieceTo);
  if (isCastle(move)) {
    // on doit se souvenir du mouvement de la tour
    Piece* rook = board.getRookCastle(move);
    Move rookMove(rook->getPos(), Pos((((move.getDX()) < 0) ? (move.getTo().getX() + 1) : (move.getTo().getX() - 1)), move.getTo().getY()));
    moveInfo.setOtherMove(rookMove);
    // on peut ensuite effectuer le coup
    board.executeMove(rookMove);
    rook->incrMoveCount(); // la tour a bougé
  } else if (isEnPassant(move)) {
    // on doit aussi faire une copie du pion pris en passant
    Piece* pawn = board.getPiece(Pos(move.getTo().getX(), move.getTo().getY() + ((pieceFrom->getColor() == Chess::C_WHITE) ? 1 : (-1))));
    moveInfo.setOther(pawn);
    // on peut ensuite effectuer le coup
    remPiece(pawn); // on vient de prendre ce pion, donc on le supprime
    take = true;
  }
  // on peut ensuite effectuer le coup
  if (pieceTo) { // il faut supprimer la piece de destination de la mémoire si ce n'est pas une case vide (on la prend)
    remPiece(pieceTo);
    take = true; // on a pris une piece
  }
  board.executeMove(move);
  pieceFrom->incrMoveCount();
  return take;
}

void Chess::Game::cancelMove(MoveInfo& moveInfo) {
  Piece* oldPiece = moveInfo.getTo();
  Move m = moveInfo.getMove();

  // on doit inverser le mouvement
  m.reverse();
  board.executeMove(m);
  board.getPiece(m.getTo())->decrMoveCount();
  // remettre l'éventuelle pièce prise à sa place
  if (oldPiece)
    setPiece(oldPiece->getCopy(), oldPiece->getPos());
  // inverser le mouvement d'une éventuelle autre piece impliquée dans le coup
  if (moveInfo.isThereOtherMove()) {
    m = moveInfo.getOtherMove();
    m.reverse();
    board.executeMove(m);
    board.getPiece(m.getTo())->decrMoveCount();
  }
  // remettre un pion pris en passant
  oldPiece = moveInfo.getOther();
  if (oldPiece)
    setPiece(oldPiece->getCopy(), oldPiece->getPos());
}

int Chess::Game::executeMove(const Move& move) { // effectue le mouvement si possible, sinon renvoie un code d'erreur
  MoveInfo moveInfo;
  bool take = false;
  Color c;

  if (!validateSelect(move))
    return 1; // on essaye de bouger une pièce qu'on ne devrait pas bouger (pas à nous)

  if (!isLegal(move))
    return 2; // coup illégal

  if (isCastle(move) && !isLegalCastle(move))
    return 3; // On tente de roquer alors que le roi (ou la tour est en échec)

  c = board.getPiece(move.getFrom())->getColor();
  // on simule le coup
  take = simulateMove(move, moveInfo);

  // si le coup a mis le roi en échec
  if (isCheck(getPlayerFrom(c), ((c == Chess::C_WHITE) ? whiteKing->getPos() : blackKing->getPos()))) {
    cancelMove(moveInfo); // on l'annule
    return 4;
  }

  board.setLastMove(move);
  take ? notake = 0 : ++notake;
  trait = !trait;
  return 0;
}

int Chess::Game::getNotake() {
  return notake;
}

// protected
void Chess::Game::setPiece(Piece* piece, Pos pos) {
  board.setPiece(piece, pos);
  if (piece) {
    piece->setBoard(board);
    (piece->getColor() == C_WHITE) ? whitePieces.push_back(piece) : blackPieces.push_back(piece);
    if (piece->isKing())
      (piece->getColor() == C_WHITE) ? whiteKing = (King*)piece : blackKing = (King*)piece;
  }
}

bool Chess::Game::remPiece(Piece* piece) {
  auto whiteIt = std::find(whitePieces.begin(), whitePieces.end(), piece);
  if(whiteIt != whitePieces.end()) {
    whitePieces.erase(whiteIt);
  }
  else {
    auto blackIt = std::find(blackPieces.begin(), blackPieces.end(), piece);
    if(blackIt != blackPieces.end()) {
      blackPieces.erase(blackIt);
    }
    else {
      return false;
    }
  }
  board.remPiece(piece, piece->getPos()); // on s'assure que la piece ne soit plus sur le plateau (ce serait bête d'avoir un ptr vers de la mémoire qui n'est plus allouée...)
  delete piece;                           // on libère la mémoire
  return true;
}

void Chess::Game::init() {
  whitePieces.reserve(16);
  blackPieces.reserve(16);
  
  // blancs
  setPiece(new Rook(board, C_WHITE), Pos(0, Y_SIZE - 1));
  setPiece(new Knight(board, C_WHITE), Pos(1, Y_SIZE - 1));
  setPiece(new Bishop(board, C_WHITE), Pos(2, Y_SIZE - 1));
  setPiece(new Queen(board, C_WHITE), Pos(3, Y_SIZE - 1));
  setPiece(whiteKing = new King(board, C_WHITE), Pos(4, Y_SIZE - 1));
  setPiece(new Bishop(board, C_WHITE), Pos(5, Y_SIZE - 1));
  setPiece(new Knight(board, C_WHITE), Pos(6, Y_SIZE - 1));
  setPiece(new Rook(board, C_WHITE), Pos(7, Y_SIZE - 1));
  for (int i = 0; i < X_SIZE; ++i)
    setPiece(new Pawn(board, C_WHITE), Pos(i, Y_SIZE - 1 - 1));

  // noirs
  setPiece(new Rook(board, C_BLACK), Pos(0, 0));
  setPiece(new Knight(board, C_BLACK), Pos(1, 0));
  setPiece(new Bishop(board, C_BLACK), Pos(2, 0));
  setPiece(new Queen(board, C_BLACK), Pos(3, 0));
  setPiece(blackKing = new King(board, C_BLACK), Pos(4, 0));
  setPiece(new Bishop(board, C_BLACK), Pos(5, 0));
  setPiece(new Knight(board, C_BLACK), Pos(6, 0));
  setPiece(new Rook(board, C_BLACK), Pos(7, 0));
  for (int i = 0; i < X_SIZE; ++i)
    setPiece(new Pawn(board, C_BLACK), Pos(i, 1));

  white.setColor(Chess::C_WHITE);
  black.setColor(Chess::C_BLACK);
  trait = Chess::C_WHITE;
  notake = 0;
}

void Chess::Game::clean() {
  for(auto& piece : whitePieces) {
    board.remPiece(piece, piece->getPos());
    delete piece;
  }
  for(auto& piece : blackPieces) {
    board.remPiece(piece, piece->getPos());
    delete piece;
  }
  whitePieces.clear();
  blackPieces.clear();
  notake = 0;
}

void Chess::Game::reset() {
  clean();
  init();
}

// Minimax
Chess::Move Chess::Minimax::simulateMove(const Move& move, MoveInfo& moveInfo) {
  Move lastMove = game->getBoard().getLastMove();
  bool take = game->simulateMove(move, moveInfo);
  game->getBoard().setLastMove(move);
  take ? game->notake = 0 : ++(game->notake);
  game->setTrait(!game->getTrait());
  return lastMove;
}

void Chess::Minimax::cancelMove(MoveInfo& moveInfo, const Move& lastMove) {
  game->cancelMove(moveInfo);
  game->setTrait(!game->getTrait());
  game->getBoard().setLastMove(lastMove);
}

Chess::Move Chess::Minimax::getBestMove(const unsigned int deepness) {
  int alpha;
  int beta;
  Move bestMove;
  MoveList moveList;
  std::vector<EvaluatedMove> eMoves;
  PVLine pvLine2Build;

  game->listLegalMoves(game->getPlayerFrom(game->getTrait()), moveList);
  eMoves.reserve(64);
  for (int i = 0; i < moveList.length(); ++i)
    eMoves.push_back(EvaluatedMove(moveList.get(i), 0));
  for (unsigned int iterativeDeepness = 0; iterativeDeepness < deepness; ++iterativeDeepness) {
    alpha = -50000;
    beta = 50000;

    if (iterativeDeepness > 0) {
      std::sort(eMoves.begin(), eMoves.end(), [](const EvaluatedMove& a, const EvaluatedMove& b) -> bool {
        return a.getValue() > b.getValue(); // tri par ordre décroissant
      });
    }
    bestMove = eMoves[0].getMove();
    for (auto& eMove : eMoves) {
      MoveInfo moveInfo;
      int oldnotake = game->notake;
      pvLine2Build.reset();
      Move lastMove = simulateMove(eMove.getMove(), moveInfo);
      eMove.setValue(mini(iterativeDeepness, alpha, beta, pvLine2Build, (iterativeDeepness > 0) ? &(eMove.getPVLine()) : nullptr, 0));
      game->notake = oldnotake;
      cancelMove(moveInfo, lastMove);
      eMove.setPVLine(pvLine2Build);
      if (eMove.getValue() > alpha) {
        alpha = eMove.getValue();
        bestMove = eMove.getMove();
      }
    }
  }
  return bestMove;
}

void Chess::Minimax::setColor(const Color c) {
  this->c = c;
}

void Chess::Minimax::setGame(Game* game) {
  this->game = game;
}

int Chess::Minimax::evaluate(MoveList& moveList) {
  if (moveList.length() == 0 && game->isCheck(game->getPlayerFrom(game->getTrait()), ((game->getTrait() == Chess::C_WHITE) ? game->whiteKing->getPos() : game->blackKing->getPos())))
    return c == game->getTrait() ? -50000 : 50000; // echec et mat
  else if (game->notake >= 30 || moveList.length() == 0)
    return 0; // partie nulle

  Color evalColor = ((game->getTrait() == c) ? c : !game->getTrait());
  int val = 0;
  for(Piece* piece : (evalColor == Chess::C_WHITE) ? game->whitePieces : game->blackPieces) {
    val += piece->evaluate() + 5;
  }
  for(Piece* piece : (evalColor == Chess::C_WHITE) ? game->blackPieces : game->whitePieces) {
    val -= piece->evaluate();
  }

  MoveList moveListB; // adversaire
  game->listLegalMoves(game->getPlayerFrom(!evalColor), moveListB);
  val += (evalColor == game->getTrait()) ? moveList.length() : moveListB.length();
  val -= (evalColor != game->getTrait()) ? moveList.length() : moveListB.length();
  return val;
}

int Chess::Minimax::mini(const unsigned int deepness, int alpha, int beta, PVLine& pvLineParent, const PVLine* pvLine2Play, int curdeep) {
  MoveList moveList;
  PVLine pvLine2Build;
  Move bestMove(0, 0);

  if (pvLine2Play && pvLine2Play->length() > curdeep)
    moveList.add(pvLine2Play->get(curdeep));
  game->listLegalMoves(game->getPlayerFrom(game->getTrait()), moveList);

  if (deepness <= 0 || moveList.length() == 0 || game->notake >= 30)
    return evaluate(moveList);

  for (int i = 0; i < moveList.length(); ++i) {
    MoveInfo moveInfo;
    int oldnotake = game->notake;
    Move lastMove = simulateMove(moveList.get(i), moveInfo);
    int val = maxi(deepness - 1, alpha, beta, pvLine2Build, (i == 0 && pvLine2Play) ? pvLine2Play : nullptr, curdeep + 1);
    game->notake = oldnotake;
    cancelMove(moveInfo, lastMove);
    if (val <= alpha)
      return val;
    if (val < beta) {
      beta = val;
      bestMove = moveList.get(i);
    }
  }

  if (bestMove != NO_MOVE) {
    pvLineParent.set(0, bestMove);
    pvLineParent.concat(pvLine2Build);
  }
  return beta;
}

int Chess::Minimax::maxi(const unsigned int deepness, int alpha, int beta, PVLine& pvLineParent, const PVLine* pvLine2Play, int curdeep) {
  MoveList moveList;
  PVLine pvLine2Build;
  Move bestMove(0, 0);

  if (pvLine2Play && pvLine2Play->length() > curdeep)
    moveList.add(pvLine2Play->get(curdeep));
  game->listLegalMoves(game->getPlayerFrom(game->getTrait()), moveList);

  if (deepness <= 0 || moveList.length() == 0 || game->notake >= 30)
    return evaluate(moveList);

  for (int i = 0; i < moveList.length(); ++i) {
    MoveInfo moveInfo;
    int oldnotake = game->notake;
    Move lastMove = simulateMove(moveList.get(i), moveInfo);
    int val = mini(deepness - 1, alpha, beta, pvLine2Build, (i == 0 && pvLine2Play) ? pvLine2Play : nullptr, curdeep + 1);
    game->notake = oldnotake;
    cancelMove(moveInfo, lastMove);
    if (val >= beta)
      return val;
    if (val > alpha) {
      alpha = val;
      bestMove = moveList.get(i);
    }
  }

  if (bestMove != NO_MOVE) {
    pvLineParent.set(0, bestMove);
    pvLineParent.concat(pvLine2Build);
  }
  return alpha;
}
