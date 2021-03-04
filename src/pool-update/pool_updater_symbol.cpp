#include "pool_updater.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "util/util.hpp"
#include "pool/symbol.hpp"

namespace horizon {
void PoolUpdater::update_symbols(const std::string &directory, const std::string &prefix)
{
    Glib::Dir dir(directory);
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(directory, it);
        if (endswith(it, ".json")) {
            update_symbol(filename, false);
        }
        else if (Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
            update_symbols(filename, Glib::build_filename(prefix, it));
        }
    }
}

void PoolUpdater::update_symbol(const std::string &filename, bool partial)
{
    try {
        status_cb(PoolUpdateStatus::FILE, filename, "");
        auto symbol = Symbol::new_from_file(filename, *pool);
        bool overridden = false;
        if (exists(ObjectType::SYMBOL, symbol.uuid)) {
            overridden = true;
            delete_item(ObjectType::SYMBOL, symbol.uuid);
        }
        if (partial)
            overridden = false;

        SQLite::Query q(pool->db,
                        "INSERT INTO symbols "
                        "(uuid, name, filename, unit, pool_uuid, overridden) "
                        "VALUES "
                        "($uuid, $name, $filename, $unit, $pool_uuid, $overridden)");
        q.bind("$uuid", symbol.uuid);
        q.bind("$name", symbol.name);
        q.bind("$unit", symbol.unit->uuid);
        q.bind("$pool_uuid", pool_uuid);
        q.bind("$overridden", overridden);
        q.bind("$filename", get_path_rel(filename));
        q.step();
        add_dependency(ObjectType::SYMBOL, symbol.uuid, ObjectType::UNIT, symbol.unit->uuid);
    }
    catch (const std::exception &e) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, e.what());
    }
    catch (...) {
        status_cb(PoolUpdateStatus::FILE_ERROR, filename, "unknown exception");
    }
}
} // namespace horizon
