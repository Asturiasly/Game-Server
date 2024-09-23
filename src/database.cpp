#include "database.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

 postgres::Database::Database(std::shared_ptr<ConnectionPool> connection_pool) : connection_pool_(connection_pool)
{
    auto conn = connection_pool_->GetConnection();
    pqxx::work w{ *conn };

    w.exec(R"(
                CREATE TABLE IF NOT EXISTS retired_players( 
                id UUID CONSTRAINT player_id_constraint PRIMARY KEY,
                name varchar(100) NOT NULL,
                score integer NOT NULL,
                play_time_ms integer NOT NULL)
            )"_zv);
    w.exec("CREATE INDEX IF NOT EXISTS score_play_time_name_idx ON retired_players (score DESC, play_time_ms ASC, name ASC); "_zv);
    w.commit();
}

void postgres::Database::AddRetiredPlayersToDB(std::vector<std::shared_ptr<Player>>& plrs)
{
    auto conn = connection_pool_->GetConnection();
    pqxx::work w{ *conn };
    std::string query = "INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ";
    for (const auto& plr : plrs)
    {
        std::string uuid = util::detail::UUIDToString(util::detail::NewUUID());
        double play_time = static_cast<double>(plr->GetPlayTime().count());
        query.append("('" + uuid + "'");
        query.append(", '" + w.esc(plr->GetName()) + "'");
        query.append(", " + std::to_string(plr->GetDog()->GetCurrentScore()));
        if (plr == plrs.back())
        {
            query.append(", " + std::to_string(play_time) + ");");
        }
        else
        {
            query.append(", " + std::to_string(play_time) + "), ");
        }
    }
    w.exec(query);
    w.commit();
}

std::vector<std::tuple<std::string, int, int>> postgres::Database::GetRetiredPlayersFromDB(RecordsParams& p)
{
    auto conn = connection_pool_->GetConnection();
    pqxx::read_transaction r(*conn);
    std::string query = "SELECT name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms ASC, name ASC OFFSET " 
        + r.quote(p.start) + " LIMIT " + r.quote(p.maxItems) + ";";

    std::vector<std::tuple<std::string, int, int>> retired_players_info;

    for (const auto row : r.query<std::string, int, int>(query))
    {
        retired_players_info.push_back(row);
    }

    return retired_players_info;
}