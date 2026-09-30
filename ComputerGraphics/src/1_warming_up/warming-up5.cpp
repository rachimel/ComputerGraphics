#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

struct Vec2
{
	int x{ 0 };
	int y{ 0 };
};

Vec2 rect1Begin{}, rect1Diff{};
Vec2 rect2Begin{}, rect2Diff{};
Vec2 boardSize{ 30, 30 };
std::vector<std::vector<char>> board(40, std::vector<char>(40, '.'));

// All input uses getline: no leftover newline between commands and Reset.
// Returns false only if the input stream closes or fails.
bool InputRect(Vec2& begin, Vec2& diff)
{
	std::string line;
	while (std::getline(std::cin, line))
	{
		std::istringstream input(line);
		Vec2 first{}, last{};
		std::string extra;
		if (!(input >> first.x >> first.y >> last.x >> last.y) || (input >> extra))
		{
			std::cout << "[Error] Enter exactly four integers: x1 y1 x2 y2\n";
			continue;
		}
		if (first.x < 0 || first.x >= boardSize.x ||
			last.x < 0 || last.x >= boardSize.x ||
			first.y < 0 || first.y >= boardSize.y ||
			last.y < 0 || last.y >= boardSize.y)
		{
			std::cout << "[Error] Coordinates must be in range 0.."
				<< boardSize.x - 1 << " for x and 0.."
				<< boardSize.y - 1 << " for y.\n";
			continue;
		}
		begin = { std::min(first.x, last.x), std::min(first.y, last.y) };
		diff = { std::abs(first.x - last.x), std::abs(first.y - last.y) };
		return true;
	}
	return false;
}

bool Reset()
{
	boardSize = { 30, 30 };
	std::cout << "Enter Rect1 coordinates: x1 y1 x2 y2\n";
	if (!InputRect(rect1Begin, rect1Diff))
		return false;
	std::cout << "Enter Rect2 coordinates: x1 y1 x2 y2\n";
	return InputRect(rect2Begin, rect2Diff);
}

Vec2 VisibleDiff(const Vec2& diff)
{
	return { std::clamp(diff.x, 0, boardSize.x - 1),
			std::clamp(diff.y, 0, boardSize.y - 1) };
}

void DrawRect(const Vec2& begin, const Vec2& diff, char symbol)
{
	const Vec2 visible = VisibleDiff(diff);
	for (int y = 0; y <= visible.y; ++y)
	{
		for (int x = 0; x <= visible.x; ++x)
		{
			char& cell = board[(begin.y + y) % boardSize.y]
				[(begin.x + x) % boardSize.x];
			cell = (symbol == 'X' && cell == 'O') ? '#' : symbol;
		}
	}
}

void BuildBoard()
{
	for (auto& row : board)
		std::fill(row.begin(), row.end(), '.');
	DrawRect(rect1Begin, rect1Diff, 'O');
	DrawRect(rect2Begin, rect2Diff, 'X');
}

void Render(const std::string& message)
{
#ifdef _WIN32
	std::system("cls");
	const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	const bool colorAvailable = GetConsoleScreenBufferInfo(output, &info) != 0;
#endif
	BuildBoard();
	std::cout << "Board: " << boardSize.x << " x " << boardSize.y
		<< " | Rect1: O | Rect2: X | Overlap: #\n";
	for (int y = 0; y < boardSize.y; ++y)
	{
		for (int x = 0; x < boardSize.x; ++x)
		{
			const char cell = board[y][x];
#ifdef _WIN32
			if (cell == '#' && colorAvailable)
			{
				std::cout.flush();
				SetConsoleTextAttribute(output, 11);
				std::cout << cell << ' ' << std::flush;
				SetConsoleTextAttribute(output, info.wAttributes);
			}
			else
				std::cout << cell << ' ';
#else
			if (cell == '#')
				std::cout << "\033[96m# \033[0m";
			else
				std::cout << cell << ' ';
#endif
		}
		std::cout << '\n';
	}
	std::cout << "Rect1: x/X right/left, y/Y down/up, s/S shrink/grow both\n"
		<< "       i/I grow/shrink width, j/J grow/shrink height\n"
		<< "       a/A width+/height- or width-/height+, b area\n"
		<< "Rect2: z/Z right/left, w/W down/up, p/P shrink/grow both\n"
		<< "       k/K grow/shrink width, l/L grow/shrink height\n"
		<< "       e/E width+/height- or width-/height+, B area\n"
		<< "Board: c grow, d shrink | r reset | q quit\n";
	if (!message.empty())
		std::cout << message << '\n';
}

void Move(Vec2& begin, int dx, int dy)
{
	begin.x = (begin.x + dx + boardSize.x) % boardSize.x;
	begin.y = (begin.y + dy + boardSize.y) % boardSize.y;
}

void ResizeAxis(int& diff, int change, int boardLength)
{
	// Preserve original dimensions when the board shrinks.
	// A shrink command subtracts one even if the original exceeds the board.
	if (change > 0 && diff < boardLength - 1)
		++diff;
	else if (change < 0 && diff > 0)
		--diff;
}

void ResizeRect(Vec2& diff, int dx, int dy)
{
	ResizeAxis(diff.x, dx, boardSize.x);
	ResizeAxis(diff.y, dy, boardSize.y);
}

std::string AreaMessage(const char* name, const Vec2& diff)
{
	const Vec2 visible = VisibleDiff(diff);
	return std::string(name) + " original area: " +
		std::to_string((diff.x + 1) * (diff.y + 1)) +
		" | displayed cells: " +
		std::to_string((visible.x + 1) * (visible.y + 1));
}

int main()
{
	if (!Reset())
		return 0;
	std::string line, message;
	while (true)
	{
		Render(message);
		message.clear();
		std::cout << "> ";
		if (!std::getline(std::cin, line))
			break;
		std::istringstream input(line);
		char command{};
		std::string extra;
		if (!(input >> command) || (input >> extra))
		{
			message = "[Error] Enter one command character.";
			continue;
		}
		switch (command)
		{
		case 'x': Move(rect1Begin, 1, 0); break;
		case 'X': Move(rect1Begin, -1, 0); break;
		case 'y': Move(rect1Begin, 0, 1); break;
		case 'Y': Move(rect1Begin, 0, -1); break;
		case 'z': Move(rect2Begin, 1, 0); break;
		case 'Z': Move(rect2Begin, -1, 0); break;
		case 'w': Move(rect2Begin, 0, 1); break;
		case 'W': Move(rect2Begin, 0, -1); break;
		case 's': ResizeRect(rect1Diff, -1, -1); break;
		case 'S': ResizeRect(rect1Diff, 1, 1); break;
		case 'p': ResizeRect(rect2Diff, -1, -1); break;
		case 'P': ResizeRect(rect2Diff, 1, 1); break;
		case 'i': ResizeRect(rect1Diff, 1, 0); break;
		case 'I': ResizeRect(rect1Diff, -1, 0); break;
		case 'j': ResizeRect(rect1Diff, 0, 1); break;
		case 'J': ResizeRect(rect1Diff, 0, -1); break;
		case 'k': ResizeRect(rect2Diff, 1, 0); break;
		case 'K': ResizeRect(rect2Diff, -1, 0); break;
		case 'l': ResizeRect(rect2Diff, 0, 1); break;
		case 'L': ResizeRect(rect2Diff, 0, -1); break;
		case 'a': ResizeRect(rect1Diff, 1, -1); break;
		case 'A': ResizeRect(rect1Diff, -1, 1); break;
		case 'e': ResizeRect(rect2Diff, 1, -1); break;
		case 'E': ResizeRect(rect2Diff, -1, 1); break;
		case 'b': message = AreaMessage("Rect1", rect1Diff); break;
		case 'B': message = AreaMessage("Rect2", rect2Diff); break;
		case 'c':
			if (boardSize.x < 40)
				boardSize = { boardSize.x + 1, boardSize.y + 1 };
			else
				message = "Maximum board size is 40 x 40.";
			break;
		case 'd':
			if (boardSize.x > 10)
			{
				boardSize = { boardSize.x - 1, boardSize.y - 1 };
				rect1Begin.x %= boardSize.x;
				rect1Begin.y %= boardSize.y;
				rect2Begin.x %= boardSize.x;
				rect2Begin.y %= boardSize.y;
			}
			else
				message = "Minimum board size is 10 x 10.";
			break;
		case 'r':
			if (!Reset())
				return 0;
			break;
		case 'q': return 0;
		default: message = "Unknown command."; break;
		}
	}
	return 0;
}
