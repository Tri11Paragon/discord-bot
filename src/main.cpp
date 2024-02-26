#include <iostream>
#include <dpp/dpp.h>
#include <cstdlib>
#include <utility>
#include <blt/std/logging.h>
#include <blt/parse/argparse.h>
#include <sqlite_orm/sqlite_orm.h>
#include "blt/std/types.h"
#include "blt/std/utility.h"
#include <curl/curl.h>

namespace sql = sqlite_orm;

struct user_info_t
{
    blt::u64 userID;
    std::string username;
    std::string global_nickname;
    std::string server_name;
};

auto make_user_table()
{
    return sql::make_table("users",
                           sql::make_column("userID", &user_info_t::userID, sql::primary_key()),
                           sql::make_column("username", &user_info_t::username),
                           sql::make_column("global_nickname", &user_info_t::global_nickname),
                           sql::make_column("server_name", &user_info_t::server_name));
}

using user_table_t = decltype(make_user_table());

struct user_history_t
{
    blt::u64 userID;
    blt::u64 time_changed;
    std::string old_username;
    std::string old_global_nickname;
    std::string old_server_name;
};

auto make_user_history_table()
{
    return sql::make_table("user_history",
                           sql::make_column("userID", &user_history_t::userID, sql::primary_key()),
                           sql::make_column("time_changed", &user_history_t::time_changed, sql::primary_key()),
                           sql::make_column("old_username", &user_history_t::old_username),
                           sql::make_column("old_global_nickname", &user_history_t::old_global_nickname),
                           sql::make_column("old_server_name", &user_history_t::old_server_name),
                           sql::foreign_key(&user_history_t::userID).references(&user_info_t::userID));
}

using user_history_table_t = decltype(make_user_history_table());

struct channel_info_t
{
    blt::u64 channelID;
    std::string channel_name;
};

auto make_channel_table()
{
    return sql::make_table("channels",
                           sql::make_column("channelID", &channel_info_t::channelID, sql::primary_key()),
                           sql::make_column("channel_name", &channel_info_t::channel_name));
}

using channel_table_t = decltype(make_channel_table());

struct channel_history_t
{
    blt::u64 channelID;
    blt::u64 time_changed;
    std::string old_channel_name;
};

auto make_channel_history_table()
{
    return sql::make_table("channel_history",
                           sql::make_column("channelID", &channel_history_t::channelID, sql::primary_key()),
                           sql::make_column("time_changed", &channel_history_t::time_changed, sql::primary_key()),
                           sql::make_column("old_channel_name", &channel_history_t::old_channel_name),
                           sql::foreign_key(&channel_history_t::channelID).references(&channel_info_t::channelID));
}

using channel_history_table_t = decltype(make_channel_history_table());

struct message_t
{
    blt::u64 messageID;
    blt::u64 channelID;
    blt::u64 userID;
    std::string content;
};

auto make_message_table()
{
    return sql::make_table("messages",
                           sql::make_column("messageID", &message_t::messageID, sql::primary_key()),
                           sql::make_column("channelID", &message_t::channelID),
                           sql::make_column("userID", &message_t::userID),
                           sql::make_column("content", &message_t::content),
                           sql::foreign_key(&message_t::channelID).references(&channel_info_t::channelID),
                           sql::foreign_key(&message_t::userID).references(&user_info_t::userID));
}

using message_table_t = decltype(make_message_table());

struct attachment_t
{
    blt::u64 messageID;
    std::string url;
};

auto make_attachment_table()
{
    return sql::make_table("attachments",
                           sql::make_column("messageID", &attachment_t::messageID, sql::primary_key()),
                           sql::make_column("url", &attachment_t::url, sql::primary_key()),
                           sql::foreign_key(&attachment_t::messageID).references(&message_t::messageID));
}

using attachment_table_t = decltype(make_attachment_table());

struct message_edits_t
{
    blt::u64 messageID;
    std::string old_content;
    std::string new_content;
};

auto make_message_edits_table()
{
    return sql::make_table("message_edits",
                           sql::make_column("messageID", &message_edits_t::messageID, sql::primary_key()),
                           sql::make_column("old_content", &message_edits_t::old_content, sql::primary_key()),
                           sql::make_column("new_content", &message_edits_t::new_content, sql::primary_key()),
                           sql::foreign_key(&message_edits_t::messageID).references(&message_t::messageID));
}

using message_edits_table_t = decltype(make_message_edits_table());

struct message_deletes_t
{
    blt::u64 messageID;
    blt::u64 channelID;
    std::string content;
};

auto make_message_deletes_table()
{
    return sql::make_table("message_deletes",
                           sql::make_column("messageID", &message_deletes_t::messageID, sql::primary_key()),
                           sql::make_column("channelID", &message_deletes_t::channelID, sql::primary_key()),
                           sql::make_column("content", &message_deletes_t::content),
                           sql::foreign_key(&message_deletes_t::messageID).references(&message_t::messageID),
                           sql::foreign_key(&message_deletes_t::channelID).references(&channel_info_t::channelID));
}

using message_deletes_table_t = decltype(make_message_deletes_table());

auto make_database(std::string path)
{
    return sql::make_storage(std::move(path), make_user_table(), make_user_history_table(), make_channel_table(), make_channel_history_table(),
                             make_message_table(), make_attachment_table(), make_message_edits_table(), make_message_deletes_table());
}

using database_type = decltype(make_database(""));

struct db_obj
{
    private:
        blt::u64 guildID;
        database_type db;
        
        void ensure_channel_exists()
        {
        
        }
        
        void ensure_user_exists()
        {
        
        }
    
    public:
        explicit db_obj(blt::u64 guildID, const std::string& path): guildID(guildID), db(make_database(path + "/" + std::to_string(guildID) + "/"))
        {
            db.sync_schema();
        }
        
        void commit(const user_info_t& edited)
        {
        
        }
        
        void commit(const user_history_t& edited)
        {
        
        }
        
        void commit(const channel_info_t& channel)
        {
        
        }
        
        void commit(const channel_history_t& channel)
        {
        
        }
        
        void commit(const message_t& message)
        {
        
        }
        
        void commit(const attachment_t& attachment)
        {
        
        }
        
        void commit(const message_edits_t& edited)
        {
        
        }
        
        void commit(const message_deletes_t& deleted)
        {
        
        }
    
};

blt::hashmap_t<blt::u64, db_obj> databases;
std::string path;

db_obj& get(blt::u64 id)
{
    if (databases.find(id) == databases.end())
        databases.insert({id, db_obj{id, path}});
    return databases.at(id);
}

int main(int argc, const char** argv)
{
    blt::arg_parse parser;
    parser.addArgument(blt::arg_builder("-t", "--token").setAction(blt::arg_action_t::STORE).setHelp("The discord bot token").build());
    parser.addArgument(blt::arg_builder("-p", "--path").setAction(blt::arg_action_t::STORE).setHelp("Path to store the archive data").build());
    
    auto args = parser.parse_args(argc, argv);
    path = args.get<std::string>("path");
    
    dpp::cluster bot(args.get<std::string>("token"), dpp::i_default_intents | dpp::i_message_content | dpp::i_all_intents);
    
    bot.on_user_update([&bot](const dpp::user_update_t& event) {
        BLT_INFO("User '%s' updated in some way; global name: '%s'", event.updated.username.c_str(), event.updated.global_name.c_str());
    });
    
    bot.on_guild_member_update([&bot](const dpp::guild_member_update_t& event) {
    
    });
    
    bot.on_message_delete([&bot](const dpp::message_delete_t& event) {
        BLT_DEBUG("Message %ld deleted content in %ld", event.id, event.channel_id);
    });
    
    bot.on_message_delete_bulk([&bot](const dpp::message_delete_bulk_t& event) {
    
    });
    
    bot.on_message_update([&bot](const dpp::message_update_t& event) {
        auto& storage = get(event.msg.guild_id);
        
        BLT_INFO("%ld (from user %ld in channel %ld ['%s']) -> '%s'", event.msg.id, event.msg.author.id, event.msg.channel_id,
                 event.msg.author.username.c_str(), event.msg.content.c_str());
    });
    
    bot.on_message_create([&bot](const dpp::message_create_t& event) {
        if (event.msg.id == bot.me.id)
            return;
        if (blt::string::starts_with(event.msg.content, "!dump"))
        {
        
        }
//        auto& storage = get(event.msg.guild_id);
//        storage.messages.push_back({
//                                           event.msg.id,
//                                           event.msg.channel_id,
//                                           event.msg.author.id,
//                                           event.msg.content
//                                   });
//
//        for (const dpp::attachment& attach : event.msg.attachments)
//        {
//            storage.attachments.push_back({event.msg.id, attach.url});
//        }
    });
    
    bot.start(dpp::st_wait);
    return 0;
}
