/*
 *  Copyright (C) 2024  Brett Terpstra
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <filemanager.h>
#include <blt/std/logging.h>
#include <blt/std/string.h>
#include "blt/std/time.h"
#include <dpp/cluster.h>

namespace db
{
    
    void guild_fs_manager::message_create(blt::u64, blt::u64, std::string_view, std::string_view,
                                          std::string_view, const std::vector<dpp::attachment>&)
    {
    
    }
    
    void guild_fs_manager::message_delete(blt::u64, blt::u64)
    {
    
    }
    
    void guild_fs_manager::message_update(blt::u64, blt::u64, std::string_view)
    {
    
    }
    
    void guild_fs_manager::flush(std::string new_path)
    {
        complete = 3;
        check_for_guild_updates();
        flusher.flush_channels(channels);
        complete--;
        path = std::move(new_path);
    }
    
    void guild_fs_manager::check_for_channel_updates()
    {
        bot.guild_get(dpp::snowflake(guildID), [this](const dpp::confirmation_callback_t& con) {
            auto guild = con.get<dpp::guild>();
            f_server_name.seekg(0);
            
        });
    }
    
    void guild_fs_manager::check_for_guild_updates()
    {
    
    }
    
    void guild_fs_manager::await_completions()
    {
        BLT_DEBUG("Awaiting all requests to complete");
        while (complete > 0)
        {}
        complete = 0;
        BLT_DEBUG("Awaiting completed");
    }
    
    void fs_manager::flush()
    {
        BLT_TIME_FUNC(now);
        if (current_day != now.tm_yday)
        {
            is_flushing = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            current_day = now.tm_yday;
            
            for (auto& gm : guild_handlers)
            {
                gm.second->flush(create_path(gm.first));
                gm.second->await_completions();
            }
            
            is_flushing = false;
        }
    }
    
    fs_manager::fs_manager(std::string_view archive_path, dpp::cluster& bot): bot(bot), archive_path(archive_path)
    {
        BLT_INFO("Creating with path '%s'", this->archive_path.c_str());
        BLT_TIME_FUNC(now);
        current_day = now.tm_yday;
        blt::string::replaceAll(this->archive_path, "\\", "/");
        if (!blt::string::ends_with(archive_path, '/'))
            this->archive_path += "/";
        BLT_INFO("Modified path '%s'", this->archive_path.c_str());
    }
    
    std::string fs_manager::create_path(blt::u64 guildID)
    {
        BLT_TIME_FUNC(now);
        auto local_guild_path = archive_path + "raw/" + std::to_string(guildID) + "/";
        auto local_day_path = local_guild_path + std::to_string(now.tm_year + 1900) + "-" + std::to_string(now.tm_mon + 1) +
                              "-" + std::to_string(now.tm_mday) + "/";
        std::filesystem::create_directories(local_day_path);
        return local_day_path;
    }
    
    void channel_flusher::flush_channels(blt::hashmap_t<blt::u64, std::unique_ptr<channel_fs_manager>>& map)
    {
        channels_to_flush.reserve(map.size());
        for (auto& v : map)
            channels_to_flush.push_back(std::move(v.second));
        map.clear();
    }
}