#include <iostream>		// Библиотека стандартного ввода/вывода данных
#include <vector>		// Библиотека динамических массивов
#include <chrono>		// Библиотека для работы с временем (реализация задержек респавна и задержек)
#include <thread>		// Библиотека для работы с многопоточностью (в данном случае используется для создания задержек между кадрами)
#include <windows.h>	// Библиотека для чтения ввода пользователя с клавиатуры
#include "RandomNum.h"  // Кастомная библиотека для генерации случайных чисел

// Глобальные переменные 
const int WIDTH = 23;	// + 1 из-за невидимого символа окончания строки
const int HEIGHT = 13; 
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
	"#...@@@@.^....@@@@...#",
	"#....@@........@@....#",
	"#...@@@@......@@@@...#",
	"#....................#",
	"######################"
}; 

// Вспомогательные функции (изменение цвета консольного шрифта, задержки и т.д)

void setColor(int color) {      // Функция для установки цвета в командной строке
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
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

// "\033[H\033[J" for screen cleaning;
void updateCell(int x, int y, char newSymbol) {
	std::cout << "\033[" << (y + 1) << ";" << (x + 1) << "H" << newSymbol;
}

void delay(int milliseconds) {
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

char getPressedKey() {
	// Кнопки для выбора направления движения танка
	if (GetAsyncKeyState('W') & 0x8000) return 'w';
	if (GetAsyncKeyState('A') & 0x8000) return 'a';
	if (GetAsyncKeyState('S') & 0x8000) return 's';
	if (GetAsyncKeyState('D') & 0x8000) return 'd';
	// Кнопка для выстрела снарядом
	if (GetAsyncKeyState(' ') & 0x8000) return 'p';
	// Кнопка для выхода из игры
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) return 'e';

	return '\0';	// Если ни одна кнопка не была нажата, возвращается пустой символ (терминарный ноль)
}

class Projectile {
private:
	int x, y;
	char direction;
	bool isActive;

public: 

	// Projectile(int startX, int startY, char startDir, bool active = true)
	// 	: x(startX), y(startY), direction(startDir), isActive(active) {}

	Projectile() : x(0), y(0), direction('^'), isActive(false) {}

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

	void move() {
		switch(direction) {
			case '^': y--; break;
			case 'v': y++; break;
			case '>': x++; break;
			case '<': x--; break;
		}
	}

	void collision() {
		if ()
	}

};

// Абстрактный класс для его наследования PlayerTank и EnemyTank
class Tank {
protected:	// Нужно для передачи данных полей дочерним классам	
	int x, y;
	char direction;
	bool isAlive;
	Projectile projectile;
  
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
	int incX() { return x++; }
	int incY() { return y++; }

	int decX() { return x--; }
	int decY() { return y--; }

	// Методы для управления снарядом
	void fireProjectile() {
		projectile.setIsActive(true);
		projectile.setDirection(direction);
		
		switch(direction) {
			case '^': projectile.setPosition(x, (y - 1)); break;
			case 'v': projectile.setPosition(x, (y + 1)); break;
			case '>': projectile.setPosition((x + 1), y); break;
			case '<': projectile.setPosition((x - 1), y); break; 
		}
	}

	// Виртуальные

	virtual void moveTank();

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

int main() {
	hideCursor();
	startScreen();

	initialDrawMap();
	while (true) {
		// drawMap();
		delay(500);
		updateCell(13, 5, '%');
		delay(500);
		updateCell(14, 6, '%');
		delay(500);
	}

	showCursor();

	system("cls");

	return 0;
}

