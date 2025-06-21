#include <iostream>		// Библиотека стандартного ввода/вывода данных
#include <vector>		// Библиотека динамических массивов
#include <chrono>		// Библиотека для работы с временем (реализация задержек респавна и задержек)
#include <thread>		// Библиотека для работы с многопоточностью (в данном случае используется для создания задержек между кадрами)
#include <windows.h>	// Библиотека для чтения ввода пользователя с клавиатуры
#include "RandomNum.h"  // Кастомная библиотека для генерации случайных чисел

// Глобальные переменные 
const int WIDTH = 23;	// + 1 из-за невидимого символа окончания строки
const int HEIGHT = 13; 
bool isRunning = false; // Состояние игры
XorShift32 rnd;		// Объект для генерации случайного значения

char map[HEIGHT][WIDTH] = {		// Массив для генерации карты игры 
	"######################",
	"#....................#",
	"#...@@@@......@@@@...#",
	"#....@@........@@....#",
	"#...@@@@......@@@@...#",
	"#....................#",
	"#...@@@..@@@@..@@@...#",
	"#....................#",
	"#...@@@@......@@@@...#",
	"#....@@........@@....#",
	"#...@@@@......@@@@...#",
	"#....................#",
	"######################"
}; 

void delay(int milliseconds) {
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Вспомогательные функции (изменение цвета консольного шрифта, задержки и т.д)

void setColor(int color) {      // Функция для установки цвета в командной строке
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
} 

char getPressedKey() {		// Функция считывания нажатия кнопки
	// Кнопки для выбора направления движения танка
	if (GetAsyncKeyState('W') & 0x8000) return 'w';
	if (GetAsyncKeyState('A') & 0x8000) return 'a';
	if (GetAsyncKeyState('S') & 0x8000) return 's';
	if (GetAsyncKeyState('D') & 0x8000) return 'd';
	// Кнопка для выстрела снарядом
	if (GetAsyncKeyState(' ') & 0x8000) return 'p';
	// Кнопка для выхода из игры
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return 'e';
	// Кнопка для начала игры (или же выхода со стартового экрана)
	if (GetAsyncKeyState(VK_RETURN) & 0x8000) return 'n';

	return '\0';	// Если ни одна кнопка не была нажата, возвращается пустой символ (терминарный ноль)
}

void startScreen() {
	setColor(7);    // Обычный (белый) цвет консольного шрифта
    std::cout << "Welcome to BattleCity.exe!" << std::endl << "\n";
    setColor(2);	// Зеленый цвет консольного шрифта
    std::cout << "Controls: " << std::endl << "\n";
    setColor(2);
    std::cout << "W: move tank one cell up" << std::endl;
    std::cout << "A: move tank one cell left" << std::endl;
    std::cout <<"S: move tank one cell down" << std::endl;
    std::cout <<"D: move tank one cell right" << std::endl;
    std::cout <<"Space: fire a projectile that destroys one enemy or obstacle upon collision" << std::endl;
    std::cout <<"Escape: exit the game" << std::endl << std::endl;
    setColor(4);	// Красный цвет консольного шрифта
    std::cout << "Press 'Enter' key to start the game!" << std::endl;
    setColor(7);	
}

void runGame() {	// Функция обрабатывания начала игры
	char key = getPressedKey();
    if (key == 'n') {
    	std::cout << "Starting the game: " << std::endl;
    	delay(1000);
    	isRunning = true;
    	system("cls");
    	return;
    } else if (key == 'e') {
    	std::cout << "Exiting the game: " << std::endl;
    	delay(1000);
    	isRunning = false;
    	system("cls");
    	exit(0);
    }
}

void hideCursor() {
    // Получаем стандартный вывод
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    // Получаем текущие параметры курсора
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    // Скрываем курсор, устанавливаем bVisible в 0 (не видим)
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void showCursor() {
    // Получаем стандартный вывод
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    // Получаем текущие параметры курсора
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    // Показываем курсор, устанавливаем bVisible в 1 (видим)
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// "\033[H\033[J" для очистки экрана;
void updateCell(int x, int y, char newSymbol) {
	std::cout << "\033[" << (y + 1) << ";" << (x + 1) << "H" << newSymbol;
}

class Projectile {
private:
	int x, y;
	char direction;
	bool isActive;

public: 

	// Projectile(int startX, int startY, char startDir, bool active = true)
	// 	: x(startX), y(startY), direction(startDir), isActive(active) {}

	Projectile() : x(0), y(0), direction('#'), isActive(false) {}

	// Геттеры
	int getX() const { return x; }
	int getY() const { return y; }
	char getDirection() const { return direction; }
	bool getIsActive() const { return isActive; }
	// Сеттеры
	void setDirection(char newDirection) { direction = newDirection; } 
	void setIsActive(bool newIsActive) { isActive = newIsActive; }
	void setPosition(int newX, int newY) {
		x = newX;
		y = newY;
	} 

	// Методы класса

	// Метод для отрисовки снарядов
	void draw() const {
		updateCell(x, y, direction);
	}

	void move() {
		if (isActive) {
			int oldX = x;
			int oldY = y;
			char oldDirection;
			
			switch(direction) {
				case '^': y--; break;
				case 'v': y++; break;
				case '>': x++; break;
				case '<': x--; break;
			}
			updateCell(oldX, oldY, '.');
		}
	}

	bool checkCollision() {
		if (x >= WIDTH || x <= 0 || y >= HEIGHT	|| y <= 0) {
			isActive = false;
			return true;	// При условии выхода за пределы карты
		} 

		char cell = map[y][x];
		if (cell == '@' || cell == '^' || cell == 'v' || cell == '>' || cell == '<') {
			// updateCell(x, y, '*');
			isActive = false;
			return true;	// При условии попадания в стену или в танк
		} 

		return false;
	}

};

// Абстрактный класс для его наследования PlayerTank и EnemyTank
class Tank {
protected:	// Нужно для передачи данных полей дочерним классам	
	int x, y;
	char direction;
	bool isAlive;
	std::vector<Projectile> projectiles; // Вектор снарядов
  
public:
	
	// Конструктор
	Tank(int startX, int startY, char startDir, bool alive = true)
		: x(startX), y(startY), direction(startDir), isAlive(alive) {}

	// Геттеры
	int getX() const { return x; }
	int getY() const { return y; }
	char getDirection() const { return direction; }
	bool getIsAlive() const { return isAlive; }
	// Сеттеры
	void setDirection(char newDir) { direction = newDir; } 
	void setIsAlive(bool newIsAlive ) { isAlive = newIsAlive; }
	void setPosition(int newX, int newY) {
		x = newX;
		y = newY;
	} 

	// Методы класса

	// Обычные 

	// Реализация декремента/инкремента значения позиции
	// int incX() { return x++; }
	// int incY() { return y++; }

	// int decX() { return x--; }
	// int decY() { return y--; }

	// Метод для отрисовки танка
	void draw() const {
		updateCell(x, y, direction);
	}

	// Метод для отрисовки снаряда
	void drawProjectile(Projectile projectile) {
		projectile.draw();
	}
	// Метод для движения снаряда
	void moveProjectile(Projectile projectile) {
		projectile.move();
	}

	// Виртуальные методы

	virtual void moveTank() = 0;
	// virtual void fireProjectile() = 0;
	virtual void handleProjectile() = 0;
	// virtual void moveProjectile() {
	// 	projectile.move();
	// }
	//virtual void fireProjectile();

};

// Класс танка игрока

class PlayerTank : public Tank {
public:
	PlayerTank(int x, int y, char dir, bool alive) : Tank(x, y, dir, alive) {}

	void moveTank() override {
		int oldX = x;
		int oldY = y;
		char oldDirection;

		bool moveW = true;
		bool moveA = true;
		bool moveS = true;
		bool moveD = true;

		int dx = 1;
		int dy = 1;

		// Работает нормально за исключением застревания в боковых клетках, можно исправить с помощью булевой переменной
		// if (map[y - 1][x] == '#' || map[y - 1][x] == '@' || map[y - 1][x] == '^' 
		// 	|| map[y - 1][x] == 'v' || map[y - 1][x] == '>' || map[y - 1][x] == '<' ) moveW = false;
		// else if (map[y + 1][x] == '#' || map[y + 1][x] == '@' || map[y + 1][x] == '^' 
		// 	|| map[y + 1][x] == 'v' || map[y + 1][x] == '>' || map[y + 1][x] == '<') moveS = false;

		// if (map[y][x - 1] == '#' || map[y][x - 1] == '@' || map[y][x - 1] == '^' 
		// 	|| map[y][x - 1] == 'v' || map[y][x - 1] == '>' || map[y][x - 1] == '<') moveA = false;
		// else if (map[y][x + 1] == '#' || map[y][x + 1] == '@' || map[y][x + 1] == '^' 
		// 	|| map[y][x + 1] == 'v' || map[y][x + 1] == '>' || map[y][x + 1] == '<') moveD = false;

		if (map[y - 1][x] == '#' || map[y - 1][x] == '@') moveW = false;
		if (map[y + 1][x] == '#' || map[y + 1][x] == '@') moveS = false;

		if (map[y][x - 1] == '#' || map[y][x - 1] == '@') moveA = false;
		if (map[y][x + 1] == '#' || map[y][x + 1] == '@') moveD = false;
			
		switch(getPressedKey()) {
			case 'w': direction = '^'; if (moveW) y -= dy; break;
			case 'a': direction = '<'; if (moveA) x -= dx; break;
			case 's': direction = 'v'; if (moveS) y += dy; break;
			case 'd': direction = '>'; if (moveD) x += dx; break; 
		} 
		updateCell(oldX, oldY, '.'); 
	}

	void handleProjectile(std::vector<Projectile>& projectiles) {

		char key = getPressedKey();

		if (key == 'p') {
			projectiles.push_back(Projectile());
		}
		
		for (int i = 0; i <= projectiles.size(); i++) {
			Projectile proj = projectiles[i];
			if (projectiles[i].getIsActive()) {
				moveProjectile(proj);
				drawProjectile(proj);
			} else {
				projectiles.erase(projectiles.begin() + i);
			}
		}
	}

	// Методы для управления снарядом

	// void fireProjectile() override {
	// 	if (getPressedKey() == 'p') {
	// 		projectile = spawnProjectile();
	// 		projectile.setIsActive(true);
	// 		projectile.setDirection(direction);

	// 		switch(direction) {
	// 			case '^': projectile.setPosition(x, (y - 1)); break;
	// 			case 'v': projectile.setPosition(x, (y + 1)); break;
	// 			case '>': projectile.setPosition((x + 1), y); break;
	// 			case '<': projectile.setPosition((x - 1), y); break; 
	// 		}
	// 	}
	// } 
};

void initialDrawMap() {
	std::cout << "\033[H\033[J";
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			std::cout << map[y][x];
		}
		std::cout << std::endl;
	}
}

void handleInput() {
	char key = getPressedKey();
	if (key == 'e') {
		system("cls");
		exit(0);
	}
	delay(50);
}

// void updateGameLogic() {
// 	player->moveTank();
// 	player.projectile->move();
// }

int main() {
	hideCursor();
	startScreen();

	while (!isRunning) {
		runGame();
	}

	initialDrawMap();

	PlayerTank* player = new PlayerTank(8, 9, '^', true);

	while (isRunning) {
		handleInput();
		player->moveTank();
		player->draw();
		player->handleProjectile();

		// if (getPressedKey() == 'p') {
		// 	player->fireProjectile();
		// }

		// player->fireProjectile();
		// player->drawProjectile();

		// player->moveProjectile();	
		// player->drawProjectile();
	
		delay(200);
	}

	showCursor();

	system("cls");

	delete player;

	return 0;
}

