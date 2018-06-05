#ifndef CHESS_H
#define CHESS_H

#include "Matrix.h"
#include "tools.h"

#include <string>
#include <vector>

namespace Chess {
class Board;
typedef bool Color;

extern UniquidGenerator excIDGenerator;
const int X_SIZE = 8; // X -> Lignes     Y -> Colones
const int Y_SIZE = 8; // dimensions du plateau de jeu
const Color C_BLACK = false;
const Color C_WHITE = true; // couleurs des pieces (ne pas modifier : les valeurs importent pour les déplacements des pions)

const int IS_PAWN = 1;
const int IS_BISHOP = 2;
const int IS_KNIGHT = 4;
const int IS_ROOK = 8;
const int IS_QUEEN = 16;
const int IS_KING = 32;

extern const int EXC_POS_INVALID; // générer des ID uniques pour les exceptions (ou faire des classes...)

// Représente l'emplacement d'une case sur l'échiquier (en retenant son index dans le bloc mémoire représentant les cases de l'échiquier)
class Pos { // all inline
public:
  Pos();
  Pos(const int i);              // index
  Pos(const int x, const int y); // coordonnées
  ~Pos();
  void set(const int i);
  void set(const int x, const int y);
  bool build(Pos from, const int dx, const int dy);
  int getX() const;     // récupérer la colone (par calcul)
  int getY() const;     // récupérer la ligne (par calcul)
  operator int() const; // conversion en int (renvoie i)
  bool operator==(const Pos& p) const;
  bool operator!=(const Pos& p) const;
  bool isLegal() const;

private:
  int8_t i; // index
};

class Move { // all inline
public:
  Move();
  Move(const Chess::Pos from, const Chess::Pos to);
  ~Move();
  void setFrom(const Chess::Pos from);
  void setTo(const Chess::Pos to);
  void set(const Chess::Pos from, const Chess::Pos to);
  bool build(Pos from, const int dx, const int dy);
  const Chess::Pos& getFrom() const;
  const Chess::Pos& getTo() const;
  bool incrStraight(); // fait avancer la position from d'une case vers la position to (en ligne la plus droite possible)
  int getDX() const;
  int getDY() const;
  void reverse();

  bool operator==(const Move& m) const;
  bool operator!=(const Move& m) const;

  // règles de déplacement :
  // sert à vérifier si un déplacement est valide ou si 2 pièces sont sur la même diag, ligne, etc...
  static bool isMove(const Move& move); // vérifie qu'il y a bien eu déplacement

  // les suivantes renvoie tjrs true pr dx == 0 && dy == 0 (une pièce est sur la même diagonale qu'elle-même etc...)
  static bool isDiag(const Move& move);     // déplacement en diagonale
  static bool isLine(const Move& move);     // déplacement sur une ligne
  static bool isCol(const Move& move);      // déplacement sur une colone
  static bool isStraight(const Move& move); // déplacement en ligne droite
  static bool isAdj(const Move& move);      // déplacement sur une case adjacente

  // règles de base pour les pièces
  static bool isKing(const Move& move);                         // roi
  static bool isKingCastle(const Move& move);                   // roi roque
  static bool isQueen(const Move& move);                        // reine
  static bool isRook(const Move& move);                         // tour
  static bool isBishop(const Move& move);                       // fou
  static bool isKnight(const Move& move);                       // cavalier
  static bool isPawn(const Move& move, const Color c);          // pion déplacement standard
  static bool isPawnFirst(const Move& move, const Color c);     // pion, premier déplacement
  static bool isPawnTake(const Move& move, const Color c);      // prise du pion (.. déterminer si c'est en passant ou pas)
  static bool isPawnEnPassant(const Move& move, const Color c); // susceptible d'être une prise en passant (ça peut aussi etre une prise normale)

private:
  Chess::Pos from;
  Chess::Pos to;
  int8_t dx;
  int8_t dy;
};

class PVLine {
public:
  enum { MAXDEPTH = 25 };
  PVLine();
  ~PVLine();
  void set(int depth, Move move);
  Move get(int depth) const;
  void reset();
  int length() const;
  void concat(PVLine& pv);

private:
  Move line[MAXDEPTH];
  int depth;
};

class EvaluatedMove {
public:
  EvaluatedMove();
  EvaluatedMove(const Move& move, int value);
  ~EvaluatedMove();
  const Move& getMove() const;
  int getValue() const;
  void setValue(int value);
  void setPVLine(PVLine pvLine);
  const PVLine& getPVLine() const;

private:
  Move move;
  int value;
  PVLine pvLine2Play;
};

class MoveList {
public:
  enum { MAXLEN = 256 };
  MoveList();
  ~MoveList();
  bool add(const Move move); // Move est une structure de 32 bits, passage par copie plutot que par r�f�rence pour gagner du temps.
  Move get(int x) const;
  int length() const;
  void reset();

private:
  Move list[MAXLEN];
  int len;
};

class Piece {
public:
  Piece(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  Piece(int type, int value, Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~Piece();
  virtual bool isLegal(const Move& move) const;     // vérifie qu'un mouvement est valide pour la pièce
  virtual bool threatens(const Pos& pos) const = 0; // indique si la piece peut atteindre la pos avec un mouvement valide (sans prendre en compte les obstacles)

  bool isRook() const;
  bool isKnight() const;
  bool isBishop() const;
  bool isQueen() const;
  bool isKing() const;
  bool isPawn() const;

  int evaluate() const; // évalue la valeur de la pièce selon différents critères

  virtual unsigned int listLegalMoves(MoveList& moveList) const = 0; // liste le nombre de coups légaux pour cette piece et renvoie le nombre de coups listés

  virtual Piece* getCopy() const = 0;

  bool threatens(const Piece& piece) const; // indique si la piece pourrait en atteindre une autre au prochain coup si elles étaient seules sur l'échiquier (pas d'obstacle)
  bool targets(const Piece& piece) const;   // peut effectivement atteindre l'autre piece
  bool targets(const Pos& pos) const;       // peut effectivement atteindre la case
  void kill();
  void resurrect();
  bool isAlive();
  bool moved();
  Piece* getPiece(const Pos& pos) const; // renvoie la piece à pos sur l'échiquier (board) passé en param dans le constructeur
  const Board& getBoard() const;
  Board& getBoard();
  void setBoard(Board& board);
  const Pos& getPos() const;
  void setPos(Pos pos);
  unsigned int getMoveCount() const;
  bool getColor() const;
  void setColor(Color c);
  bool isAlly(const Piece* piece) const;
  bool isFoe(const Piece* piece) const;

  static bool isEmpty(const Piece* piece);
  static bool isAlly(const Piece* a, const Piece* b);
  static bool isFoe(const Piece* a, const Piece* b);

  void resetMoveCount();
  void incrMoveCount();
  void decrMoveCount();

protected:                               // les méthodes et attributs protégés sont directement accessibles dans les méthodes des classes filles (contrairement aux privés)
  Move moveTo(const Pos& pos) const;     // renvoie un Move de la piece this vers la pos passée en param
  Move moveTo(const Piece& piece) const; // renvoie un Move de la piece this vers la piece passée en param

private:
  int type;     // type de pièce
  int value;    // valeur de la pièce
  Board& board; // l'échiquier sur lequel se trouve la pièce :
  Pos pos;
  Color color;
  unsigned int moveCount; // nbre de fois qu'on a bougé la pièce
  bool alive;
};

class RookLister : public virtual Piece {
public:
  RookLister(int type, int value, Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~RookLister();
  virtual unsigned int listLegalMoves(MoveList& moveList) const;
};

class BishopLister : public virtual Piece {
public:
  BishopLister(int type, int value, Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~BishopLister();
  virtual unsigned int listLegalMoves(MoveList& moveList) const;
};

class King : public Piece {
public:
  King(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~King();
  virtual bool isLegal(const Move& move) const;
  virtual bool threatens(const Pos& pos) const;

  virtual unsigned int listLegalMoves(MoveList& moveList) const;
  virtual Piece* getCopy() const;

private:
  //bool castling; // a-t-on déjà roqué ?
  bool checkmate;     // échec et mat ?
  unsigned int check; // nbre de fois en échec

  bool canCastle(const Move& move) const;
};

class Queen : public RookLister, public BishopLister {
public:
  Queen(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~Queen();
  virtual bool isLegal(const Move& move) const;
  virtual bool threatens(const Pos& pos) const;

  virtual unsigned int listLegalMoves(MoveList& moveList) const;
  virtual Piece* getCopy() const;
};

class Rook : public RookLister {
public:
  Rook(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~Rook();
  virtual bool isLegal(const Move& move) const;
  virtual bool threatens(const Pos& pos) const;

  virtual Piece* getCopy() const;
};

class Bishop : public BishopLister {
public:
  Bishop(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~Bishop();
  virtual bool isLegal(const Move& move) const;
  virtual bool threatens(const Pos& pos) const;

  virtual Piece* getCopy() const;
};

class Knight : public Piece {
public:
  Knight(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~Knight();
  virtual bool isLegal(const Move& move) const;
  virtual bool threatens(const Pos& pos) const;

  virtual unsigned int listLegalMoves(MoveList& moveList) const;
  virtual Piece* getCopy() const;
};

class Pawn : public Piece {
public:
  Pawn(Board& board, Color c = C_WHITE, Pos pos = Pos(0, 0));
  virtual ~Pawn();
  bool isEnPassant(const Move& move) const;
  virtual bool isLegal(const Move& move) const;
  virtual bool threatens(const Pos& pos) const;

  virtual unsigned int listLegalMoves(MoveList& moveList) const;
  virtual Piece* getCopy() const;
};

class Board : public Matrix<Piece*> {
public:
  Board();
  ~Board();
  bool setPiece(Piece* piece, const Pos pos); // piece n'est pas const car il faut s'assurer que son attribut pos soit correct
  bool remPiece(const Piece* piece, const Pos pos);
  Piece* getPiece(const Pos& pos) const; // employer plutôt operator()(int i) héritée de Matrix -> déplacer en private ?
  bool executeMove(const Move& move);
  void setLastMove(const Move& move); // devrait plutot se trouver dans Game ?
  const Move& getLastMove() const;    // devrait plutot se trouver dans Game ?
  bool isRightClean(Move move) const; // vrai si aucune piece n'occupe les cases strictement comprises entre from et to (en ligne droite)
  bool isEmpty(const Pos& pos) const;
  bool isDestAlly(const Move& move) const;
  bool isDestEmpty(const Move& move) const;
  bool isDestFoe(const Move& move) const;
  Piece* getRookCastle(const Move& move) const; // renvoie la piece à la position ou devrait se trouver la tour pour un mouvement correspondant à un roque

  // static
  static bool isValidMove(const Move& move);

private:
  Move lastMove; // devrait plutot se trouver dans Game ?
};

class MoveInfo {
public:
  MoveInfo();
  ~MoveInfo();

  void setTo(Piece* piece);
  void setOther(Piece* piece);
  void setMove(const Move& move);
  void setOtherMove(const Move& move);
  Piece* getTo();
  Piece* getOther();
  Move& getMove();
  Move& getOtherMove();
  bool isThereOtherMove();
  void reset();

private:
  Piece* to;
  Piece* other;
  Move move;
  Move otherMove;
};

class Player {
public:
  Player();
  Player(const std::string name, const Color color = C_WHITE);
  ~Player();
  void setColor(const Color color);
  Color getColor() const;
  void setName(const std::string name);
  const std::string& getName() const;

private:
  std::string name;
  bool color;
};

class Game {
public:
  friend class Minimax;
  Game() = default;
  Game(Player& white, Player& black);
  ~Game();
  Board& getBoard();
  Color getTrait() const;
  void setTrait(const Color c);
  void setWhitePlayer(const Player& player);
  void setBlackPlayer(const Player& player);
  const Player& getWhitePlayer() const;
  const Player& getBlackPlayer() const;
  const Player& initForPlayers(const std::string nameA, const std::string nameB); // initialise les joueurs avec ces 2 noms, renvoie une reférence vers le joueur blanc
  const Player& getPlayerFromName(const std::string name) const;
  const Player& getPlayerFrom(const Color c) const;
  bool isCheck(const Player& player, const Pos& pos); // vrai si au moins une piece adverse menace directement cette pos
  unsigned int listLegalMoves(const Player& player, MoveList& moveList);
  bool canMakeLegalMove(const Player& player); // le joueur dispose d'au moins un coup valide
  bool isCheckMate(const Player& player);      // vrai si en echec et ne peut pas faire de mouvement valide (ou vérifier ça plus "intelligemment" ?)
  bool isDraw(const Player& player);           // vrai si partie nulle (player est le joueur auquel c'est au tour de jouer)
  bool hasTrait(const Player& player);
  bool validateSelect(const Pos pos);
  bool validateSelect(const Move& move);
  bool isEnPassant(const Move& move);
  bool isCastle(const Move& move);
  bool isLegalCastle(const Move& move);
  bool isLegal(const Move& move);
  bool simulateMove(const Move& move, MoveInfo& moveInfo); // effectue un coup en mémorisant ce qu'il faut pour pouvoir l'annuler, renvoie vrai si on prend une piece
  void cancelMove(MoveInfo& moveInfo);
  int executeMove(const Move& move); // effectue le mouvement si possible, sinon renvoie un code d'erreur
  int getNotake();
  void reset();

private:
  Player white;
  Player black;
  Board board;
  Color trait;
  int notake; // nombre de coups sans prise de piece
  King* whiteKing;
  King* blackKing;
  std::vector<Piece*> whitePieces;
  std::vector<Piece*> blackPieces;

  void setPiece(Piece* piece, Pos pos);
  bool remPiece(Piece* piece);
  void init();
  void clean();
};

class Minimax {
public:
  Minimax() = default;
  ~Minimax() = default;

  Move simulateMove(const Move& move, MoveInfo& moveInfo);
  void cancelMove(MoveInfo& moveInfo, const Move& lastMove);
  Move getBestMove(const unsigned int deepness);
  void setColor(const Color c);
  void setGame(Game* game);

private:
  int evaluate(MoveList& moveList);
  int alphaBeta(const unsigned int deepness, int alpha, int beta, PVLine& pvLineParent, const PVLine* pvLine2Play, int curdeep);
  int mini(const unsigned int deepness, int alpha, int beta, PVLine& pvLineParent, const PVLine* pvLine2Play, int curdeep);
  int maxi(const unsigned int deepness, int alpha, int beta, PVLine& pvLineParent, const PVLine* pvLine2Play, int curdeep);

  Color c;
  Game* game;

  static const Move NO_MOVE;
};
}; // namespace Chess

#endif // CHESS_H
