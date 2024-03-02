/*
 *  <Short Description>
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

#ifndef DISCORD_BOT_DATA_STRUCTS_H
#define DISCORD_BOT_DATA_STRUCTS_H

#include <dpp/dpp.h>
#include <sqlite_orm/sqlite_orm.h>

namespace db
{
    struct server_info_t
    {
        blt::u32 member_count;
        std::string name;
        std::string description;
        std::string icon;
        std::string splash;
        std::string discovery_splash;
        std::string banner;
    };
    
    struct user_info_t
    {
        blt::u64 userID;
        std::string username;
        std::string global_nickname;
        std::string server_name;
    };
    
    struct user_history_t
    {
        blt::u64 userID;
        blt::u64 time_changed;
        std::string old_username;
        std::string old_global_nickname;
        std::string old_server_name;
    };
    
    struct channel_info_t
    {
        blt::u64 channelID;
        std::string channel_topic;
        std::string channel_name;
    };
    
    struct channel_history_t
    {
        blt::u64 channelID;
        blt::u64 time_changed;
        std::string old_channel_name;
    };
    
    struct message_t
    {
        blt::u64 messageID;
        blt::u64 channelID;
        blt::u64 userID;
        std::string content;
    };
    
    struct attachment_t
    {
        blt::u64 messageID;
        blt::u64 attachmentID;
        std::string filename;
        std::string description;
        std::string url;
    };
    
    struct message_edits_t
    {
        blt::u64 messageID;
        std::string old_content;
        std::string new_content;
    };
    
    struct message_deletes_t
    {
        blt::u64 messageID;
        blt::u64 channelID;
        std::string content;
    };
    
    auto make_server_info_table()
    {
        using namespace sqlite_orm;
        return make_table("server_info",
                          make_column("member_count", &server_info_t::member_count),
                          make_column("name", &server_info_t::name),
                          make_column("description", &server_info_t::description),
                          make_column("icon", &server_info_t::icon),
                          make_column("splash", &server_info_t::splash),
                          make_column("discovery_splash", &server_info_t::discovery_splash),
                          make_column("banner", &server_info_t::banner),
                          primary_key(&server_info_t::member_count, &server_info_t::name, &server_info_t::description, &server_info_t::icon,
                                      &server_info_t::splash, &server_info_t::discovery_splash, &server_info_t::banner));
    }
    
    using server_info_table_t = decltype(make_server_info_table());
    
    auto make_user_table()
    {
        using namespace sqlite_orm;
        return make_table("users",
                          make_column("userID", &user_info_t::userID, primary_key()),
                          make_column("username", &user_info_t::username),
                          make_column("global_nickname", &user_info_t::global_nickname),
                          make_column("server_name", &user_info_t::server_name));
    }
    
    using user_table_t = decltype(make_user_table());
    
    auto make_user_history_table()
    {
        using namespace sqlite_orm;
        return make_table("user_history",
                          make_column("userID", &user_history_t::userID),
                          make_column("time_changed", &user_history_t::time_changed),
                          make_column("old_username", &user_history_t::old_username),
                          make_column("old_global_nickname", &user_history_t::old_global_nickname),
                          make_column("old_server_name", &user_history_t::old_server_name),
                          foreign_key(&user_history_t::userID).references(&user_info_t::userID),
                          primary_key(&user_history_t::userID, &user_history_t::time_changed));
    }
    
    using user_history_table_t = decltype(make_user_history_table());
    
    auto make_channel_table()
    {
        using namespace sqlite_orm;
        return make_table("channels",
                          make_column("channelID", &channel_info_t::channelID, primary_key()),
                          make_column("channel_topic", &channel_info_t::channel_topic),
                          make_column("channel_name", &channel_info_t::channel_name));
    }
    
    using channel_table_t = decltype(make_channel_table());
    
    auto make_channel_history_table()
    {
        using namespace sqlite_orm;
        return make_table("channel_history",
                          make_column("channelID", &channel_history_t::channelID),
                          make_column("time_changed", &channel_history_t::time_changed),
                          make_column("old_channel_name", &channel_history_t::old_channel_name),
                          foreign_key(&channel_history_t::channelID).references(&channel_info_t::channelID),
                          primary_key(&channel_history_t::channelID, &channel_history_t::time_changed));
    }
    
    using channel_history_table_t = decltype(make_channel_history_table());
    
    auto make_message_table()
    {
        using namespace sqlite_orm;
        return make_table("messages",
                          make_column("messageID", &message_t::messageID, primary_key()),
                          make_column("channelID", &message_t::channelID),
                          make_column("userID", &message_t::userID),
                          make_column("content", &message_t::content),
                          foreign_key(&message_t::channelID).references(&channel_info_t::channelID),
                          foreign_key(&message_t::userID).references(&user_info_t::userID));
    }
    
    using message_table_t = decltype(make_message_table());
    
    auto make_attachment_table()
    {
        using namespace sqlite_orm;
        return make_table("attachments",
                          make_column("messageID", &attachment_t::messageID),
                          make_column("attachmentID", &attachment_t::attachmentID),
                          make_column("filename", &attachment_t::filename),
                          make_column("description", &attachment_t::description),
                          make_column("url", &attachment_t::url),
                          foreign_key(&attachment_t::messageID).references(&message_t::messageID),
                          primary_key(&attachment_t::messageID, &attachment_t::attachmentID));
    }
    
    using attachment_table_t = decltype(make_attachment_table());
    
    auto make_message_edits_table()
    {
        using namespace sqlite_orm;
        return make_table("message_edits",
                          make_column("messageID", &message_edits_t::messageID),
                          make_column("old_content", &message_edits_t::old_content),
                          make_column("new_content", &message_edits_t::new_content),
                          foreign_key(&message_edits_t::messageID).references(&message_t::messageID),
                          primary_key(&message_edits_t::messageID, &message_edits_t::old_content, &message_edits_t::new_content));
    }
    
    using message_edits_table_t = decltype(make_message_edits_table());
    
    auto make_message_deletes_table()
    {
        using namespace sqlite_orm;
        return make_table("message_deletes",
                          make_column("messageID", &message_deletes_t::messageID),
                          make_column("channelID", &message_deletes_t::channelID),
                          make_column("content", &message_deletes_t::content),
                          foreign_key(&message_deletes_t::messageID).references(&message_t::messageID),
                          foreign_key(&message_deletes_t::channelID).references(&channel_info_t::channelID),
                          primary_key(&message_deletes_t::messageID, &message_deletes_t::channelID));
    }
    
    using message_deletes_table_t = decltype(make_message_deletes_table());
    
    auto make_database(std::string path)
    {
        using namespace sqlite_orm;
        return make_storage(std::move(path), make_user_table(), make_user_history_table(), make_channel_table(), make_channel_history_table(),
                            make_message_table(), make_attachment_table(), make_message_edits_table(), make_message_deletes_table(),
                            make_server_info_table());
    }
    
    using database_type = decltype(make_database(""));
}

#endif //DISCORD_BOT_DATA_STRUCTS_H
