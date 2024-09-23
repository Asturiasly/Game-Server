#pragma once
#include <pqxx/pqxx>

#include "model.h"
#include "players.h"
#include "players_tokens.h"
#include "serializator.h"
#include "database.h"

struct GameSettings
{
	bool is_auto_tick;
	bool is_save_mode;
	bool is_auto_save_mode;
	bool randomize_spawn_points;
	uint32_t save_period;
	uint32_t retirement_time;
};

class Application
{
public:
	Application(model::Game& game, GameSettings settings, std::filesystem::path save_path, std::unique_ptr<postgres::Database> db);
	std::shared_ptr<Player> FindPlayerByToken(Token token);
	Token SetTokenForPlayer(std::shared_ptr<Player> player);
	const std::vector<std::shared_ptr<Player>> GetPlayersInSession(int session_id) const;
	void AddPlayer(std::shared_ptr<Player> player);
	GameSettings& GetSettings();
	model::Game& GetGame();
	void Serialize();
	void SerializeOnTick(std::chrono::milliseconds time);
	void Deserialize();
	void RestoreToken(Token token, std::shared_ptr<Player> player);
	void DeleteUnusedInfo(std::chrono::milliseconds delta);
	std::vector<std::tuple<std::string, int, int>> GetRetiredPlayersInfo(RecordsParams& p);
	void OnTick(std::chrono::milliseconds time);

private:
	model::Game& game_;
	GameSettings settings_;
	std::unique_ptr<Players> players_;
	std::unique_ptr<token::PlayersTokens> tokens_;
	std::unique_ptr<Serializator> serializator_;
	std::unique_ptr<postgres::Database> db_;

	void DeleteEmptySessions();
	void DeleteIdlePlayers(std::chrono::milliseconds time, std::vector<int>& ids);
	void AddRetiredPlayersToDB(std::vector<std::shared_ptr<Player>>& ids);
	void UpdateAllPlayersPlayTime(std::chrono::milliseconds time);
};