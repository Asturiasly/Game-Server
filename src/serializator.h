#pragma once

#include "model_serialization.h"
#include <fstream>
#include <chrono>
#include <filesystem>

using namespace std::chrono_literals;
using namespace std::string_literals;

class Application;

class Serializator
{
public:

	Serializator() = default;
	Serializator(bool is_auto_save, const std::chrono::milliseconds save_period, const std::filesystem::path save_path);
	void SerializeData
		(const std::unordered_map<std::string, std::shared_ptr<model::GameSession>>& sessions,
		std::unordered_map<int, std::vector<std::shared_ptr<Player>>> players_to_session);
	void DeserializeData(Application& app);
	void SerializeOnTick
		(std::chrono::milliseconds delta,
		const std::unordered_map<std::string, std::shared_ptr<model::GameSession>>& sessions,
		std::unordered_map<int, std::vector<std::shared_ptr<Player>>> players_to_session);

private:
	bool is_auto_save_;
	const std::chrono::milliseconds save_period_;
	std::filesystem::path save_path_;
	std::filesystem::path temp_path_;
	std::chrono::milliseconds time_since_save_;

	void DataReconstruction(Application& app,
		std::unordered_map<std::string, std::vector<model::LootSer>> all_serialized_loot,
		std::unordered_map<std::string, std::unordered_map<int, model::DogSer>> all_serialized_dogs,
		std::unordered_map<std::string, std::unordered_map<int, model::PlayerSer>> all_serialized_players);
};