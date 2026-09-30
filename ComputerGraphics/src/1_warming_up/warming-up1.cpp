#include <print>
#include <vector>
#include <iostream>
#include <random>

std::random_device rd;
std::default_random_engine dre(rd());
std::uniform_int_distribution<int> uid{ 0,9 };
struct Mat4
{
public:
	// 랜덤으로 설정
	Mat4()
		: data{}
	{
	}

	void Reset()
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				data[i][j] = uid(dre);
			}
		}
	}
public:
	int data[4][4];
};

Mat4 MatMinFilter(const Mat4& a)
{
	Mat4 ret;
	for (int i = 0; i < 4; ++i)
	{
		int min{ std::numeric_limits<int>::max() };
		for (int j = 0; j < 4; ++j)
		{
			if (a.data[i][j] < min)
			{
				min = a.data[i][j];
			}
		}
		// apply min
		for (int j = 0; j < 4; ++j)
		{
			ret.data[i][j] = a.data[i][j] - min;
		}
	}
	return ret;
}

Mat4 MatMaxFilter(const Mat4& a)
{
	Mat4 ret;
	for (int j = 0; j < 4; ++j)
	{
		int max{ std::numeric_limits<int>::min() };
		for (int i = 0; i < 4; ++i)
		{
			if (a.data[i][j] > max)
			{
				max = a.data[i][j];
			}
		}
		// apply max
		for (int i = 0; i < 4; ++i)
		{
			ret.data[i][j] = a.data[i][j] + max;
		}
	}
	return ret;
}
Mat4 Multiply(Mat4& a, Mat4& b, bool minFilter, bool maxFilter)
{
	Mat4 ret{};
	Mat4 tempA{a}, tempB{b};
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
		tempB = MatMinFilter(tempB);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
		tempB = MatMaxFilter(tempB);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			for (int k = 0; k < 4; ++k)
			{
				ret.data[i][j] += tempA.data[i][k] * tempB.data[k][j];
			}
		}
	}
	return ret;
}
void PrintMatPair(const Mat4& a, const Mat4& b, bool minFilter, bool maxFilter)
{
	Mat4 tempA{ a }, tempB{ b };
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
		tempB = MatMinFilter(tempB);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
		tempB = MatMaxFilter(tempB);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			std::print("{} ", tempA.data[i][j]);
		}
		std::print("\t");
		for (int j = 0; j < 4; ++j)
		{
			std::print("{} ", tempB.data[i][j]);
		}
		std::print("\n");
	}
	std::print("\n");
}

void PrintMatTuple(const Mat4& a, const Mat4& b, const Mat4& res, bool minFilter, bool maxFilter)
{
	Mat4 tempA{ a }, tempB{ b };
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
		tempB = MatMinFilter(tempB);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
		tempB = MatMaxFilter(tempB);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			std::print("{} ", tempA.data[i][j]);
		}
		std::print("\t");
		for (int j = 0; j < 4; ++j)
		{
			std::print("{} ", tempB.data[i][j]);
		}

		if (i == 2)
		{
			std::print("=");
		}
		std::print("\t");
		for (int j = 0; j < 4; ++j)
		{
			std::print("{} ", res.data[i][j]);
		}
		std::print("\n");
	}
	std::print("\n");
}

Mat4 Add(const Mat4& a, const Mat4& b, bool minFilter, bool maxFilter)
{
	Mat4 ret{};
	Mat4 tempA{ a }, tempB{ b };
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
		tempB = MatMinFilter(tempB);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
		tempB = MatMaxFilter(tempB);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.data[i][j] = tempA.data[i][j] + tempB.data[i][j];
		}
	}
	return ret;
}

Mat4 Sub(const Mat4& a, const Mat4& b, bool minFilter, bool maxFilter)
{
	Mat4 ret{};
	Mat4 tempA{ a }, tempB{ b };
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
		tempB = MatMinFilter(tempB);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
		tempB = MatMaxFilter(tempB);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.data[i][j] = tempA.data[i][j] - tempB.data[i][j];
		}
	}
	return ret;
}

// Calculate Determinant by Row
int Det(int size, const Mat4& mat)
{
	if (size == 2)
	{
		return mat.data[0][0] * mat.data[1][1] - mat.data[1][0] * mat.data[0][1];
	}

	int det{};
	for (int i = 0; i < size; ++i)
	{
		// 0 Row에 대한 행렬식을 계산하니까 1부터 계산
		// Col
		// Get minor matrix
		Mat4 minor{};
		int cnt{};
		for (int col = 1; col < size; ++col)
		{
			for (int row = 0; row < size; ++row)
			{
				if (row == i)
					continue;
				minor.data[cnt % (size - 1)][cnt / (size - 1)] = mat.data[col][row];
				cnt++;
			}
		}
		int cofactor{ (i % 2 == 0) ? 1 : -1 };
		det += cofactor * mat.data[0][i] * Det(size - 1, minor);
	}
	return det;
}

Mat4 Transpose(const Mat4& a)
{
	Mat4 ret{};
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.data[i][j] = a.data[j][i];
		}
	}
	return ret;
}

int mod_add(int a, int b, int m)
{
	a %= m;
	b %= m;
	
	if (a >= m - b)
	{
		return a - (m - b);
	}
	return (a + b) % m;
}

int mod_sub(int a, int b, int m)
{
	a %= m;
	b %= m;
	if (a >= b)
	{
		return (a - b) % m;
	}
	return m - (b - a) % m;
}
Mat4 Inc(const Mat4& a, bool minFilter, bool maxFilter)
{
	Mat4 ret;
	Mat4 tempA{ a };
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.data[i][j] = mod_add(tempA.data[i][j], 1, 10);
		}
	}
	return ret;
}

Mat4 Dec(const Mat4& a, bool minFilter, bool maxFilter)
{
	Mat4 ret;
	Mat4 tempA{ a };
	if (minFilter)
	{
		tempA = MatMinFilter(tempA);
	}
	if (maxFilter)
	{
		tempA = MatMaxFilter(tempA);
	}
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			ret.data[i][j] = mod_sub(tempA.data[i][j], 1, 10);
		}
	}
	return ret;
}
int main()
{
	bool MinMod{ false };
	bool MaxMod{ false };
	char cmd{};
	Mat4 a{}, b{};
	a.Reset();
	b.Reset();

	while (cmd != 'q')
	{
		// print matrix
		PrintMatPair(a, b, MinMod, MaxMod);
		std::cin >> cmd;
		if (std::cin.fail())
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			std::println("Invalid Command!");
			continue;
		}
		switch (cmd)
		{
		case 'm':
		{
			Mat4 res = Multiply(a, b, MinMod, MaxMod);
			PrintMatTuple(a, b, res, MinMod, MaxMod);
			break;
		}
		case 'a':
		{
			Mat4 res = Add(a, b, MinMod, MaxMod);
			PrintMatTuple(a, b, res, MinMod, MaxMod);
			break;
		}
		case 'd':
		{
			Mat4 res = Sub(a, b, MinMod, MaxMod);
			PrintMatTuple(a, b, res, MinMod, MaxMod);
			break;
		}
		case 'r':
		{
			Mat4 tempA{ a }, tempB{ b };
			if (MinMod)
			{
				tempA = MatMinFilter(tempA);
				tempB = MatMinFilter(tempB);
			}
			if (MaxMod)
			{
				tempA = MatMaxFilter(tempA);
				tempB = MatMaxFilter(tempB);
			}
			int detA{ Det(4, tempA) }, detB{ Det(4, tempB) };
			std::println("det(A) : {}, det(B) : {}", detA, detB);
			break;
		}
		case 't':
		{
			Mat4 tempA{ a }, tempB{ b };
			if (MinMod)
			{
				tempA = MatMinFilter(tempA);
				tempB = MatMinFilter(tempB);
			}
			if (MaxMod)
			{
				tempA = MatMaxFilter(tempA);
				tempB = MatMaxFilter(tempB);
			}
			Mat4 aT{ Transpose(tempA) }, bT{ Transpose(tempB) };
			int detAT{ Det(4, aT) }, detBT{ Det(4, bT) };
			PrintMatPair(aT, bT, MinMod, MaxMod);
			std::println("det(A^T) : {}, det(B^T) : {}", detAT, detBT);
			break;
		}
		case 'e':
		{
			if (MaxMod && !MinMod) MaxMod = false;
			MinMod = !MinMod;
			break;
		}
		case 'f':
		{
			if (MinMod && !MaxMod) MinMod = false;
			MaxMod = !MaxMod;
			break;
		}
		case '+':
		{
			a = Inc(a, MinMod, MaxMod);
			b = Inc(b, MinMod, MaxMod);
			PrintMatPair(a, b, MinMod, MaxMod);
			break;
		}
		case '-':
		{
			a = Dec(a, MinMod, MaxMod);
			b = Dec(b, MinMod, MaxMod);
			PrintMatPair(a, b, MinMod, MaxMod);
			break;
		}
		case 's':
		{
			a.Reset();
			b.Reset();
			break;
		}

		}
	}


}