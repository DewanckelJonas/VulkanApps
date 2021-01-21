#pragma once
#include <vector>
#include <string>
std::vector<char> readFile(const std::string& filename);
std::string GetFilePath(const std::string& str);
std::string GetSuffix(const std::string& filepath);
std::string GetFileName(const std::string& filepath, bool removeExtension = false);