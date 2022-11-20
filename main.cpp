#include <iostream>
#include <SFML/Graphics.hpp>
using namespace std;
using namespace sf;
RenderWindow window;
int window_width = VideoMode::getDesktopMode().width;
int window_height = VideoMode::getDesktopMode().height;
const int BOARD_SIZE = 8;
Font font;
Text text;
struct Tile {
	string type;
	int team;
};
using Grid = Tile[BOARD_SIZE][BOARD_SIZE];
Grid grid;
Mouse mouse;
Keyboard keyboard;
Vector2i mouse_pos;
Vector2i tile_selected;
int current_player = 1;
bool piece_selected = false;
const int TILES_NUMBER = 64;
using tilesArray = Vector2i[TILES_NUMBER];
tilesArray possible_moves;
int moves_number = 0;
const int MAXIMUM_PIECES = 32;
Vector2i whiteKing, blackKing;
int keyboardDirection = 1;
bool spacePressed = false;
int winner = 0;
bool team_castling[2] = { true, true };
bool teamInChess[2] = { false, false };
bool rook_move[2][2] = { {true, true}, {true, true} };
int enPassantColumn = -1;
int waitTransformation = -1;

bool inChess(int team);
bool isStalemated(int team);
bool inCheckmated(int team);
void addTile(tilesArray& tiles, int& size, int x, int y);

int getTeam(Vector2i tile) { return grid[tile.y][tile.x].team; }
int getRank(int team) { if (team == 1) return 1; return 0; }
string getType(Vector2i tile) { return grid[tile.y][tile.x].type; }
void movePiece(Vector2i tile, Vector2i target) {
	grid[target.y][target.x] = grid[tile.y][tile.x];
	grid[tile.y][tile.x].team = 0;
	if (grid[target.y][target.x].type == "king") {
		if (grid[target.y][target.x].team == -1) blackKing = target;
		else whiteKing = target;
	}
}
void castling(Vector2i tile, Vector2i target) {
	int direction;
	if (target.x > tile.x) direction = 1;
	else direction = -1;
	grid[tile.y][tile.x + 2 * direction] = grid[tile.y][tile.x];
	grid[tile.y][tile.x + 1 * direction] = grid[target.y][target.x];
	if (grid[tile.y][tile.x].team == -1) blackKing = target;
	else whiteKing = target;
	grid[tile.y][tile.x].team = 0;
	grid[target.y][target.x].team = 0;
}
void transformPiece(Vector2i tile, Vector2i target) {
	movePiece(tile, target);
	waitTransformation = target.x;
}
void enPassant(Vector2i tile, Vector2i target) {
	movePiece(tile, target);
	grid[tile.y][target.x].team = 0;
}
bool Xor(bool b1, bool b2) { return (b1 and not b2) or (b2 and not b1); }
bool inBoard(int x, int y) { return x >= 0 and x <= 7 and y >= 0 and y <= 7; }
bool isEmpty(int x, int y) {
	if (not inBoard(x, y)) return false;
	return grid[y][x].team == 0;
}
bool isAlly(int x, int y, int team) {
	if (not inBoard(x, y)) return false;
	return grid[y][x].team == team;
}
bool isEnemy(int x, int y, int team) {
	if (not inBoard(x, y)) return false;
	return grid[y][x].team == -team;
}
bool canMove(Vector2i tile, Vector2i target) {
	Tile t = grid[target.y][target.x];
	movePiece(tile, target);
	bool result = not inChess(current_player);
	movePiece(target, tile);
	grid[target.y][target.x] = t;
	return result;
}
void getAllTeam(tilesArray& pieces, int& size, int team) {
	for (int j = 0; j <= 7; j++) {
		for (int i = 0; i <= 7; i++) {
			if (grid[j][i].team == team) {
				addTile(pieces, size, i, j);
			}
		}
	}
}

struct Board {
	Sprite sprite;
	Image image;
	Texture texture;
	Board() {
		image.create(1000, 1000);
		for (int j = 0; j < 1000; ++j) {
			for (int i = 0; i < 1000; ++i) {
				if (Xor(j % 250 < 125, i % 250 < 125)) image.setPixel(i, j, Color(60, 60, 60, 255));
				else image.setPixel(i, j, Color(200, 200, 200, 255));
			}
		}
		texture.create(1000, 1000);
		texture.loadFromImage(image);
		sprite.setTexture(texture);
		sprite.setTextureRect(IntRect(0, 0, 1000, 1000));
		sprite.setPosition(Vector2f(200, 10));
	}
	void display() { window.draw(sprite); }
};

Board board;

Vector2f getPos(Vector2i tile) {
	float y = tile.y;
	if (keyboardDirection == -1) y = 7 - y;
	return Vector2f(board.sprite.getPosition().x + 125 * tile.x, board.sprite.getPosition().y + 125 * y);
}

void initBoard() {
	grid[0][0].type = "rook";
	grid[0][1].type = "knight";
	grid[0][2].type = "bishop";
	grid[0][3].type = "queen";
	grid[0][4].type = "king";
	grid[0][5].type = "bishop";
	grid[0][6].type = "knight";
	grid[0][7].type = "rook";
	for (int i = 0; i <= 7; ++i) {
		for (int j = 0; j < 2; ++j) grid[j][i].team = -1;
		for (int j = 2; j < 6; ++j) grid[j][i].team = 0;
		for (int j = 6; j <= 7; ++j) grid[j][i].team = 1;
		grid[1][i].type = "pawn";
		grid[6][i].type = "pawn";
	}
	grid[7][0].type = "rook";
	grid[7][1].type = "knight";
	grid[7][2].type = "bishop";
	grid[7][3].type = "queen";
	grid[7][4].type = "king";
	grid[7][5].type = "bishop";
	grid[7][6].type = "knight";
	grid[7][7].type = "rook";
	blackKing = Vector2i(4, 0);
	whiteKing = Vector2i(4, 7);
}

void displayMoves() {
	CircleShape c;
	c.setRadius(62);
	c.setFillColor(Color(0, 200, 50, 255));
	c.setPosition(getPos(tile_selected));
	window.draw(c);
	c.setFillColor(Color(200, 200, 50, 255));
	for (int i = 0; i < moves_number; i++) {
		c.setPosition(getPos(possible_moves[i]));
		window.draw(c);
	}
}

int getIndexfromType(string type) {
	if (type == "rook") return 0;
	if (type == "knight") return 1;
	if (type == "bishop") return 2;
	if (type == "queen") return 3;
	if (type == "king") return 4;
	if (type == "pawn") return 5;
}

void displayPieces() {
	Sprite sprite;
	Image image;
	Texture texture;
	image.loadFromFile("pictures/pieces.png");
	texture.loadFromImage(image);
	sprite.setTexture(texture);
	int shift, k;
	shift = 0;
	for (int j = 0; j <= 7; j++) {
		if (keyboardDirection == 1) k = j;
		else k = 7 - j;
		for (int i = 0; i <= 7; i++) {
			if (grid[j][i].team != 0) {
				shift = getIndexfromType(grid[j][i].type);
				sprite.setTextureRect(IntRect(shift * 100, 50 - 50 * grid[j][i].team, 100, 100));
				sprite.setPosition(210 + i * 125, 12 + k * 125);
				window.draw(sprite);
			}
		}
	}
	if (waitTransformation != -1) {
		for (int i = 0; i <= 3; i++) {
			sprite.setTextureRect(IntRect(i * 100, 50 - 50 * current_player, 100, 100));
			sprite.setPosition(50, 50 + i*100);
			window.draw(sprite);
		}
	}
}

void addTile(tilesArray& tiles, int& size, int x, int y) {
	tiles[size] = Vector2i(x, y);
	size++;
}

Vector2i getTile() {
	mouse_pos = mouse.getPosition(window);
	int x = floor((mouse_pos.x - board.sprite.getPosition().x) / 125);
	int y = floor((mouse_pos.y - board.sprite.getPosition().y) / 125);
	if (keyboardDirection == -1) y = 7 - y;
	return Vector2i(x, y);
}

bool containsTile(tilesArray tiles, int size, Vector2i tile) {
	for (int i = 0; i < size; i++) {
		if ((tiles[i].x == tile.x) && (tiles[i].y == tile.y)) return true;
	}
	return false;
}

void runInDirection(tilesArray& tiles, int& size, Vector2i tile, int i, int j, int team) {
	int steps = 1;
	bool stop = false;
	while (not stop) {
		int x = tile.x + i * steps;
		int y = tile.y + j * steps;
		if (isEnemy(x, y, team)) {
			addTile(tiles, size, x, y);
			stop = true;
		}
		else if (isAlly(x, y, team) or not inBoard(x, y)) stop = true;
		else addTile(tiles, size, x, y);
		steps++;
	}
}

bool lineFree(int y, int xmin, int xmax, int team) {
	bool blocked;
	for (int x = xmin; x <= xmax; x++) {
		if (not isEmpty(x, y)) return false;
		grid[y][x].type = "king";
		grid[y][x].team = team;
		blocked = inChess(team);
		grid[y][x].team = 0;
		if (blocked) return false;
	}
	return true;
}

void calculateMoves(tilesArray& tiles, int& size, Vector2i tile) {
	size = 0;
	string type = grid[tile.y][tile.x].type;
	int team = grid[tile.y][tile.x].team;
	if (type == "pawn") {
		if (isEmpty(tile.x, tile.y - team)) {
			addTile(tiles, size, tile.x, tile.y - team);
			if (isEmpty(tile.x, tile.y - 2 * team)) {
				if ((tile.y == 1 and team == -1) or (tile.y == 6 and team == 1)) addTile(tiles, size, tile.x, tile.y - 2 * team);
			}
		}
		for (int shift_x = -1; shift_x <= 1; shift_x += 2) {
			if (isEnemy(tile.x + shift_x, tile.y - team, team)) addTile(tiles, size, tile.x + shift_x, tile.y - team);
		}
		if (abs(enPassantColumn - tile.x) == 1 and enPassantColumn != -1 and ((tile.y == 3 and team == 1) or (tile.y == 4 and team == -1))) addTile(tiles, size, enPassantColumn, tile.y - team);
	}
	if (type == "knight") {
		for (int configuration = 1; configuration <= 2; configuration++) {
			for (int i = -1; i <= 1; i += 2) {
				for (int j = -1; j <= 1; j += 2) {
					int x = tile.x + configuration * i;
					int y = tile.y + (3 - configuration) * j;
					if (isEnemy(x, y, team) or isEmpty(x, y)) addTile(tiles, size, x, y);
				}
			}
		}
	}
	if (type == "bishop") {
		runInDirection(tiles, size, tile, -1, -1, team);
		runInDirection(tiles, size, tile, -1, 1, team);
		runInDirection(tiles, size, tile, 1, -1, team);
		runInDirection(tiles, size, tile, 1, 1, team);
	}
	if (type == "rook") {
		runInDirection(tiles, size, tile, -1, 0, team);
		runInDirection(tiles, size, tile, 1, 0, team);
		runInDirection(tiles, size, tile, 0, -1, team);
		runInDirection(tiles, size, tile, 0, 1, team);
	}
	if (type == "queen") {
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				if (i != 0 or j != 0) runInDirection(tiles, size, tile, i, j, team);
			}
		}
	}
	if (type == "king") {
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				if (i != 0 or j != 0) {
					int x = tile.x + i;
					int y = tile.y + j;
					if (isEnemy(x, y, team) or isEmpty(x, y)) addTile(tiles, size, x, y);
				}
			}
		}
		int line = 7 * getRank(current_player);
		if (team_castling[getRank(current_player)] and not teamInChess[getRank(current_player)]) {
			if (rook_move[getRank(current_player)][0] and lineFree(line, 1, 2, team)) addTile(tiles, size, 0, line);
			if (rook_move[getRank(current_player)][1] and lineFree(line, 5, 6, team)) addTile(tiles, size, 7, line);
		}
	}
}

void removeImpossibleMoves(tilesArray & tiles, int & size, Vector2i tile){
	int failures = 0;
	for (int i = 0; i < size; i++) {
		while (i < size - failures and not canMove(tile, tiles[i])) {
			failures++;
			tiles[i] = tiles[size - failures];
		}
	}
	size -= failures;
}

bool inChess(int team) {
	tilesArray enemies, threats;
	int enemies_number, threats_number;
	enemies_number = 0;
	getAllTeam(enemies, enemies_number, -team);
	for (int i = 0; i < enemies_number; i++) {
		threats_number = 0;
		calculateMoves(threats, threats_number, enemies[i]);
		for (int j = 0; j < threats_number; j++) {
			if (getTeam(threats[j]) == team and getType(threats[j]) == "king") return true;
		}
	}
	return false;
}

bool isStalemated(int team) {
	tilesArray pieces, moves;
	int pieces_number, moves_number;
	pieces_number = 0;
	getAllTeam(pieces, pieces_number, team);
	for (int i = 0; i < pieces_number; i++) {
		moves_number = 0;
		calculateMoves(moves, moves_number, pieces[i]);
		for (int j = 0; j < moves_number; j++) {
			if (canMove(pieces[i], moves[j])) return false;
		}
	}
	return true;
}

bool isCheckmated(int team) {
	return isStalemated(team) and inChess(team);
}

void chooseMove() {
	if (piece_selected) {
		Vector2i target = getTile();
		if (containsTile(possible_moves, moves_number, target)) {
			int side; if (target.x == 0) side = 0; else side = 1;
			if (getType(tile_selected) == "rook") rook_move[getRank(current_player)][side] = false;
			if (getType(tile_selected) == "king") team_castling[getRank(current_player)] = false;
			if (getType(tile_selected) == "pawn" and abs(tile_selected.y - target.y) == 2) enPassantColumn = tile_selected.x;
			else enPassantColumn = -1;
			if (getType(tile_selected) == "king" and abs(tile_selected.x - target.x) > 1) castling(tile_selected, target);
			else if (getType(tile_selected) == "pawn" and (target.y == 0 or target.y == 7)) transformPiece(tile_selected, target);
			else if (getType(tile_selected) == "pawn" and isEmpty(target.x, target.y) and tile_selected.x != target.x) enPassant(tile_selected, target);
			else movePiece(tile_selected, target);
			if (waitTransformation == -1) current_player *= -1;
			if (current_player == 1) text.setString("White's turn");
			else text.setString("Black's turn");
			piece_selected = false;
		}
	}
}

void checkEnd() {
	teamInChess[getRank(current_player)] = inChess(current_player);
	if (isCheckmated(current_player)) {
		text.setPosition(Vector2f(300, 400));
		text.setCharacterSize(140);
		winner = -current_player;
		if (current_player == 1) {
			text.setString("Black won !");
			text.setFillColor(Color::Black);
		}
		else text.setString("White won !");
	}
	else if (isStalemated(current_player)) {
		winner = 2;
		text.setPosition(Vector2f(300, 400));
		text.setCharacterSize(140);
		text.setString("Stalemate");
	}
}

void selectTile() {
	if (waitTransformation == -1) {
		tile_selected = getTile();
		piece_selected = true;
		if (not inBoard(tile_selected.x, tile_selected.y)) piece_selected = false;
		else if (grid[tile_selected.y][tile_selected.x].team != current_player) piece_selected = false;
		calculateMoves(possible_moves, moves_number, tile_selected);
		removeImpossibleMoves(possible_moves, moves_number, tile_selected);
	}
	else {
		mouse_pos = Mouse::getPosition(window);
		if (mouse_pos.x >= 50 and mouse_pos.x < 150 and mouse_pos.y >= 50 and mouse_pos.y < 450) {
			string type;
			if (mouse_pos.y < 150) type = "rook";
			else if (mouse_pos.y < 250) type = "knight";
			else if (mouse_pos.y < 350) type = "bishop";
			else type = "queen";
			int line; if (current_player == 1) line = 0; else line = 7;
			grid[line][waitTransformation].type = type;
			waitTransformation = -1;
			current_player *= -1;
		}
	}
}

int main() {
	window.create(VideoMode(window_width, window_height), "Chess");
	window.setPosition(Vector2i(0, 0));
	font.loadFromFile("fonts/Freeroad Black.ttf");
	text.setString("White's turn");
	text.setFont(font);
	text.setFillColor(Color::White);
	text.setPosition(Vector2f(5, 5));
	text.setCharacterSize(28);
	tile_selected = Vector2i(-1, -1);
	initBoard();
	while (window.isOpen()) {
		Event event;
		while (window.pollEvent(event)) { if (event.type == Event::Closed) window.close(); }
		if (winner == 0 and mouse.isButtonPressed(Mouse::Left)) {
			chooseMove();
			selectTile();
		}
		checkEnd();
		if (keyboard.isKeyPressed(Keyboard::Space)) {
			if (not spacePressed) {
				spacePressed = true;
				keyboardDirection *= -1;
			}
		}
		else spacePressed = false;
		window.clear();
		board.display();
		if (piece_selected) displayMoves();
		displayPieces();
		window.draw(text);
		window.display();
	}
}