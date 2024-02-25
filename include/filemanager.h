#pragma once
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

#ifndef DISCORD_BOT_FILEMANAGER_H
#define DISCORD_BOT_FILEMANAGER_H

#include <string_view>
#include <blt/std/types.h>
#include <blt/std/hashmap.h>
#include <dpp/message.h>
#include <fstream>
#include <atomic>
#include <memory>

namespace db
{
    // path to archive_folder/raw/guildID/current_day/channelID(name)/
    // in archive_folder/raw/guildID/
    // (days)
    // server_name.txt (changes in server name)
    // migrations.txt (changes in channel name)
    // channels.txt (deleted / create channels)
    
    // in current_day:
    // channelID
    
    // message id: (username):
    class msg_fs_manager
    {
        private:
            std::fstream msg_file;
        public:
            explicit msg_fs_manager(const std::string& file): msg_file(file, std::ios::in | std::ios::out)
            {}
    };
    
    class channel_fs_manager
    {
        private:
            blt::u64 channelID;
            HASHMAP<blt::u64, msg_fs_manager> msgs;
        public:
            explicit channel_fs_manager(blt::u64 channelID): channelID(channelID)
            {}
    };
    
    class channel_flusher
    {
        private:
            std::vector<std::unique_ptr<channel_fs_manager>> channels_to_flush;
        public:
            void flush_channels(HASHMAP<blt::u64, std::unique_ptr<channel_fs_manager>>& map);
    };
    
    class guild_fs_manager
    {
        private:
            std::string archive_path;
            std::string path;
            blt::u64 guildID;
            HASHMAP<blt::u64, std::unique_ptr<channel_fs_manager>> channels;
            dpp::cluster& bot;
            channel_flusher flusher;
            std::fstream f_server_name;
            std::fstream f_migrations;
            std::fstream f_channels;
            std::atomic_int32_t complete{0};
            
            void check_for_channel_updates();
            
            void check_for_guild_updates();
        
        public:
            guild_fs_manager(std::string_view archive_path, std::string_view filePath, blt::u64 guildID, dpp::cluster& bot):
                    archive_path(archive_path), path(filePath), guildID(guildID), bot(bot),
                    f_server_name(this->archive_path + "server_name.txt", std::ios::out | std::ios::in | std::ios::app),
                    f_migrations(this->archive_path + "migrations.txt", std::ios::out | std::ios::in | std::ios::app),
                    f_channels(this->archive_path + "channels.txt", std::ios::out | std::ios::in | std::ios::app)
            {}
            
            void message_create(blt::u64 channel_id, blt::u64 msg_id, std::string_view content, std::string_view username,
                                std::string_view display_name, const std::vector<dpp::attachment>& attachments);
            
            void message_delete(blt::u64 channel_id, blt::u64 msg_id);
            
            void message_bulk_delete(blt::u64 channel_id, const std::vector<dpp::snowflake>& message_ids)
            {
                for (const auto& v : message_ids)
                    message_delete(channel_id, format_as(v));
            }
            
            void message_update(blt::u64 channel_id, blt::u64 msg_id, std::string_view new_content);
            
            void flush(std::string new_path);
            
            void await_completions();
    };
    
    class fs_manager
    {
        private:
            HASHMAP<blt::u64, std::unique_ptr<guild_fs_manager>> guild_handlers;
            dpp::cluster& bot;
            std::string archive_path;
            int current_day = 0;
            std::atomic_bool is_flushing{false};
            
            std::string create_path(blt::u64 guildID);
        
        public:
            explicit fs_manager(std::string_view archive_path, dpp::cluster& bot);
            
            void create(blt::u64 guildID)
            {
                guild_handlers.insert({guildID, std::make_unique<guild_fs_manager>(archive_path, create_path(guildID), guildID, bot)});
            }
            
            void flush();
            
            guild_fs_manager& operator[](blt::u64 guildID)
            {
                while (is_flushing)
                {}
                return *guild_handlers.at(guildID);
            }
    };
    
}

#endif //DISCORD_BOT_FILEMANAGER_H
