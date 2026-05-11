#include <std_include.hpp>
#include "troll.hpp"
#include "loader/component_loader.hpp"
#include "scheduler.hpp"
#include "console.hpp"
#include "game/game.hpp"
#include <utils/hook.hpp>
#include "vars.hpp"
#include "command.hpp"

namespace troll
{
    namespace
    {
        utils::hook::detour getfriendbyindex_hook;
        game::ISteamFriends_vtbl steam_friends_vtbl{};
        unsigned long long steam_id_override = 0;
        void getfriendbyindex_stub(game::ISteamFriends* _this, game::steam_id* out, int iFriend, int iFriendFlags)
        {   
            game::steam_id myid{};
            myid.raw.universe = 1;
            myid.raw.account_type = 1;
            myid.raw.account_instance = 1;
            myid.raw.account_id = 1;
            myid.bits = steam_id_override;
            *out = myid;
        }

        class component final : public component_interface
        {
        public:

            void pre_load() override
            {
                if (!game::environment::is_tpp()) return;

                constexpr uintptr_t RVA_PICKUP_HIGH = 0x0221C790;
                uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
                char* addr = (char*)(base + RVA_PICKUP_HIGH);

                if (strncmp(addr, "PICKUP_HIGH", 11) != 0) {
                    MessageBoxA(NULL, "Mismatch — wrong offset or game version",
                        "Patch Failed", MB_ICONERROR);
                    return;
                }

                utils::hook::copy(addr, "", 12);
                utils::hook::copy(addr, "FOLLOW", 6);
            }

            void start() override
            {
                if (!game::environment::is_tpp()) return;

                scheduler::once([]()
                {
                    const auto steam_friends = (*game::SteamFriends)();
                    steam_friends_vtbl.GetFriendByIndex = steam_friends->__vftable->GetFriendByIndex;
                    utils::hook::set(&steam_friends->__vftable->GetFriendByIndex, getfriendbyindex_stub);
                }, scheduler::main, 0ms);

                command::add("override_steamid_getfriend", [&](const command::params& params)
                {
                     const auto id_s = params.get(1);
                     const auto id = static_cast<unsigned long long>(std::stoull(id_s.data()));

                     steam_id_override = id;
                });
            }
        };
    }
}

REGISTER_COMPONENT(troll::component)