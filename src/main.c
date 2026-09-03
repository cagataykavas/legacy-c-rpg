#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int read_int(const char *prompt, int min_value, int max_value) {
    char line[128];
    long value;
    char *end;
    for (;;) {
        if (prompt != NULL) {
            fputs(prompt, stdout);
        }
        if (fgets(line, sizeof(line), stdin) == NULL) {
            return min_value;
        }
        value = strtol(line, &end, 10);
        if (end != line && value >= min_value && value <= max_value) {
            return (int)value;
        }
        printf("Choose a number between %d and %d.\n", min_value, max_value);
    }
}

static Stats create_stats(int budget) {
    Stats stats = {0};
    for (;;) {
        printf("\nSTATS — budget %d\n", budget);
        stats.strength = read_int("STR: ", 0, budget);
        stats.agility = read_int("AGI: ", 0, budget);
        stats.stamina = read_int("STA: ", 0, budget);
        stats.luck = read_int("LU : ", 0, budget);
        stats.magic = read_int("MA : ", 0, budget);
        if (stats_fit_budget(stats, budget)) {
            return stats;
        }
        puts("Hey that is cheating! Total stats exceed the difficulty budget.");
    }
}

static void print_player(const Player *player) {
    char summary[512];
    size_t index;
    player_format_summary(player, summary, sizeof(summary));
    printf("\n%s\n", summary);
    puts("Equipment:");
    for (index = 0; index < RPG_EQUIPMENT_SLOTS; ++index) {
        const EquipmentPiece *piece = &player->equipment[index];
        printf(
            "  %zu. %-22s owned=%s equipped=%s STA+%d\n",
            index + 1,
            piece->name,
            piece->owned ? "yes" : "no",
            piece->equipped ? "yes" : "no",
            piece->stamina_bonus
        );
    }
}

static void equipment_menu(Player *player) {
    for (;;) {
        int choice;
        print_player(player);
        choice = read_int("Toggle equipment 1-4, or 5 to leave: ", 1, 5);
        if (choice == 5) {
            return;
        }
        if (!player_toggle_equipment(player, (EquipmentSlot)(choice - 1))) {
            puts("You do not own that item yet.");
        }
    }
}

static void shop_menu(Player *player) {
    size_t count;
    size_t index;
    const EquipmentPiece *catalog = shop_catalog(&count);
    for (;;) {
        int choice;
        printf(
            "\nArmorcraftsman — Gold %d | Wolfskins %d\n",
            player->inventory.gold,
            player->inventory.wolfskins
        );
        for (index = 0; index < count; ++index) {
            printf(
                "%zu. %-22s %d gold + %d wolfskin(s), STA +%d\n",
                index + 1,
                catalog[index].name,
                catalog[index].gold_cost,
                catalog[index].wolfskin_cost,
                catalog[index].stamina_bonus
            );
        }
        printf("%zu. Leave\n", count + 1);
        choice = read_int("Choice: ", 1, (int)count + 1);
        if (choice == (int)count + 1) {
            return;
        }
        {
            PurchaseResult result = shop_purchase_armor(
                player,
                (EquipmentSlot)(choice - 1)
            );
            puts(result.message);
        }
    }
}

static void town_menu(Player *player) {
    for (;;) {
        int choice = read_int(
            "\nTown: armorcraftsman(1), inventory/equipment(2), leave(3): ",
            1,
            3
        );
        if (choice == 1) {
            shop_menu(player);
        } else if (choice == 2) {
            equipment_menu(player);
        } else {
            return;
        }
    }
}

static void announce_enemy(const Enemy *enemy) {
    switch (enemy->kind) {
        case ENEMY_ZOMBIE:
            puts("A zombie approaches you.");
            break;
        case ENEMY_WEREWOLF:
            puts("Oh, a werewolf is coming!");
            break;
        case ENEMY_SHREK:
            /* Preserve the original student's glorious line exactly. */
            puts("Mighty .:SHREK:. appears!");
            break;
        default:
            puts("Something horrible appears.");
            break;
    }
}

static CombatResult run_combat(Player *player, Enemy *enemy, Rng *rng, const GameRules *rules) {
    CombatResult result = COMBAT_CONTINUES;
    while (result == COMBAT_CONTINUES) {
        CombatTurn turn;
        int choice;
        printf("\nYour HP=%d/%d | %s HP=%d/%d\n", player->hp, player_max_hp(player), enemy->name, enemy->hp, enemy->max_hp);
        choice = read_int("Attack(1), Block(2), Heal(3), Run(4): ", 1, 4);
        turn = combat_take_turn(player, enemy, (CombatAction)choice, rng, rules);
        if (turn.player_damage > 0) {
            printf("You deal %d damage.\n", turn.player_damage);
        }
        if (turn.healing > 0) {
            printf("You channel MA and heal %d HP.\n", turn.healing);
        }
        if (choice == COMBAT_BLOCK) {
            puts(turn.blocked ? "You brace and reduce the incoming blow." : "You try to block. The legacy rules do absolutely nothing.");
        }
        if (turn.enemy_damage > 0) {
            printf("%s hits you for %d.\n", enemy->name, turn.enemy_damage);
        }
        result = turn.result;
    }
    return result;
}

static void plains(Player *player, Rng *rng, const GameRules *rules) {
    Enemy enemy;
    CombatResult result;
    int before_level;
    int before_gold;
    int before_wolfskins;
    int before_onions;

    /* Original program reset charhp when entering the plains encounter. */
    player_restore_hp(player);
    enemy = roll_enemy(rng);
    announce_enemy(&enemy);
    result = run_combat(player, &enemy, rng, rules);

    if (result == COMBAT_PLAYER_WON) {
        before_level = player->level;
        before_gold = player->inventory.gold;
        before_wolfskins = player->inventory.wolfskins;
        before_onions = player->inventory.onions;
        award_enemy_rewards(player, &enemy, rng, rules);
        printf("Victory over %s.\n", enemy.name);
        printf(
            "Loot: +%d gold, +%d wolfskin, +%d onion.\n",
            player->inventory.gold - before_gold,
            player->inventory.wolfskins - before_wolfskins,
            player->inventory.onions - before_onions
        );
        if (enemy.kind == ENEMY_SHREK) {
            puts("The onion is yours. Layers, etc.");
        }
        if (player->level > before_level) {
            printf("LEVEL UP — you are now level %d. STR, STA and MA increased.\n", player->level);
        }
    } else if (result == COMBAT_PLAYER_DIED) {
        player->deaths += 1;
        puts("You died. Town healers drag you back together with no dignity.");
        player_restore_hp(player);
    } else {
        puts("You escaped back toward town.");
    }
}

static void save_menu(Player *player) {
    if (player_save(player, "rpg-save.bin")) {
        puts("Saved to rpg-save.bin");
    } else {
        puts("Save failed.");
    }
}

static void load_menu(Player *player) {
    Player loaded;
    if (player_load(&loaded, "rpg-save.bin")) {
        *player = loaded;
        puts("Loaded rpg-save.bin");
    } else {
        puts("No valid save found.");
    }
}

int main(int argc, char **argv) {
    Rng rng;
    GameRules rules = game_rules(RULES_MODERNIZED);
    int difficulty;
    int budget;
    Stats stats;
    Player player;
    uint64_t seed = (uint64_t)time(NULL);

    if (argc > 1 && strcmp(argv[1], "--legacy-rules") == 0) {
        rules = game_rules(RULES_LEGACY_FAITHFUL);
    }
    if (argc > 2 && strcmp(argv[1], "--seed") == 0) {
        seed = (uint64_t)strtoull(argv[2], NULL, 10);
    }
    rng_seed(&rng, seed);

    puts("Legacy C RPG — faithful modernization of a student text RPG");
    puts("The original fossil remains untouched in legacy-student-rpg-original.c.");
    printf("Rules: %s | RNG seed: %llu\n", rules.profile == RULES_LEGACY_FAITHFUL ? "legacy-faithful" : "modernized", (unsigned long long)seed);

    difficulty = read_int("Choose difficulty 1-4: ", 1, 4);
    budget = skill_points_for_difficulty(difficulty);
    stats = create_stats(budget);
    player = player_create("Çağatay's unfortunate adventurer", stats);

    for (;;) {
        int choice;
        print_player(&player);
        choice = read_int(
            "\nPlains(1), Town(2), Inventory(3), Save(4), Load(5), Quit(6): ",
            1,
            6
        );
        switch (choice) {
            case 1:
                plains(&player, &rng, &rules);
                break;
            case 2:
                town_menu(&player);
                break;
            case 3:
                equipment_menu(&player);
                break;
            case 4:
                save_menu(&player);
                break;
            case 5:
                load_menu(&player);
                break;
            case 6:
                puts("Farewell. Shrek remains in the plains.");
                return 0;
            default:
                break;
        }
    }
}
