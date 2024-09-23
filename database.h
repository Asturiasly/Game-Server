#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>
#include <memory>
#include <tuple>

#include "connection_pool.h"
#include "tagged_uuid.h"
#include "player.h"

using namespace std::literals;
using pqxx::operator"" _zv;

namespace postgres {
    class Database {
    public:

        explicit Database(std::shared_ptr<ConnectionPool> connection_pool);
        void AddRetiredPlayersToDB(std::vector<std::shared_ptr<Player>>& plrs);
        std::vector<std::tuple<std::string, int, int>> GetRetiredPlayersFromDB(RecordsParams& p);

    private:
        std::shared_ptr<ConnectionPool> connection_pool_;
    };
}