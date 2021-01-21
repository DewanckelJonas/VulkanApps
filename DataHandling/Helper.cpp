#include "Helper.h"
#include <fstream>
#include <cassert>

std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary); //ate makes it so you start reading at the end of the file handy for knowing filesize.
	if (!file.is_open())
	{
		assert(0 && "Failed to open file!");
		std::exit(-1);
	}
	
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

std::string GetFilePath(const std::string& str)
{
	size_t found = str.find_last_of("/\\");
	return str.substr(0, found);
}


std::string GetSuffix(const std::string& filepath)
{
	const size_t pos = filepath.rfind('.');
	return (pos == std::string::npos) ? "" : filepath.substr(filepath.rfind('.') + 1);
}

std::string GetFileName(const std::string& filepath, bool removeExtension)
{
	std::string fileName = filepath.substr(filepath.find_last_of("/\\") + 1);
	if(removeExtension)
	{
		std::string::size_type const pos(fileName.find_last_of('.'));
		fileName = fileName.substr(0, pos);
	}
	return fileName;
}