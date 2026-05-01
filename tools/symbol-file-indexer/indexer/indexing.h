#pragma once

#include <filesystem>
#include <string>

std::string HashForPE(const std::filesystem::path& path);
std::string SignatureForPDB(const std::filesystem::path& path);
