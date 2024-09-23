#include "application.h"
#include <fstream>

Application::Application(model::Game& game, GameSettings settings, std::filesystem::path save_path, std::unique_ptr<postgres::Database> db) :
	game_(game),
	settings_(settings),
	players_(std::make_unique<Players>()),
	tokens_(std::make_unique<token::PlayersTokens>()),
	serializator_(std::make_unique<Serializator>
		(settings.is_auto_save_mode,
			std::chrono::milliseconds(settings.save_period),
			save_path)),
	db_(std::move(db))
{}

std::shared_ptr<Player> Application::FindPlayerByToken(Token token)
{
	return tokens_->FindPlayerByToken(token);
}

Token Application::SetTokenForPlayer(std::shared_ptr<Player> player)
{
	return tokens_->SetTokenForPlayer(player);
}

const std::vector<std::shared_ptr<Player>> Application::GetPlayersInSession(int session_id) const
{
	return players_->GetPlayersInSession(session_id);
}

void Application::AddPlayer(std::shared_ptr<Player> player)
{
	players_->Add(player);
}

GameSettings& Application::GetSettings()
{
	return settings_;
}

model::Game& Application::GetGame()
{
	return game_;
}

void Application::Serialize()
{
	serializator_->SerializeData(game_.GetSessions(), players_->GetAllPlayersBySessions());
}

void Application::SerializeOnTick(std::chrono::milliseconds time)
{
	serializator_->SerializeOnTick(time, game_.GetSessions(), players_->GetAllPlayersBySessions());
}

void Application::Deserialize()
{
	serializator_->DeserializeData(*this);
}

void Application::RestoreToken(Token token, std::shared_ptr<Player> player)
{
	tokens_->AddTokenAndPlayer(player, token);
}

void Application::DeleteUnusedInfo(std::chrono::milliseconds delta)
{
	auto ids = game_.GetIdleDogs(delta);
	if (ids.empty())
		return;

	std::vector<std::shared_ptr<Player>> plrs;
	for (const auto id : ids)
	{
		plrs.push_back(players_->GetPlayerByIndx(id));
	}

	AddRetiredPlayersToDB(plrs);
	DeleteIdlePlayers(delta, ids);
	plrs.clear();
	DeleteEmptySessions();
}

std::vector<std::tuple<std::string, int, int>> Application::GetRetiredPlayersInfo(RecordsParams& p)
{
	return db_->GetRetiredPlayersFromDB(p);
}

void Application::OnTick(std::chrono::milliseconds time)
{
	std::fstream out;
	if (!out.is_open())
	{
		out.open("/tmp/volume/log.txt", std::ios::out | std::ios::app);
	}

	//"C:\\Users\\impro\\Desktop\\fall_logs\\test_log.txt"

	out << "START UPDATE ALL PLAYERS PLAYTIME" << std::endl;
	UpdateAllPlayersPlayTime(time);
	out << "END UPDATE ALL PLAYERS PLAYTIME" << std::endl;
	out << "START UpdateGameState" << std::endl;
	game_.UpdateGameState(time);
	out << "END UpdateGameState" << std::endl;
}

void Application::DeleteEmptySessions()
{
	game_.DeleteEmptySessions();
}

void Application::DeleteIdlePlayers(std::chrono::milliseconds time, std::vector<int>& ids)
{
	for (const auto& id : ids)
	{
		auto player = players_->GetPlayerByIndx(id);
		tokens_->DeletePlayerByToken(Token(player->GetPlayerToken()));
		players_->DeletePlayer(id);
	}
}

void Application::AddRetiredPlayersToDB(std::vector<std::shared_ptr<Player>>& ids)
{
	db_->AddRetiredPlayersToDB(ids);
}

void Application::UpdateAllPlayersPlayTime(std::chrono::milliseconds time)
{
	for (const auto& plr : players_->GetPlayers())
	{
		plr.second->AddPlayTime(time);
	}
}