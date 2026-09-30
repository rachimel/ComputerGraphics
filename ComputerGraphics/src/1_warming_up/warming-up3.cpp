#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

constexpr int Capacity = 10;

struct Vertex
{
	int x{}, y{}, z{};
	bool isEmpty{ true };
};

std::array<Vertex, Capacity> buffer{};
// Distinguish slots freed by d from slots that have never been filled.
std::array<bool, Capacity> bottomVacancies{};
int size = 0;
bool distanceView = false;

long double DistanceSquared(const Vertex& a, const Vertex& b = Vertex{})
{
	// Convert BEFORE subtracting or multiplying to avoid integer overflow.
	const long double dx = static_cast<long double>(a.x) - b.x;
	const long double dy = static_cast<long double>(a.y) - b.y;
	const long double dz = static_cast<long double>(a.z) - b.z;
	return dx * dx + dy * dy + dz * dz;
}

void PrintVertex(const Vertex& v)
{
	std::cout << '(' << v.x << ", " << v.y << ", " << v.z << ')';
}

void PrintList()
{
	auto display = buffer;
	if (distanceView)
	{
		// Sort only the display copy so f can restore the original layout.
		std::stable_sort(display.begin(), display.end(),
			[](const Vertex& a, const Vertex& b)
			{
				if (a.isEmpty != b.isEmpty)
					return !a.isEmpty;
				if (a.isEmpty)
					return false;
				return DistanceSquared(a) < DistanceSquared(b);
			});
	}

	std::cout << "\n=== Point List";
	if (distanceView)
		std::cout << " (distance ascending from index 0)";
	std::cout << " ===\n";

	for (int i = Capacity - 1; i >= 0; --i)
	{
		std::cout << '[' << i << "] ";
		if (display[i].isEmpty)
			std::cout << "(empty)";
		else
		{
			PrintVertex(display[i]);
			if (distanceView)
				std::cout << "  distance = " << std::sqrt(DistanceSquared(display[i]));
		}
		std::cout << '\n';
	}
}

void PushTop(Vertex v)
{
	if (size == Capacity)
	{
		std::cout << "List is full.\n";
		return;
	}
	int target = -1;
	// Reuse bottom-deletion holes from index 0 upward.
	// Merely having an unused slot at 0 does NOT trigger this exception.
	if (!buffer[Capacity - 1].isEmpty)
	{
		for (int i = 0; i < Capacity; ++i)
		{
			if (buffer[i].isEmpty && bottomVacancies[i])
			{
				target = i;
				break;
			}
		}
	}
	// Normal insertion: 9, 8, 7, ...; skip occupied slots.
	if (target == -1)
	{
		for (int i = Capacity - 1; i >= 0; --i)
		{
			if (buffer[i].isEmpty)
			{
				target = i;
				break;
			}
		}
	}
	buffer[target] = v;
	bottomVacancies[target] = false;
	++size;
}

void PushBottom(Vertex v)
{
	if (size == Capacity)
	{
		std::cout << "List is full.\n";
		return;
	}
	// Insert at physical bottom (0), shifting upward to the first empty slot.
	int empty = 0;
	while (!buffer[empty].isEmpty)
		++empty;
	for (int i = empty; i > 0; --i)
	{
		buffer[i] = buffer[i - 1];
		bottomVacancies[i] = false;
	}
	buffer[0] = v;
	bottomVacancies[0] = false;
	++size;
}

void PopTop()
{
	if (size == 0)
	{
		std::cout << "List is empty.\n";
		return;
	}
	int top = Capacity - 1;
	while (buffer[top].isEmpty)
		--top;
	buffer[top] = Vertex{};
	bottomVacancies[top] = false;
	if (--size == 0)
		bottomVacancies = {};
}

void PopBottom()
{
	if (size == 0)
	{
		std::cout << "List is empty.\n";
		return;
	}
	int bottom = 0;
	while (buffer[bottom].isEmpty)
		++bottom;
	buffer[bottom] = Vertex{}; // Leave the deleted physical slot empty.
	bottomVacancies[bottom] = true;
	if (--size == 0)
		bottomVacancies = {};
}

void RotateDown()
{
	const auto old = buffer;
	const auto oldVacancies = bottomVacancies;
	for (int i = 0; i < Capacity; ++i)
	{
		buffer[(i + Capacity - 1) % Capacity] = old[i];
		bottomVacancies[(i + Capacity - 1) % Capacity] = oldVacancies[i];
	}
}

void ClearList()
{
	buffer = {};
	bottomVacancies = {};
	size = 0;
	distanceView = false;
}

void PrintPair(const char* label, int i, int j)
{
	std::cout << label << '[' << i << "] ";
	PrintVertex(buffer[i]);
	std::cout << " <-> [" << j << "] ";
	PrintVertex(buffer[j]);
	std::cout << "  distance = "
		<< std::sqrt(DistanceSquared(buffer[i], buffer[j])) << '\n';
}

void PrintAllDistances()
{
	if (size < 2)
	{
		std::cout << "At least two points are required.\n";
		return;
	}

	int nearestA = -1, nearestB = -1;
	int farthestA = -1, farthestB = -1;
	long double minDistance = 0, maxDistance = 0;

	for (int i = 0; i < Capacity; ++i)
	{
		if (buffer[i].isEmpty)
			continue;
		for (int j = i + 1; j < Capacity; ++j)
		{
			if (buffer[j].isEmpty)
				continue;
			const auto distance = DistanceSquared(buffer[i], buffer[j]);
			PrintPair("Pair: ", i, j);
			if (nearestA == -1 || distance < minDistance)
			{
				minDistance = distance;
				nearestA = i;
				nearestB = j;
			}
			if (farthestA == -1 || distance > maxDistance)
			{
				maxDistance = distance;
				farthestA = i;
				farthestB = j;
			}
		}
	}
	// In a tie, report the first pair found in physical index order.
	PrintPair("Nearest: ", nearestA, nearestB);
	PrintPair("Farthest: ", farthestA, farthestB);
}

int main()
{
	std::cout << std::fixed << std::setprecision(3);
	std::cout << "+ x y z : push top     - : pop top\n"
		<< "e x y z : push bottom  d : pop bottom\n"
		<< "a : count  b : rotate down  c : clear\n"
		<< "f : toggle distance view  g : pair distances  q : quit\n";
	PrintList();

	std::string line;
	while (std::cout << "\n> " && std::getline(std::cin, line))
	{
		std::istringstream input(line);
		char command{};
		Vertex v;
		bool valid = static_cast<bool>(input >> command);
		if (valid && (command == '+' || command == 'e'))
		{
			valid = static_cast<bool>(input >> v.x >> v.y >> v.z);
			v.isEmpty = false;
		}
		if (valid)
		{
			input >> std::ws;
			valid = input.eof();
		}
		if (!valid)
		{
			std::cout << "Invalid input. Enter a command and its exact arguments.\n";
			PrintList();
			continue;
		}

		switch (command)
		{
		case '+': PushTop(v); break;
		case '-': PopTop(); break;
		case 'e': PushBottom(v); break;
		case 'd': PopBottom(); break;
		case 'a': std::cout << "Point count: " << size << '\n'; break;
		case 'b': RotateDown(); break;
		case 'c': ClearList(); break;
		case 'f': distanceView = !distanceView; break;
		case 'g': PrintAllDistances(); break;
		case 'q': PrintList(); return 0;
		default: std::cout << "Unknown command.\n"; break;
		}
		PrintList();
	}
}
