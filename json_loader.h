#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "boost/json.hpp"
#include "model.h"
#include "loot_generator.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);
std::string ReadJSONFromPath(const std::filesystem::path& json_path);
std::string ReadJSONFromStream(std::stringstream& input);
void AddingRoadsToMap(const boost::json::array& roads, model::Map& map);
void AddingBuildingsToMap(const boost::json::array& buildings, model::Map& map);
void AddingOfficesToMap(const boost::json::array& offices, model::Map& map);
}  // namespace json_loader
