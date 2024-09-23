#include "serializator.h"
#include "application.h"

using namespace std::chrono_literals;
using namespace std::string_literals;

	Serializator::Serializator
	(bool is_auto_save, 
	 const std::chrono::milliseconds save_period, 
	 const std::filesystem::path save_path)
		:
		is_auto_save_(is_auto_save),
		save_period_(save_period),
		save_path_(save_path),
		temp_path_("temp"s),
		time_since_save_(0ms)
	{}

	void Serializator::SerializeData
	(const std::unordered_map<std::string, std::shared_ptr<model::GameSession>>& sessions,
		std::unordered_map<int, std::vector<std::shared_ptr<Player>>> players_to_session)
	{
		using namespace std::filesystem;
		//ÍÅ ÂÛÕÎÄÈÒ ÏÈÑÀÒÜ ÂÎ ÂĞÅÌÅÍÍÛÉ ÔÀÉË ÈÌÅÍÍÎ Â ÒÅÑÒÀÕ - ÍÅÂÎÇÌÎÆÍÎ ÏÅĞÅÈÌÅÍÎÂÀÍÈÅ ÂĞÅÌÅÍÍÎÃÎ ÔÀÉËÀ Â ÖÅËÅÂÎÉ
		//ËÎÊÀËÜÍÎ ĞÀÁÎÒÀÅÒ. ÏĞÎÁËÅÌÀ Ñ ÏĞÀÂÀÌÈ.
		//ÏĞÎÕÎÄÈÒ ÂÑÅ ÒÅÑÒÛ Â ÂÀĞÈÀÍÒÅ ÁÅÇ ÂĞÅÌÅÍÍÎÃÎ ÔÀÉËÀ

		bool is_target_file_exists = std::filesystem::exists(save_path_); // 28-30 òåñòîâûå ñòğîêè.
		std::filesystem::path t;
		is_target_file_exists ? t = temp_path_ : t = save_path_;

		std::fstream out(t, std::ios::out | std::ios::binary | std::ios::trunc);
		out.open(t);
		if (!out.is_open())
		{
			std::string err_mes = "Can't open the file: "s + save_path_.string();
			throw std::logic_error(err_mes);
		}
		boost::archive::binary_oarchive oa{ out };

		std::unordered_map<std::string, std::vector<model::LootSer>> all_serialized_loot;
		std::unordered_map<std::string, std::unordered_map<int, model::DogSer>> all_serialized_dogs;
		std::unordered_map<std::string, std::unordered_map<int, model::PlayerSer>> all_serialized_players;

		for (const auto& session : sessions)
		{
			auto players = players_to_session[session.second->GetObjectId()];
			auto map = session.second->GetMap();

			for (const auto& l : map->GetMapLoot())
			{
				auto serialized_loot = model::LootSer(*l);
				all_serialized_loot[*map->GetId()].push_back(serialized_loot);
			}

			for (const auto& dog : session.second->GetDogs())
			{
				auto serialized_dog = model::DogSer(*dog.second);
				all_serialized_dogs[*map->GetId()][dog.first] = serialized_dog;
			}

			for (const auto& player : players)
			{
				auto serialized_player = model::PlayerSer(*player);
				all_serialized_players[*map->GetId()][player->GetObjectId()] = serialized_player;
			}
		}

		permissions(t, perms::owner_exec | perms::group_exec | perms::group_write | perms::others_write, perm_options::add); // òåñò

		oa << all_serialized_loot;
		oa << all_serialized_dogs;
		oa << all_serialized_players;
		out.close();

		if (is_target_file_exists)
			std::filesystem::rename(temp_path_, save_path_);
	}

	void Serializator::DeserializeData(Application& app)
	{
		if (!std::filesystem::exists(save_path_))
			return;

		std::unordered_map<std::string, std::vector<model::LootSer>> all_serialized_loot;
		std::unordered_map<std::string, std::unordered_map<int, model::DogSer>> all_serialized_dogs;
		std::unordered_map<std::string, std::unordered_map<int, model::PlayerSer>> all_serialized_players;

		std::fstream in;
		in.open(save_path_, std::ios::in | std::ios::binary);
		if (in.is_open())
		{
			{
				boost::archive::binary_iarchive ia{ in };
				ia >> all_serialized_loot;
				ia >> all_serialized_dogs;
				ia >> all_serialized_players;
			}

			if (all_serialized_dogs.empty() || all_serialized_players.empty())
			{
				in.close();
				std::filesystem::remove(save_path_);
				return;
			}
		}
		in.close();
		DataReconstruction(
			app,
			all_serialized_loot,
			all_serialized_dogs,
			all_serialized_players);
		std::filesystem::remove(save_path_);
	}

	void Serializator::SerializeOnTick
	(std::chrono::milliseconds delta,
		const std::unordered_map<std::string, std::shared_ptr<model::GameSession>>& sessions,
		std::unordered_map<int, std::vector<std::shared_ptr<Player>>> players_to_session)
	{
		if (is_auto_save_ == false)
			return;

		time_since_save_ += delta;
		if (time_since_save_ >= save_period_)
		{
			SerializeData(sessions, players_to_session);
			time_since_save_ = 0ms;
		}
	}

	void Serializator::DataReconstruction(Application& app,
		std::unordered_map<std::string, std::vector<model::LootSer>> all_serialized_loot,
		std::unordered_map<std::string, std::unordered_map<int, model::DogSer>> all_serialized_dogs,
		std::unordered_map<std::string, std::unordered_map<int, model::PlayerSer>> all_serialized_players)
	{
		for (const auto& [map_id, dogs] : all_serialized_dogs)
		{
			//Âîññòàíàâëèâàåì ñåññèş
			auto map_ptr_shared = app.GetGame().FindMap(model::Map::Id(map_id));
			std::shared_ptr<model::GameSession> session_ptr_shared = std::make_shared<model::GameSession>(map_ptr_shared);
			//Âîññòàíàâëèâàåì âåñü ëóò íà êàğòå
			for (const auto& loot : all_serialized_loot[*map_ptr_shared->GetId()])
			{
				std::shared_ptr<model::Loot> l = loot.Restore();
				map_ptr_shared->AddLoot(l);
			}

			//Âîññòàíàâëèâàåì ñîáàê è èõ èãğîêîâ, äîáàâëÿåì òîêåíû. object_id ñîáàêè = object_id èãğîêà
			for (const auto& dog : dogs)
			{
				std::shared_ptr<model::Dog> d = dog.second.Restore();
				auto ser_player = all_serialized_players[*map_ptr_shared->GetId()][d->GetObjectId()];
				std::shared_ptr<Player> pl = ser_player.Restore(session_ptr_shared, d);
				session_ptr_shared->AddDog(d);
				app.RestoreToken(Token(pl->GetPlayerToken()), pl);
				app.AddPlayer(pl);
			}
			app.GetGame().AddSession(session_ptr_shared, *map_ptr_shared->GetId());
		}
	}