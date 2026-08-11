#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int strength;
    int agility;
    int stamina;
    int luck;
    int magic;
} Stats;

typedef struct {
    int gold;
    int wolfskins;
    int onions;
} Inventory;

typedef enum {
    SLOT_HEAD,
    SLOT_CHEST,
    SLOT_LEGS,
    SLOT_BOOTS
} EquipmentSlot;

typedef struct {
    bool owned;
    bool equipped;
    int stamina_bonus;
} EquipmentPiece;

typedef struct {
    Stats stats;
    Inventory inventory;
    EquipmentPiece equipment[4];
    int level;
    int experience;
    int next_level_xp;
    int hp;
} Player;

typedef enum {
    ENEMY_ZOMBIE,
    ENEMY_WEREWOLF,
    ENEMY_SHREK
} EnemyKind;

typedef struct {
    EnemyKind kind;
    const char *name;
    int hp;
    int attack;
    int xp_reward;
    int max_gold_reward;
} Enemy;

typedef struct {
    uint64_t state;
} Rng;

void rng_seed(Rng *rng, uint64_t seed);
uint32_t rng_next(Rng *rng);
int rng_range(Rng *rng, int min_inclusive, int max_inclusive);

Player player_create(Stats stats);
int player_max_hp(const Player *player);
void player_restore_hp(Player *player);
void player_add_experience(Player *player, int amount);

Enemy roll_enemy(Rng *rng);
int player_attack_damage(const Player *player, Rng *rng);
int enemy_attack_damage(const Enemy *enemy, Rng *rng, bool blocking);
int player_heal_amount(const Player *player, Rng *rng);

#endif
