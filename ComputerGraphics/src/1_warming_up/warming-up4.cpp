#include <iostream>
#include <print>

#include <cctype>
#include <cstdlib>

#include <sstream>
#include <chrono>
#include <random>

#define NOMINMAX
#include <Windows.h>

#include <array>
#include <vector>
#include <deque>

#include <algorithm>


std::random_device rd;
std::default_random_engine dre{rd()};
std::string buffer{};

std::vector<char> array{};
std::deque<bool> check{};
bool jokerFound{false};
bool forceOpen{ false };
int width, height;
int score{}, hitPairsWithoutJoker{};
int availableHints{};
int availableMoves{};

std::array<int, 14> colorTable{ 1,2,3,4,5,6,8,9,10,11,12,13,14,15 };
void Init()
{
	jokerFound = false;
	hitPairsWithoutJoker = 0;
	forceOpen = false;
	score = 0;
	array.clear();

	std::println("Enter the width and height");
	while (true)
	{
		std::getline(std::cin, buffer);
		std::istringstream iss{ buffer };
		iss >> width >> height;
		if (iss.fail() || (width > 6 || width < 3) || ((height > 6 || height < 3)))
		{
			std::println("Invalid Width and Height! (3 ~ 6)");
			iss.clear();
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < (width * height) / 2; ++i)
	{
		array.push_back('a' + i);
		array.push_back('a' + i);
	}
	check = std::deque<bool>(width * height, false);
	// place joker
	if ((width * height) % 2 == 1)
	{
		array.push_back('@'); 
	}
	availableMoves = 2 * (width * height);
	availableHints = std::max(width, height);

	std::shuffle(array.begin(), array.end(),dre);
	buffer.clear();
}

void Render()
{
	/*std::print("\x1b[2J\x1b[H");
	std::fflush(stdout);*/
	std::system("cls");
	std::print(" \t");
	for (int x = 0; x < width; ++x)
	{
		std::print("{} ", static_cast<char>('a' + x));
	}
	std::println();
	for (int y = 0; y < height; ++y)
	{
		std::print("{}\t", y + 1);
		for (int x = 0; x < width; ++x)
		{
			if(check[y * width + x] || forceOpen)
			{
				if (array[y * width + x] == '@')
				{
					std::print("@ ");
					continue;
				}
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colorTable[std::toupper(array[y * width + x]) % 14]);
				std::print("{} ", array[y * width + x]);
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			}
			else
			{
				std::print("* ");
			}
		}
		std::println();
	}
	std::println("Current Score : {}, Moves Left : {}, Hints : {}", score, availableMoves, availableHints);
}

int main()
{
	Init();
	char command{};
	char colA{}, colB{};
	int locA{}, locB{};
	while (command != 'q')
	{
		Render();
		std::getline(std::cin, buffer);
		std::istringstream iss{ buffer };
		// Parse location
		iss >> colA >> locA >> colB >> locB;
		if (iss.fail())
		{
			iss.clear();
			if (buffer.size() > 1)
			{
				std::println("Unknown Command!");
				buffer.clear();
				std::system("timeout -1");
				continue;
			}
			iss.str(buffer);
			// Parse command
			iss >> command;
			if (iss.fail())
			{
				std::println("Unknown Command!");
				iss.clear();
				buffer.clear();
				std::system("timeout -1");
				continue;
			}
			switch (command)
			{
			case 'r':
				Init();
				break;
			case 'h':
			{
				if (availableHints == 0)
				{
					std::println("No more hints...");
					std::system("timeout -1");
					break;
				}
				--availableHints;
				forceOpen = true;
				Render();
				system("timeout /t 3 /nobreak");
				forceOpen = false;
				break;
			}
			case 'q':
				break;
			default:
				std::println("Invalid Command!");
				std::system("timeout -1");
				break;
			}
		}
		else
		{
			bool isInvalidLocation{ !((colA >= 'a' && colA < ('a' + width)) && (colB >= 'a' && colB < ('a' + width))
				&& (locA > 0 && locA <= height) && (locB > 0 && locB <= height))};
			if (isInvalidLocation)
			{
				std::println("Invalid location!");
				std::system("timeout -1");
				continue;
			}
			bool isDuplicated{ colA == colB && locA == locB };
			if (isDuplicated)
			{
				std::println("Duplicated location!");
				std::system("timeout -1");
				continue;
			}

			int aIndex{ (locA - 1)* width + (colA - 'a') }, bIndex{ (locB - 1) * width + (colB - 'a') };
			bool isAlreadyFound{check[aIndex] || check[bIndex]};
			if (isAlreadyFound)
			{
				std::println("Location is already found");
				std::system("timeout -1");
				continue;
			}
			--availableMoves;
			check[aIndex] = true;
			check[bIndex] = true;
			Render();
			if (array[aIndex] == array[bIndex])
			{
				array[aIndex] = std::toupper(array[aIndex]);
				array[bIndex] = std::toupper(array[bIndex]);
				++score;
				++hitPairsWithoutJoker;
				std::println("Hit!");
			}
			// TODO : Implement Wildcard logic
			else if (!jokerFound && (array[aIndex] == '@' || array[bIndex] == '@'))
			{
				jokerFound = true;
				int nonJokerIdx{ (array[aIndex] == '@') ? bIndex : aIndex };
				char findChar{ array[nonJokerIdx] };
				int i;
				for (i = 0; i < width * height; ++i)
				{
					if (i == nonJokerIdx || array[i] != findChar)
						continue;
					break;
				}
				check[i] = true;
				array[nonJokerIdx] = std::toupper(array[nonJokerIdx]);
				array[i] = std::toupper(array[i]);
				++score;
				std::println("Hit! (Joker)");
			}
			else
			{
				check[aIndex] = false;
				check[bIndex] = false;
				std::println("Miss");
				--score;
			}
			std::system("timeout -1");
			bool gameClear{(hitPairsWithoutJoker == (width * height) / 2 && jokerFound == false)||
				std::all_of(check.begin(), check.end(), [](const bool a) { return a;}) };
			if (gameClear)
			{
				// 게임 끝 조건 우선
				Render();
				std::println("[Game Clear] \n Base Score : {}\n Available Moves Bonus : {}, Hint Bonus : {} Final Score : {} + {} + {} = {}", 
					score, availableMoves, 2 * availableHints, score,availableMoves, 2* availableHints, score + availableMoves + availableHints * 2);
				std::system("timeout -1");
				command = 'q';
			}
			else if (availableMoves == 0)
			{
				Render();
				std::println("[Game Over] \n Base Score : {}\n Hint Bonus : {} Final Score : {} + {} = {}",
					score, 2 * availableHints, score, 2 * availableHints, score + availableHints * 2);
				std::system("timeout -1");
				command = 'q';
			}
		}
	}
}