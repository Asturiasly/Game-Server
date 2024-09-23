#pragma once

#include <memory>
#include <chrono>

#include "model_serialization.h"
class Player
{
	friend class model::PlayerSer;
public:

	Player(std::shared_ptr<model::GameSession> session, std::shared_ptr<model::Dog> dog, std::string player_name) 
		: session_(session), dog_(dog), player_name_(player_name), play_time_(0)
	{
		++generation_id_;
		object_id_ = generation_id_;
	}

	const int GetObjectId() const noexcept {
		return object_id_;
	}

	const int GetGenerationId() const noexcept
	{
		return generation_id_;
	}

	const std::string& GetName() const
	{
		return player_name_;
	}

	void SetToken(std::string token)
	{
		token_ = token;
	}

	std::shared_ptr<model::Dog>& GetDog()
	{
		return dog_;
	}

	std::shared_ptr<model::GameSession> GetSession()
	{
		return session_;
	}

	const std::string& GetPlayerToken() const
	{
		return token_;
	}

	void AddPlayTime(std::chrono::milliseconds delta)
	{
		play_time_ += delta;
	}

	std::chrono::milliseconds GetPlayTime() const
	{
		return play_time_;
	}

private:
	static inline int generation_id_ = -1;
	int object_id_;
	std::shared_ptr<model::GameSession> session_;
	std::shared_ptr<model::Dog> dog_;
	std::string player_name_;
	std::chrono::milliseconds play_time_;
	std::string token_;
};