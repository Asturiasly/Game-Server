#pragma once

#include <map>
#include <memory>

#include "player.h"
#include "model.h"

class Players
{
public:
	Players() = default;

	void Add(std::shared_ptr<Player> player)
	{
		int id = player->GetObjectId();
		players_[id] = player;

		session_to_players_[player->GetSession()->GetObjectId()].push_back(player);
	}

	const std::map<int, std::shared_ptr<Player>>& GetPlayers() const
	{
		return players_;
	}

	const std::vector<std::shared_ptr<Player>>& GetPlayersInSession(int session_id)
	{
		return session_to_players_[session_id];
	}

	const std::unordered_map<int, std::vector<std::shared_ptr<Player>>>& GetAllPlayersBySessions() const
	{
		return session_to_players_;
	}

	void DeletePlayer(int id)
	{
		auto player_it = players_.find(id);
		int player_id = player_it->second->GetObjectId();
		int session_id = player_it->second->GetSession()->GetObjectId();

		auto it = std::find_if(session_to_players_[session_id].begin(), session_to_players_[session_id].end(), [player_id](std::shared_ptr<Player> pl)
			{
				return player_id == pl->GetObjectId();
			});

		if (it == session_to_players_[session_id].end())
			throw std::runtime_error("Can't delete uncreated user");

		session_to_players_[session_id].erase(it);

		if (session_to_players_[session_id].empty())
		{
			auto ses_it = session_to_players_.find(session_id);

			if (ses_it == session_to_players_.end())
				throw std::runtime_error("");
			session_to_players_.erase(ses_it);
		}

		players_.erase(player_it);
	}

	std::shared_ptr<Player> GetPlayerByIndx(int id) const
	{
		if (!players_.contains(id))
			throw std::runtime_error("Can't find uncreated player");
		return players_.at(id);
	}

private:
	std::map<int, std::shared_ptr<Player>> players_; //all players by id
	std::unordered_map<int, std::vector<std::shared_ptr<Player>>> session_to_players_; //all players in session by id
};
