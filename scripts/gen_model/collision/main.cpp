#include "collision_writer.hpp"
#include "model_importer.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
	std::vector<std::filesystem::path> parseInputs(const std::string& value) {
		if (value.empty())
			throw std::invalid_argument("INPUT cannot be empty");

		std::vector<std::filesystem::path> inputs;
		std::stringstream stream(value);
		std::string item;
		while (std::getline(stream, item, ',')) {
			if (item.empty())
				throw std::invalid_argument("INPUT contains an empty path");
			inputs.emplace_back(item);
		}
		return inputs;
	}

	std::filesystem::path collisionPath(const std::filesystem::path& input) {
		std::string extension = input.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		if (extension != ".obj")
			throw std::invalid_argument("Collision input must use the .obj extension: " + input.string());
		if (input.stem().string().ends_with(".collision"))
			throw std::invalid_argument("Collision input is already a collision output: " + input.string());
		return input.parent_path() / (input.stem().string() + ".collision.obj");
	}
}

int main(int argc, char** argv) {
	try {
		if (argc != 2)
			throw std::invalid_argument("Usage: gen_collision input1.obj,input2.obj");

		const auto inputs = parseInputs(argv[1]);
		std::set<std::filesystem::path> inputSet;
		std::set<std::filesystem::path> outputSet;
		std::vector<std::filesystem::path> outputs;
		outputs.reserve(inputs.size());

		for (const auto& input : inputs) {
			if (!std::filesystem::is_regular_file(input))
				throw std::invalid_argument("Input file does not exist: " + input.string());

			const auto normalizedInput = std::filesystem::weakly_canonical(input);
			if (!inputSet.insert(normalizedInput).second)
				throw std::invalid_argument("Duplicate collision input: " + input.string());

			const auto output = collisionPath(input);
			const auto normalizedOutput = std::filesystem::weakly_canonical(output);
			if (!outputSet.insert(normalizedOutput).second)
				throw std::invalid_argument("Duplicate collision output: " + output.string());
			if (normalizedInput == normalizedOutput)
				throw std::invalid_argument("Collision output would overwrite its input: " + input.string());
			outputs.push_back(output);
		}

		for (std::size_t index = 0; index < inputs.size(); ++index) {
			const auto mesh = gen_model::collision::importObj(inputs[index]);
			gen_model::collision::writeObj(mesh, outputs[index]);
			std::cout << inputs[index].string() << " -> " << outputs[index].string() << '\n';
		}
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "gen_collision: " << error.what() << '\n';
		return 1;
	}
}
