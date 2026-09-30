#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <filesystem>

struct Vertex
{
	double x{}, y{}, z{};
};

struct TexCoord
{
	double s{}, t{};
};

struct FaceIndex
{
	int vertex{};  // Keep the file's 1-based index.
	int texture{}; // 0 means no texture index was supplied.
};

struct Triangle
{
	std::array<FaceIndex, 3> corners{};
	std::size_t sourceLine{};
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<TexCoord> texCoords;
	std::vector<Triangle> triangles;
};

[[noreturn]] void ParseError(std::size_t line, const std::string& message)
{
	throw std::runtime_error("Line " + std::to_string(line) + ": " + message);
}

bool InRange(double value, double minimum, double maximum)
{
	return std::isfinite(value) && value >= minimum && value <= maximum;
}

bool SameVertex(const Vertex& a, const Vertex& b)
{
	// Compare stored coordinate values; keep all original indices unchanged.
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool Collinear(const Vertex& a, const Vertex& b, const Vertex& c)
{
	const long double ux = static_cast<long double>(b.x) - a.x;
	const long double uy = static_cast<long double>(b.y) - a.y;
	const long double uz = static_cast<long double>(b.z) - a.z;
	const long double vx = static_cast<long double>(c.x) - a.x;
	const long double vy = static_cast<long double>(c.y) - a.y;
	const long double vz = static_cast<long double>(c.z) - a.z;
	return uy * vz - uz * vy == 0 &&
		uz * vx - ux * vz == 0 &&
		ux * vy - uy * vx == 0;
}

void RequireEnd(std::istringstream& input, std::size_t line)
{
	std::string extra;
	if (input >> extra)
		ParseError(line, "Unexpected extra value: " + extra);
}

int ParsePositiveIndex(const std::string& text, std::size_t line)
{
	int index{};
	const auto result = std::from_chars(text.data(), text.data() + text.size(), index);
	if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || index <= 0)
		ParseError(line, "Index must be a positive integer: " + text);
	return index;
}

FaceIndex ParseFaceIndex(const std::string& token, std::size_t line)
{
	const auto slash = token.find('/');
	if (slash == std::string::npos)
		return { ParsePositiveIndex(token, line), 0 };
	if (token.find('/', slash + 1) != std::string::npos)
		ParseError(line, "Expected vertex/texture; normals are not part of this assignment.");
	return { ParsePositiveIndex(token.substr(0, slash), line),
			ParsePositiveIndex(token.substr(slash + 1), line) };
}

Mesh ReadMesh(std::istream& source)
{
	Mesh mesh;
	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(source, line))
	{
		++lineNumber;
		// Accept a UTF-8 BOM at the beginning of a Windows text file.
		if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
			line.erase(0, 3);
		const auto comment = line.find('#');
		if (comment != std::string::npos)
			line.erase(comment);

		std::istringstream input(line);
		std::string type;
		if (!(input >> type))
			continue;

		if (type == "v")
		{
			Vertex v;
			if (!(input >> v.x >> v.y >> v.z))
				ParseError(lineNumber, "v requires three numeric values: x y z.");
			RequireEnd(input, lineNumber);
			if (!InRange(v.x, -1.0, 1.0) || !InRange(v.y, -1.0, 1.0) ||
				!InRange(v.z, -1.0, 1.0))
				ParseError(lineNumber, "Vertex coordinates must be within [-1, 1].");
			mesh.vertices.push_back(v);
		}
		else if (type == "vt")
		{
			TexCoord uv;
			if (!(input >> uv.s >> uv.t))
				ParseError(lineNumber, "vt requires two numeric values: s t.");
			RequireEnd(input, lineNumber);
			if (!InRange(uv.s, 0.0, 1.0) || !InRange(uv.t, 0.0, 1.0))
				ParseError(lineNumber, "Texture coordinates must be within [0, 1].");
			mesh.texCoords.push_back(uv);
		}
		else if (type == "f")
		{
			Triangle triangle;
			triangle.sourceLine = lineNumber;
			for (auto& corner : triangle.corners)
			{
				std::string token;
				if (!(input >> token))
					ParseError(lineNumber, "A triangle requires exactly three face entries.");
				corner = ParseFaceIndex(token, lineNumber);
			}
			RequireEnd(input, lineNumber);
			const bool textured = triangle.corners[0].texture != 0;
			for (const auto& corner : triangle.corners)
			{
				if ((corner.texture != 0) != textured)
					ParseError(lineNumber, "Use the same face format for all three corners.");
			}
			mesh.triangles.push_back(triangle);
		}
		else
			ParseError(lineNumber, "Unknown record: " + type + " (expected v, vt, or f).");
	}
	if (source.bad() || (source.fail() && !source.eof()))
		throw std::runtime_error("Failed while reading the input file.");

	// Validate against separate arrays: vertex and texture indices may differ.
	// Validating after reading also permits references to later records.
	for (const auto& triangle : mesh.triangles)
	{
		for (const auto& corner : triangle.corners)
		{
			if (static_cast<std::size_t>(corner.vertex) > mesh.vertices.size())
				ParseError(triangle.sourceLine,
					"Vertex index out of range: " + std::to_string(corner.vertex));
			if (static_cast<std::size_t>(corner.texture) > mesh.texCoords.size())
				ParseError(triangle.sourceLine,
					"Texture index out of range: " + std::to_string(corner.texture));
		}
		const int ia = triangle.corners[0].vertex;
		const int ib = triangle.corners[1].vertex;
		const int ic = triangle.corners[2].vertex;
		if (ia == ib || ib == ic || ic == ia)
			ParseError(triangle.sourceLine,
				"Triangle has repeated vertex indices; three distinct indices are required.");
		const auto& a = mesh.vertices[ia - 1];
		const auto& b = mesh.vertices[ib - 1];
		const auto& c = mesh.vertices[ic - 1];
		if (SameVertex(a, b) || SameVertex(b, c) || SameVertex(c, a))
			ParseError(triangle.sourceLine,
				"Triangle has duplicate vertex coordinates, even though its indices differ.");
		if (Collinear(a, b, c))
			ParseError(triangle.sourceLine,
				"Triangle vertices are collinear (zero area).");
	}
	return mesh;
}

void PrintMesh(const Mesh& mesh, std::ostream& output)
{
	output << std::fixed << std::setprecision(6);
	output << "Vertices: " << mesh.vertices.size()
		<< " | Texture coordinates: " << mesh.texCoords.size()
		<< " | Triangles: " << mesh.triangles.size() << '\n';
	for (std::size_t i = 0; i < mesh.triangles.size(); ++i)
	{
		const auto& triangle = mesh.triangles[i];
		output << "\nFace " << i + 1 << " ("
			<< triangle.corners[0].vertex << ", "
			<< triangle.corners[1].vertex << ", "
			<< triangle.corners[2].vertex << "): vertex";
		for (const auto& index : triangle.corners)
		{
			// File indices start at 1, while vector indices start at 0.
			const auto& v = mesh.vertices[index.vertex - 1];
			output << " (" << v.x << ", " << v.y << ", " << v.z << ')';
		}
		output << "\n                  texture";
		if (triangle.corners[0].texture != 0)
		{
			for (const auto& index : triangle.corners)
			{
				const auto& uv = mesh.texCoords[index.texture - 1];
				output << " (" << uv.s << ", " << uv.t << ')';
			}
		}
		else
			output << " none (no texture indices in this face)";
		output << '\n';
	}
	if (mesh.triangles.empty())
		output << "No triangles in this file.\n";

	bool duplicateFound = false;
	output << '\n';
	for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
	{
		for (std::size_t j = i + 1; j < mesh.vertices.size(); ++j)
		{
			if (SameVertex(mesh.vertices[i], mesh.vertices[j]))
			{
				duplicateFound = true;
				const auto& v = mesh.vertices[i];
				output << "Duplicate vertex value: v[" << i + 1 << "] and v["
					<< j + 1 << "] = (" << v.x << ", " << v.y << ", " << v.z << ")\n";
			}
		}
	}
	if (!duplicateFound)
		output << "No duplicate vertex value\n";
}

const char* SlideExample = R"(# Example from the assignment
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 1.0 0.0
v 0.0 1.0 0.0
vt 0.0 0.0
vt 1.0 0.0
vt 1.0 1.0
vt 0.0 1.0
f 1/1 2/2 3/3
f 1/1 3/3 4/4
)";

int main(int argc, char* argv[])
{
	if (argc > 2)
	{
		std::cerr << "Usage: triangle_parser [file-path | --demo]\n";
		return 1;
	}
	std::string path;
	if (argc == 2)
		path = argv[1];
	else
	{
		std::cout << "Enter the text file path (Enter = slide example):\n";
		if (!std::getline(std::cin, path))
			return 0;
	}
	// Support pasted paths with surrounding double quotes.
	if (path.size() >= 2 && path.front() == '"' && path.back() == '"')
		path = path.substr(1, path.size() - 2);

	std::filesystem::path filePath{ std::filesystem::current_path() / "data\\warming-up" / path };
	try
	{
		Mesh mesh;
		std::string outputPath;
		if (path.empty() || path == "--demo")
		{
			std::istringstream example(SlideExample);
			mesh = ReadMesh(example);
			outputPath = "triangle_result.txt";
		}
		else
		{
			std::ifstream file(filePath);
			if (!file)
				throw std::runtime_error("Cannot open file: " + filePath.string());
			mesh = ReadMesh(file);
			outputPath = filePath.string() + ".triangles.txt";
		}
		// Parse and validate completely before opening the result file.
		// Invalid input prints an error and does not write a success report.
		std::ofstream resultFile(outputPath);
		if (!resultFile)
			throw std::runtime_error("Cannot create output file: " + outputPath);
		PrintMesh(mesh, resultFile);
		resultFile.close();
		if (!resultFile)
			throw std::runtime_error("Failed to write output file: " + outputPath);
		PrintMesh(mesh, std::cout);
		std::cout << "\nSaved: " << outputPath << '\n';
	}
	catch (const std::exception& error)
	{
		std::cerr << "[Error] " << error.what() << '\n';
		return 1;
	}
	return 0;
}


