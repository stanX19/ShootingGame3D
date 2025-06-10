#include <sstream>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <iostream>
#include "utils.hpp"

std::string getParentDir(const std::string &path) {
	return std::filesystem::path(path).parent_path();
}

std::string GetFileName(const std::string &path) {
	return std::filesystem::path(path).filename();
}