#include "game.h"

#include <stdio.h>
#include <string.h>

static const EquipmentPiece ARMOR_CATALOG[RPG_EQUIPMENT_SLOTS] = {
    {SLOT_HEAD, "Wolfskin Headguard", 150, 1, 2, false, false},
    {SLOT_CHEST, "Wolfskin Chestplate", 500, 4, 5, false, false},
    {SLOT_LEGS, "Wolfskin Leggings", 350, 3, 3, false, false},
    {SLOT_BOOTS, "Wolfskin Boots", 250, 2, 1, false, false},
};

void rng_seed(Rng *rng, uint64_t seed) {
    if (rng == NULL) {
        return;
    }
    rng->state = seed ? seed : 0x9e3779b97f4a7c15ULL;
}

uint32_t rng_next(Rng *rng) {
    uint64_t x;
    if (rng == NULL) {
        return 0U;
    }
    x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return (uint32_t)((x * 2685821657736338717ULL) >> 32);
}

int rng_range(Rng *rng, int min_inclusive, int max_inclusive) {
    uint32_t span;
    if (max_inclusive < min_inclusive) {
        int temporary = min_inclusive;
        min_inclusive = max_inclusive;
        max_inclusive = temporary;
    }
    span = (uint32_t)(max_inclusive - min_inclusive + 1);
    return min_inclusive + (int)(rng_next(rng) % span);
}

GameRules game_rules(RulesProfile profile) {
    GameRules rules;
    rules.profile = profile;
    if (profile == RULES_LEGACY_FAITHFUL) {
        /*
         * The 2020-era student program offered a block action but never handled
         * case 2, allowed healing above max HP, and reset XP to zero on level-up.
         * This profile preserves those quirks for regression experiments.
         */
        rules.block_reduces_damage = false;
        rules.clamp_heal_to_max_hp = false;
        rules.carry_excess_xp = false;
        rules.award_shrek_xp = false;
    } else {
        rules.block_reduces_damage = true;
        rules.clamp_heal_to_max_hp = true;
        rules.carry_excess_xp = true;
        rules.award_shrek_xp = false;
    }
    return rules;
}

int skill_points_for_difficulty(int difficulty) {
    if (difficulty < 1 || difficulty > 4) {
        return -1;
    }
    return (5 - difficulty) * 5;
}

bool stats_fit_budget(Stats stats, int budget) {
    const int values[] = {
        stats.strength,
        stats.agility,
        stats.stamina,
        stats.luck,
        stats.magic,
    };
    int total = 0;
    size_t index;
    if (budget < 0) {
        return false;
    }
    for (index = 0; index < sizeof(values) / sizeof(values[0]); ++index) {
        if (values[index] < 0) {
            return false;
        }
        total += values[index];
    }
    return total <= budget;
}

Player player_create(const char *name, Stats stats) {
    Player player;
    size_t index;
    memset(&player, 0, sizeof(player));
    if (name == NULL || name[0] == '\0') {
        name = "Nameless Adventurer";
    }
    snprintf(player.name, sizeof(player.name), "%s", name);
    player.base_stats = stats;
    player.level = 1;
    player.next_level_xp = 70;
    for (index = 0; index < RPG_EQUIPMENT_SLOTS; ++index) {
        player.equipment[index] = ARMOR_CATALOG[index];
    }
    player_restore_hp(&player);
    return player;
}

int player_equipment_stamina_bonus(const Player *player) {
    int total = 0;
    size_t index;
    if (player == NULL) {
        return 0;
    }
    for (index = 0; index < RPG_EQUIPMENT_SLOTS; ++index) {
        if (player->equipment[index].owned && player->equipment[index].equipped) {
            total += player->equipment[index].stamina_bonus;
        }
    }
    return total;
}

int player_effective_stamina(const Player *player) {
    if (player == NULL) {
        return 0;
    }
    return player->base_stats.stamina + player_equipment_stamina_bonus(player);
}

int player_max_hp(const Player *player) {
    const int stamina = player_effective_stamina(player);
    return stamina > 0 ? stamina * 60 : 1;
}

void player_restore_hp(Player *player) {
    if (player != NULL) {
        player->hp = player_max_hp(player);
    }
}

void player_add_experience(Player *player, int amount, const GameRules *rules) {
    if (player == NULL || amount <= 0 || rules == NULL) {
        return;
    }
    player->experience += amount;
    while (player->experience >= player->next_level_xp) {
        if (rules->carry_excess_xp) {
            player->experience -= player->next_level_xp;
        } else {
            player->experience = 0;
        }
        player->level += 1;
        player->base_stats.strength += 1;
        player->base_stats.stamina += 1;
        player->base_stats.magic += 1;
        player->next_level_xp += 40;
        player_restore_hp(player);
        if (!rules->carry_excess_xp) {
            break;
        }
    }
}

const EquipmentPiece *player_equipment(const Player *player, EquipmentSlot slot) {
    if (player == NULL || slot < SLOT_HEAD || slot > SLOT_BOOTS) {
        return NULL;
    }
    return &player->equipment[(size_t)slot];
}

EquipmentPiece *player_equipment_mut(Player *player, EquipmentSlot slot) {
    if (player == NULL || slot < SLOT_HEAD || slot > SLOT_BOOTS) {
        return NULL;
    }
    return &player->equipment[(size_t)slot];
}

bool player_toggle_equipment(Player *player, EquipmentSlot slot) {
    EquipmentPiece *piece = player_equipment_mut(player, slot);
    int previous_max;
    if (piece == NULL || !piece->owned) {
        return false;
    }
    previous_max = player_max_hp(player);
    piece->equipped = !piece->equipped;
    if (piece->equipped) {
        player->hp += player_max_hp(player) - previous_max;
    } else if (player->hp > player_max_hp(player)) {
        player->hp = player_max_hp(player);
    }
    return true;
}

Enemy enemy_template(EnemyKind kind) {
    switch (kind) {
        case ENEMY_ZOMBIE:
            return (Enemy){
                ENEMY_ZOMBIE,
                "Zombie",
                200,
                200,
                1,
                10,
                59,
                0,
                0,
            };
        case ENEMY_WEREWOLF:
            return (Enemy){
                ENEMY_WEREWOLF,
                "Werewolf",
                400,
                400,
                8,
                30,
                79,
                1,
                0,
            };
        case ENEMY_SHREK:
        default:
            return (Enemy){
                ENEMY_SHREK,
                "Mighty .:SHREK:.",
                800,
                800,
                15,
                0,
                0,
                0,
                1,
            };
    }
}

Enemy roll_enemy(Rng *rng) {
    const int roll = rng_range(rng, 1, 100);
    if (roll <= 70) {
        return enemy_template(ENEMY_ZOMBIE);
    }
    if (roll <= 95) {
        return enemy_template(ENEMY_WEREWOLF);
    }
    return enemy_template(ENEMY_SHREK);
}

const char *enemy_kind_name(EnemyKind kind) {
    switch (kind) {
        case ENEMY_ZOMBIE:
            return "Zombie";
        case ENEMY_WEREWOLF:
            return "Werewolf";
        case ENEMY_SHREK:
            return "Mighty .:SHREK:.";
        default:
            return "Unknown";
    }
}

const char *combat_result_name(CombatResult result) {
    switch (result) {
        case COMBAT_PLAYER_WON:
            return "victory";
        case COMBAT_PLAYER_DIED:
            return "defeat";
        case COMBAT_PLAYER_FLED:
            return "fled";
        case COMBAT_CONTINUES:
        default:
            return "continues";
    }
}

void player_format_summary(const Player *player, char *buffer, size_t capacity) {
    if (player == NULL || buffer == NULL || capacity == 0U) {
        return;
    }
    snprintf(
        buffer,
        capacity,
        "%s | Lv %d | HP %d/%d | STR %d AGI %d STA %d(+%d) LU %d MA %d | "
        "XP %d/%d | Gold %d | Wolfskins %d | Onions %d | Wins %d",
        player->name,
        player->level,
        player->hp,
        player_max_hp(player),
        player->base_stats.strength,
        player->base_stats.agility,
        player->base_stats.stamina,
        player_equipment_stamina_bonus(player),
        player->base_stats.luck,
        player->base_stats.magic,
        player->experience,
        player->next_level_xp,
        player->inventory.gold,
        player->inventory.wolfskins,
        player->inventory.onions,
        player->victories
    );
}
