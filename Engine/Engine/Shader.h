#pragma once
#include "Platform.h"
#include <vector>

VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device);