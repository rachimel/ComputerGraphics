#include <print>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <ranges>

#define NOMINMAX
#include <windows.h>
// global toggles
bool capsSwapToggle{ false };
bool printWordCount{ false };
bool capsColorSwapToggle{ false };
bool reverseWordToggle{ false };
bool reverseSentenceToggle{ false };
bool blankAsterToggle{ false };
bool charSwapToggle{ false };
bool pushNumNextLineToggle{ false };
bool strCompareToggle{ false };

char swapTarget{}, swapInto{};
std::string findTarget{};

size_t printOffset{0};

bool EqualIgnoreCase(std::string_view a, std::string_view b)
{
	return std::ranges::equal(a, b, [](unsigned char lhs, unsigned char rhs) {
		return std::tolower(lhs) == std::tolower(rhs);
		});
}


struct Word
{
	std::string data{};
	int blank{};

	std::string FilterString() const
	{
		std::string buf{ data };
		if (reverseWordToggle)
		{
			std::ranges::reverse(buf);
		}

		for (int i = 0; i < buf.size(); ++i)
		{
			if (charSwapToggle)
			{
				if (buf[i] == swapTarget)
				{
					buf[i] = swapInto;
				}
			}

			if (capsSwapToggle)
			{
				if (std::islower(buf[i]))
				{
					buf[i] = std::toupper(buf[i]);
				}
				else if (std::isupper(buf[i]))
				{
					buf[i] = std::tolower(buf[i]);
				}
			}

			if(pushNumNextLineToggle && std::isdigit(buf[i]))
			{
				buf.insert(i + 1, 1, '\n');
				++i; // 방금 삽입한 개행 건너뛰기
			}
		}
		return buf;
	}


	void Print() const
	{
		const bool startsWithUpper{
			!data.empty() && 'A' <= data.front() && data.front() <= 'Z'
		};
		if (capsColorSwapToggle && startsWithUpper)
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
		}
		if (strCompareToggle && EqualIgnoreCase(data, findTarget))
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
		}
		std::print("{}", data);

		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	}
};

std::vector<Word> ParseLine(const std::string& line)
{
	std::vector<Word> ret;
	int blanks{ 0 };
	bool blankMode{ false };
	std::string data;
	for(const char c : line)
	{
		if (!blankMode && c == ' ')
		{
			blankMode = true;
		}
		else if (blankMode && c != ' ')
		{
			blankMode = false;
			if(!data.empty())
			{
				ret.push_back(Word{ data, blanks });
			}
			data = std::string{};
			blanks = 0;
		}

		if (blankMode)
		{
			blanks++;
		}
		else
		{
			data += c;
		}
	}
	// 마지막 단어 추가
	if (!data.empty())
	{
		ret.push_back(Word{ data, blanks });
	}
	return ret;
}
 
int CountCaps(const std::vector<std::vector<Word>>& lines)
{
	int capsCount{ 0 };
	for (const auto& line : lines)
	{
		for (const auto& word : line)
		{
			if (!word.data.empty() && std::isupper(word.data.front()))
			{
				++capsCount;
			}
		}
	}
	return capsCount;
}

std::vector<std::vector<Word>> CreateDisplayLines(const std::vector<std::vector<Word>>& lines)
{
	std::vector<std::vector<Word>> splitLines{};
	for (const auto& line : lines)
	{
		auto lineBuf = line;
		if (reverseSentenceToggle)
		{
			std::ranges::reverse(lineBuf);
		}
		std::vector<Word> newLine{};

		for (size_t wordIndex = 0; wordIndex < lineBuf.size(); ++wordIndex)
		{
			const auto& word = lineBuf[wordIndex];

			int outputBlank{ word.blank };

			if (reverseSentenceToggle)
			{
				outputBlank =
					wordIndex + 1 < lineBuf.size()
					? lineBuf[wordIndex + 1].blank
					: 0;
			}

			std::string filteredString = word.FilterString();
			std::string appendData{};
			for (int i = 0; i < filteredString.size(); i++)
			{
				if (filteredString[i] == '\n')
				{
					if (!appendData.empty())
					{
						newLine.push_back(Word{ appendData, 0 });
					}

					splitLines.push_back(newLine);
					newLine.clear();
					appendData.clear();
				}
				else
				{
					appendData += filteredString[i];
				}
			}

			if (!appendData.empty())
			{
				newLine.push_back(Word{ appendData, outputBlank});
			}
		}
		if (!newLine.empty())
		{
			splitLines.push_back(newLine);
		}
	}

	return splitLines;
}

int CountCompareHit(const std::vector<std::vector<Word>>& lines)
{
	int compareHitCount{ 0 };
	for (const auto& line : lines)
	{
		for (const auto& word : line)
		{
			if (!word.data.empty() && EqualIgnoreCase(word.data, findTarget))
			{
				++compareHitCount;
			}
		}
	}
	return compareHitCount;
}


void Print(const std::vector<std::vector<Word>>& lines)
{
	auto displayLines = CreateDisplayLines(lines);

	if (!displayLines.empty())
	{
		printOffset %= displayLines.size();
		if (printOffset > 0)
		{
			std::ranges::rotate(displayLines, displayLines.end() - printOffset);
		}
	}
	for (const auto& line : displayLines)
	{
		for (size_t i = 0; i < line.size(); ++i)
		{
			line[i].Print();

			int blankCount{ line[i].blank };

			if (blankAsterToggle && blankCount > 0)
			{
				std::print("*");
				continue;
			}
			for (int j = 0; j < blankCount; ++j)
			{
				std::print(" ");
			}
		}
		if (printWordCount)
		{
			std::println("\t Word Count : {}", line.size());
		}
		else
		{
			std::println();
		}
	}
	if (capsColorSwapToggle)
	{
		std::println("Words start with Caps : {} words", CountCaps(displayLines));
	}
	if (strCompareToggle)
	{
		std::println("Matching word count with \"{}\" : {}", findTarget, CountCompareHit(displayLines));
		strCompareToggle = false;
	}
}

int main()
{
	std::string file{};
	std::println("Enter the file!");
	std::getline(std::cin, file);
	auto path = std::filesystem::current_path() / "data\\warming-up" / file;
	std::ifstream open{path.string()};
	
	if (!open)
	{
		std::println("[Error] Failed to open file {}", path.string());
		return -1;
	}
	// Read multiple lines
	std::vector<std::vector<Word>> lines;
	std::string line;
	std::string inputBuffer;
	while (std::getline(open, line))
	{
		lines.push_back(ParseLine(line));
	}
	
	char command{};
	while (command != 'q')
	{
		Print(lines);
		std::cin >> command;
		if (std::cin.fail())
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::println("Unknown Command!");
			continue;
		}
		switch (command)
		{
		case 'a':
			capsSwapToggle = !capsSwapToggle;
			break;
		case 'b':
			printWordCount = !printWordCount;
			break;
		case 'c':
			capsColorSwapToggle = !capsColorSwapToggle;
			break;
		case 'd':
			reverseSentenceToggle = !reverseSentenceToggle;
			break;
		case 'e':
			blankAsterToggle = !blankAsterToggle;
			break;
		case 'f':
			reverseWordToggle = !reverseWordToggle;
			break;
		case 'g':
		{
			if(!charSwapToggle)
			{
				std::println("Input two characters : %SwapTarget %SwapInto");
				std::cin >> swapTarget >> swapInto;
				charSwapToggle = true;
			}
			else
			{
				charSwapToggle = false;
			}
			break;
		}
		case 'h':
			pushNumNextLineToggle = !pushNumNextLineToggle;
			break;
		case 'i':
		{
			std::cin >> findTarget;
			if (std::cin.fail())
			{
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::println("Invalid Word!");
				break;
			}
			strCompareToggle = true;
			break;
		}
		case 'j':
			++printOffset;
			break;
		}
	}
}