#include "game.h"

#include <stdbool.h>

int player_attack_damage(const Player *player, Rng *rng) {
    if (player == NULL || rng == NULL) {
        return 0;
    }
    return player->base_stats.strength * rng_range(rng, 1, 10);
}

int enemy_attack_damage(
    const Enemy *enemy,
    Rng *rng,
    bool blocking,
    const GameRules *rules
) {
    int damage;
    if (enemy == NULL || rng == NULL || rules == NULL) {
        return 0;
    }
    damage = enemy->attack * rng_range(rng, 1, 15);
    if (blocking && rules->block_reduces_damage) {
        damage = (damage + 1) / 2;
    }
    return damage;
}

int player_heal_amount(const Player *player, Rng *rng) {
    if (player == NULL || rng == NULL) {
        return 0;
    }
    return player->base_stats.magic * rng_range(rng, 1, 20);
}

static void clamp_enemy_hp(Enemy *enemy) {
    if (enemy->hp < 0) {
        enemy->hp = 0;
    }
}

static void clamp_player_hp(Player *player) {
    if (player->hp < 0) {
        player->hp = 0;
    }
}

static void apply_heal(Player *player, int amount, const GameRules *rules) {
    int next_hp;
    if (amount <= 0) {
        return;
    }
    next_hp = player->hp + amount;
    if (rules->clamp_heal_to_max_hp && next_hp > player_max_hp(player)) {
        next_hp = player_max_hp(player);
    }
    player->hp = next_hp;
}

CombatTurn combat_take_turn(
    Player *player,
    Enemy *enemy,
    CombatAction action,
    Rng *rng,
    const GameRules *rules
) {
    CombatTurn turn = {0};
    bool blocking = false;

    if (player == NULL || enemy == NULL || rng == NULL || rules == NULL) {
        turn.result = COMBAT_PLAYER_DIED;
        return turn;
    }
    if (player->hp <= 0) {
        turn.result = COMBAT_PLAYER_DIED;
        return turn;
    }
    if (enemy->hp <= 0) {
        turn.result = COMBAT_PLAYER_WON;
        return turn;
    }

    switch (action) {
        case COMBAT_ATTACK:
            turn.player_damage = player_attack_damage(player, rng);
            enemy->hp -= turn.player_damage;
            clamp_enemy_hp(enemy);
            break;
        case COMBAT_BLOCK:
            blocking = true;
            turn.blocked = rules->block_reduces_damage;
            break;
        case COMBAT_HEAL:
            turn.healing = player_heal_amount(player, rng);
            apply_heal(player, turn.healing, rules);
            break;
        case COMBAT_RUN:
            turn.fled = true;
            turn.result = COMBAT_PLAYER_FLED;
            player->retreats += 1;
            return turn;
        default:
            break;
    }

    if (enemy->hp <= 0) {
        turn.result = COMBAT_PLAYER_WON;
        return turn;
    }

    /*
     * This intentionally matches the original turn ordering: even after a heal
     * or no-op/block selection, the enemy takes its turn. The modern profile
     * merely makes the previously unimplemented block command meaningful.
     */
    turn.enemy_damage = enemy_attack_damage(enemy, rng, blocking, rules);
    player->hp -= turn.enemy_damage;
    clamp_player_hp(player);

    if (player->hp <= 0) {
        turn.result = COMBAT_PLAYER_DIED;
    } else {
        turn.result = COMBAT_CONTINUES;
    }
    return turn;
}

static int reward_gold(const Enemy *enemy, Rng *rng) {
    if (enemy->max_gold_reward <= 0) {
        return 0;
    }
    return rng_range(rng, 0, enemy->max_gold_reward);
}

static void maybe_drop_switchblade(Player *player, const Enemy *enemy, Rng *rng) {
    int legacy_roll;
    int legacy_n;

    /*
     * Original code used: acdc = rand()%20*n*n; if (acdc>80) SWITCHblade++.
     * n was 1=zombie, 2=werewolf, 3=Shrek. Recreate that odd drop table here.
     */
    legacy_n = (int)enemy->kind + 1;
    legacy_roll = rng_range(rng, 0, 19) * legacy_n * legacy_n;
    if (legacy_roll > 80) {
        player->inventory.switchblades += 1;
    }
}

void award_enemy_rewards(
    Player *player,
    const Enemy *enemy,
    Rng *rng,
    const GameRules *rules
) {
    int xp;
    if (player == NULL || enemy == NULL || rng == NULL || rules == NULL) {
        return;
    }
    if (enemy->hp > 0) {
        return;
    }

    player->inventory.gold += reward_gold(enemy, rng);
    player->inventory.wolfskins += enemy->wolfskin_reward;
    player->inventory.onions += enemy->onion_reward;

    xp = enemy->xp_reward;
    if (enemy->kind == ENEMY_SHREK && rules->award_shrek_xp) {
        xp = 50;
    }
    player_add_experience(player, xp, rules);
    maybe_drop_switchblade(player, enemy, rng);
    player->victories += 1;
}
