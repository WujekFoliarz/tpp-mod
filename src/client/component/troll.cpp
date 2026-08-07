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
        vars::var_ptr var_replace_pickup_high_with_follow;

        // Any FOB attack cheat
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

        void replace_pickup_with_follow()
        {
            constexpr uintptr_t RVA_PICKUP = 0x0221C6B0;
            uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
            char* addr = (char*)(base + RVA_PICKUP);

            if (strncmp(addr, "PICKUP", 6) != 0) {
                MessageBoxA(NULL, "Mismatch — wrong offset or game version",
                    "Patch Failed", MB_ICONERROR);
                return;
            }

            utils::hook::copy(addr, "", 7);
            utils::hook::copy(addr, "FOLLOW", 6);
        }

        void replace_pickup_high_with_emergency()
        {
            constexpr uintptr_t RVA_PICKUP_HIGH = 0x0221C690;
            uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
            char* addr = (char*)(base + RVA_PICKUP_HIGH);

            if (strncmp(addr, "PICKUP_HIGH", 11) != 0) {
                MessageBoxA(NULL, "Mismatch — wrong offset or game version",
                    "Patch Failed", MB_ICONERROR);
                return;
            }

            utils::hook::copy(addr, "", 12);
            utils::hook::copy(addr, "EMERGENCY", 9);
        }

        void replace_pickup_high_with_follow()
        {
            constexpr uintptr_t RVA_PICKUP_HIGH = 0x0221C690;
            uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
            char* addr = (char*)(base + RVA_PICKUP_HIGH);

            if (strncmp(addr, "PICKUP_HIGH", 11) != 0) {
                MessageBoxA(NULL, "Mismatch — wrong offset or game version",
                    "Patch Failed", MB_ICONERROR);
                return;
            }

            utils::hook::copy(addr, "", 12);
            utils::hook::copy(addr, "FOLLOW", 6);
            console::info("[troll] Replaced PICKUP_HIGH with FOLLOW");
        }

#if 0
        // Reverse wormhole cheat
        utils::hook::detour cmd_open_wormhole_option_pack_hook;
        static bool reverse_wormhole_enabled = true;
        char cmd_open_wormhole_option_pack_stub(void* cmd_open_wormhole_option)
        {
            uint8_t* base = (uint8_t*)cmd_open_wormhole_option;
            //int player_id_original = *(int*)(base + 0x50); 
            int to_player_id_original = *(int*)(base + 0x54);
            //int is_open_original = *(int*)(base + 0x58);  
            //int retaliate_score_original = *(int*)(base + 0x5C);

            if (reverse_wormhole_enabled)
            {
                int player_id_new = to_player_id_original;
                //int to_player_id_new = player_id_original;

                *(int*)(base + 0x50) = player_id_new;
                *(int*)(base + 0x54) = 3556297;
                *(int*)(base + 0x58) = 1; // Is open
            }

            return cmd_open_wormhole_option_pack_hook.invoke<char>(cmd_open_wormhole_option);
        }

        // No wormhole test
        utils::hook::detour cmd_sneak_mother_base_hook;
        __int64 cmd_sneak_mother_base_stub(void* a1)
        {
            uint8_t* base = (uint8_t*)a1;

            // wormhole player id
            *(int*)(base + 140) = 0;

            // Motherbase ID
            //*(int*)(base + 0x50) = 1325990;

            // is_sneak
            //*(int*)(base + 0x78) = 0;

            // is_plus
            //*(int*)(base + 0x90) = 0;

            return cmd_sneak_mother_base_hook.invoke<__int64>(a1);
        }

        // No wormhole test 2
        utils::hook::detour cmd_send_sneak_result_hook;
        char cmd_send_sneak_result_stub(void* a1)
        {
            uint8_t* base = (uint8_t*)a1;

            // Open retaliate wormhole
            *(int*)(base + 140) = 1;
            *(int*)(base + 144) = 1;

            return cmd_send_sneak_result_hook.invoke<char>(a1);
        }
#endif // 0


        class component final : public component_interface
        {
        public:

            void pre_load() override
            {
                if (!game::environment::is_tpp()) return;

                var_replace_pickup_high_with_follow = vars::register_int("var_replace_pickup_high_with_follow", 0, 0, 1, vars::var_flag_saved, "Replaces PICKUP_HIGH with FOLLOW on startup");
                
#if 0
                cmd_open_wormhole_option_pack_hook.create(SELECT_VALUE(0x145B2F090, 0x0, 0x0, 0x0), cmd_open_wormhole_option_pack_stub);
                cmd_sneak_mother_base_hook.create(SELECT_VALUE(0x145B1FFE0, 0x0, 0x0, 0x0), cmd_sneak_mother_base_stub);
                cmd_send_sneak_result_hook.create(SELECT_VALUE(0x145B2B320, 0x0, 0x0, 0x0), cmd_send_sneak_result_stub);
#endif // 0

                //cmd_get_fob_target_list_option_pack_hook.create(SELECT_VALUE(0x145B1AE00, 0x0, 0x0, 0x0), cmd_get_fob_target_list_option_pack_stub);
            }

            void start() override
            {
                if (!game::environment::is_tpp()) return;

                if (var_replace_pickup_high_with_follow->current.get_int() == 1)
                {
                    replace_pickup_high_with_follow();
                }

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