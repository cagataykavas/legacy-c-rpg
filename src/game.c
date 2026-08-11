#include "game.h"

static int equipment_stamina_bonus(const Player *player) {
    int total = 0;
    for (int i = 0; i < 4; ++i) {
        if (player->equipment[i].equipped) {
            total += player->equipment[i].stamina_bonus;
        }
    }
    return total;
}

void rng_seed(Rng *rng, uint64_t seed) {
    rng->state = seed ? seed : 0x9e3779b97f4a7c15ULL;
}

uint32_t rng_next(Rng *rng) {
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return (uint32_t)((x * 2685821657736338717ULL) >> 32);
}

int rng_range(Rng *rng, int min_inclusive, int max_inclusive) {
    const uint32_t span = (uint32_t)(max_inclusive - min_inclusive + 1);
    return min_inclusive + (int)(rng_next(rng) % span);
}

Player player_create(Stats stats) {
    Player player = {0};
    player.stats = stats;
    player.level = 1;
    player.next_level_xp = 70;
    player.equipment[SLOT_HEAD].stamina_bonus = 2;
    player.equipment[SLOT_CHEST].stamina_bonus = 5;
    player.equipment[SLOT_LEGS].stamina_bonus = 3;
    player.equipment[SLOT_BOOTS].stamina_bonus = 1;
    player_restore_hp(&player);
    return player;
}

int player_max_hp(const Player *player) {
    return 60 * (player->stats.stamina + equipment_stamina_bonus(player));
}

void player_restore_hp(Player *player) {
    player->hp = player_max_hp(player);
}

void player_add_experience(Player *player, int amount) {
    player->experience += amount;
    while (player->experience >= player->next_level_xp) {
        player->experience -= player->next_level_xp;
        player->level += 1;
        player->stats.strength += 1;
        player->stats.stamina += 1;
        player->stats.magic += 1;
        player->next_level_xp += 40;
        player_restore_hp(player);
    }
}

Enemy roll_enemy(Rng *rng) {
    int roll = rng_range(rng, 1, 100);
    if (roll <= 70) {
        return (Enemy){ENEMY_ZOMBIE, "Zombie", 200, 1, 10, 59};
    }
    if (roll <= 95) {
        return (Enemy){ENEMY_WEREWOLF, "Werewolf", 400, 8, 30, 79};
    }
    return (Enemy){ENEMY_SHREK, "Mighty .:SHREK:.", 800, 15, 0, 0};
}

int player_attack_damage(const Player *player, Rng *rng) {
    return player->stats.strength * rng_range(rng, 1, 10);
}

int enemy_attack_damage(const Enemy *enemy, Rng *rng, bool blocking) {
    int damage = enemy->attack * rng_range(rng, 1, 15);
    if (blocking) {
        damage = (damage + 1) / 2;
    }
    return damage;
}

int player_heal_amount(const Player *player, Rng *rng) {
    return player->stats.magic * rng_range(rng, 1, 20);
}
