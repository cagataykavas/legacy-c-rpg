#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RPG_EQUIPMENT_SLOTS 4
#define RPG_MAX_NAME 48
#define RPG_SAVE_VERSION 1

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
    int switchblades;
} Inventory;

typedef enum {
    SLOT_HEAD = 0,
    SLOT_CHEST = 1,
    SLOT_LEGS = 2,
    SLOT_BOOTS = 3
} EquipmentSlot;

typedef struct {
    EquipmentSlot slot;
    const char *name;
    int gold_cost;
    int wolfskin_cost;
    int stamina_bonus;
    bool owned;
    bool equipped;
} EquipmentPiece;

typedef struct {
    char name[RPG_MAX_NAME];
    Stats base_stats;
    Inventory inventory;
    EquipmentPiece equipment[RPG_EQUIPMENT_SLOTS];
    int level;
    int experience;
    int next_level_xp;
    int hp;
    int victories;
    int retreats;
    int deaths;
} Player;

typedef enum {
    ENEMY_ZOMBIE = 0,
    ENEMY_WEREWOLF = 1,
    ENEMY_SHREK = 2
} EnemyKind;

typedef struct {
    EnemyKind kind;
    const char *name;
    int max_hp;
    int hp;
    int attack;
    int xp_reward;
    int max_gold_reward;
    int wolfskin_reward;
    int onion_reward;
} Enemy;

typedef struct {
    uint64_t state;
} Rng;

typedef enum {
    COMBAT_ATTACK = 1,
    COMBAT_BLOCK = 2,
    COMBAT_HEAL = 3,
    COMBAT_RUN = 4
} CombatAction;

typedef enum {
    COMBAT_CONTINUES = 0,
    COMBAT_PLAYER_WON = 1,
    COMBAT_PLAYER_DIED = 2,
    COMBAT_PLAYER_FLED = 3
} CombatResult;

typedef struct {
    int player_damage;
    int enemy_damage;
    int healing;
    bool blocked;
    bool fled;
    CombatResult result;
} CombatTurn;

typedef struct {
    bool purchased;
    const char *message;
} PurchaseResult;

typedef enum {
    RULES_MODERNIZED = 0,
    RULES_LEGACY_FAITHFUL = 1
} RulesProfile;

typedef struct {
    RulesProfile profile;
    bool block_reduces_damage;
    bool clamp_heal_to_max_hp;
    bool carry_excess_xp;
    bool award_shrek_xp;
} GameRules;

/* RNG */
void rng_seed(Rng *rng, uint64_t seed);
uint32_t rng_next(Rng *rng);
int rng_range(Rng *rng, int min_inclusive, int max_inclusive);

/* Rules and creation */
GameRules game_rules(RulesProfile profile);
int skill_points_for_difficulty(int difficulty);
bool stats_fit_budget(Stats stats, int budget);
Player player_create(const char *name, Stats stats);

/* Player and inventory */
int player_equipment_stamina_bonus(const Player *player);
int player_effective_stamina(const Player *player);
int player_max_hp(const Player *player);
void player_restore_hp(Player *player);
void player_add_experience(Player *player, int amount, const GameRules *rules);
const EquipmentPiece *player_equipment(const Player *player, EquipmentSlot slot);
EquipmentPiece *player_equipment_mut(Player *player, EquipmentSlot slot);
bool player_toggle_equipment(Player *player, EquipmentSlot slot);

/* Enemies, combat, rewards */
Enemy enemy_template(EnemyKind kind);
Enemy roll_enemy(Rng *rng);
int player_attack_damage(const Player *player, Rng *rng);
int enemy_attack_damage(const Enemy *enemy, Rng *rng, bool blocking, const GameRules *rules);
int player_heal_amount(const Player *player, Rng *rng);
CombatTurn combat_take_turn(
    Player *player,
    Enemy *enemy,
    CombatAction action,
    Rng *rng,
    const GameRules *rules
);
void award_enemy_rewards(Player *player, const Enemy *enemy, Rng *rng, const GameRules *rules);

/* Town / shop */
const EquipmentPiece *shop_catalog(size_t *count);
PurchaseResult shop_purchase_armor(Player *player, EquipmentSlot slot);

/* Save / load */
bool player_save(const Player *player, const char *path);
bool player_load(Player *player, const char *path);

/* Presentation helpers */
const char *enemy_kind_name(EnemyKind kind);
const char *combat_result_name(CombatResult result);
void player_format_summary(const Player *player, char *buffer, size_t capacity);

#endif
