#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_difficulty_budget(void) {
    assert(skill_points_for_difficulty(1) == 20);
    assert(skill_points_for_difficulty(2) == 15);
    assert(skill_points_for_difficulty(3) == 10);
    assert(skill_points_for_difficulty(4) == 5);
    assert(skill_points_for_difficulty(0) == -1);
    assert(skill_points_for_difficulty(5) == -1);

    assert(stats_fit_budget((Stats){4, 4, 4, 4, 4}, 20));
    assert(!stats_fit_budget((Stats){5, 5, 5, 5, 1}, 20));
    assert(!stats_fit_budget((Stats){-1, 0, 0, 0, 0}, 20));
}

static void test_enemy_templates_match_original(void) {
    Enemy zombie = enemy_template(ENEMY_ZOMBIE);
    Enemy wolf = enemy_template(ENEMY_WEREWOLF);
    Enemy shrek = enemy_template(ENEMY_SHREK);

    assert(strcmp(zombie.name, "Zombie") == 0);
    assert(zombie.max_hp == 200);
    assert(zombie.attack == 1);
    assert(zombie.xp_reward == 10);
    assert(zombie.max_gold_reward == 59);

    assert(strcmp(wolf.name, "Werewolf") == 0);
    assert(wolf.max_hp == 400);
    assert(wolf.attack == 8);
    assert(wolf.xp_reward == 30);
    assert(wolf.max_gold_reward == 79);
    assert(wolf.wolfskin_reward == 1);

    assert(strcmp(shrek.name, "Mighty .:SHREK:.") == 0);
    assert(shrek.max_hp == 800);
    assert(shrek.attack == 15);
    assert(shrek.xp_reward == 0);
    assert(shrek.onion_reward == 1);
}

static void test_player_hp_and_equipment(void) {
    Player player = player_create("tester", (Stats){2, 1, 3, 1, 2});
    assert(player_max_hp(&player) == 180);
    assert(player.hp == 180);

    player.inventory.gold = 1000;
    player.inventory.wolfskins = 10;
    assert(shop_purchase_armor(&player, SLOT_CHEST).purchased);
    assert(player.inventory.gold == 500);
    assert(player.inventory.wolfskins == 6);
    assert(player.equipment[SLOT_CHEST].owned);
    assert(!player.equipment[SLOT_CHEST].equipped);

    assert(player_toggle_equipment(&player, SLOT_CHEST));
    assert(player.equipment[SLOT_CHEST].equipped);
    assert(player_effective_stamina(&player) == 8);
    assert(player_max_hp(&player) == 480);
    assert(player.hp == 480);

    assert(player_toggle_equipment(&player, SLOT_CHEST));
    assert(!player.equipment[SLOT_CHEST].equipped);
    assert(player_max_hp(&player) == 180);
    assert(player.hp == 180);
}

static void test_shop_prices_match_original(void) {
    size_t count = 0;
    const EquipmentPiece *catalog = shop_catalog(&count);
    assert(count == 4);
    assert(strcmp(catalog[SLOT_HEAD].name, "Wolfskin Headguard") == 0);
    assert(catalog[SLOT_HEAD].gold_cost == 150);
    assert(catalog[SLOT_HEAD].wolfskin_cost == 1);
    assert(catalog[SLOT_HEAD].stamina_bonus == 2);
    assert(catalog[SLOT_CHEST].gold_cost == 500);
    assert(catalog[SLOT_CHEST].wolfskin_cost == 4);
    assert(catalog[SLOT_CHEST].stamina_bonus == 5);
    assert(catalog[SLOT_LEGS].gold_cost == 350);
    assert(catalog[SLOT_LEGS].wolfskin_cost == 3);
    assert(catalog[SLOT_LEGS].stamina_bonus == 3);
    assert(catalog[SLOT_BOOTS].gold_cost == 250);
    assert(catalog[SLOT_BOOTS].wolfskin_cost == 2);
    assert(catalog[SLOT_BOOTS].stamina_bonus == 1);
}

static void test_level_progression_profiles(void) {
    Player modern = player_create("modern", (Stats){1, 1, 2, 1, 1});
    Player legacy = player_create("legacy", (Stats){1, 1, 2, 1, 1});
    GameRules modern_rules = game_rules(RULES_MODERNIZED);
    GameRules legacy_rules = game_rules(RULES_LEGACY_FAITHFUL);

    player_add_experience(&modern, 75, &modern_rules);
    assert(modern.level == 2);
    assert(modern.experience == 5);
    assert(modern.next_level_xp == 110);
    assert(modern.base_stats.strength == 2);
    assert(modern.base_stats.stamina == 3);
    assert(modern.base_stats.magic == 2);

    player_add_experience(&legacy, 75, &legacy_rules);
    assert(legacy.level == 2);
    assert(legacy.experience == 0);
    assert(legacy.next_level_xp == 110);
}

static void test_shrek_reward_is_onion(void) {
    Player player = player_create("onion collector", (Stats){1, 1, 2, 1, 1});
    Enemy shrek = enemy_template(ENEMY_SHREK);
    GameRules rules = game_rules(RULES_MODERNIZED);
    Rng rng;
    rng_seed(&rng, 42);
    shrek.hp = 0;

    award_enemy_rewards(&player, &shrek, &rng, &rules);
    assert(player.inventory.onions == 1);
    assert(player.experience == 0);
    assert(player.inventory.gold == 0);
    assert(player.inventory.wolfskins == 0);
    assert(player.victories == 1);
}

static void test_modern_block_reduces_damage(void) {
    Player player_a = player_create("a", (Stats){1, 1, 20, 1, 1});
    Player player_b = player_a;
    Enemy enemy_a = enemy_template(ENEMY_WEREWOLF);
    Enemy enemy_b = enemy_a;
    GameRules modern = game_rules(RULES_MODERNIZED);
    GameRules legacy = game_rules(RULES_LEGACY_FAITHFUL);
    Rng rng_a;
    Rng rng_b;
    CombatTurn blocked;
    CombatTurn legacy_block;

    rng_seed(&rng_a, 12345);
    rng_seed(&rng_b, 12345);
    blocked = combat_take_turn(&player_a, &enemy_a, COMBAT_BLOCK, &rng_a, &modern);
    legacy_block = combat_take_turn(&player_b, &enemy_b, COMBAT_BLOCK, &rng_b, &legacy);

    assert(blocked.blocked);
    assert(!legacy_block.blocked);
    assert(blocked.enemy_damage <= legacy_block.enemy_damage);
    assert(legacy_block.enemy_damage == blocked.enemy_damage * 2 ||
           legacy_block.enemy_damage == blocked.enemy_damage * 2 - 1);
}

static void test_combat_attack_can_win_without_retaliation(void) {
    Player player = player_create("strong", (Stats){1000, 1, 2, 1, 1});
    Enemy zombie = enemy_template(ENEMY_ZOMBIE);
    GameRules rules = game_rules(RULES_MODERNIZED);
    Rng rng;
    CombatTurn turn;
    rng_seed(&rng, 1);
    turn = combat_take_turn(&player, &zombie, COMBAT_ATTACK, &rng, &rules);
    assert(turn.result == COMBAT_PLAYER_WON);
    assert(zombie.hp == 0);
    assert(turn.enemy_damage == 0);
}

static void test_save_round_trip(void) {
    const char *path = "test-rpg-save.bin";
    Player original = player_create("save tester", (Stats){3, 2, 4, 1, 5});
    Player restored;

    original.inventory.gold = 777;
    original.inventory.wolfskins = 9;
    original.inventory.onions = 2;
    original.inventory.switchblades = 1;
    original.level = 4;
    original.experience = 33;
    original.next_level_xp = 190;
    original.victories = 12;
    assert(shop_purchase_armor(&original, SLOT_HEAD).purchased);
    assert(player_toggle_equipment(&original, SLOT_HEAD));
    original.hp -= 17;

    assert(player_save(&original, path));
    assert(player_load(&restored, path));
    assert(strcmp(restored.name, original.name) == 0);
    assert(restored.inventory.gold == original.inventory.gold);
    assert(restored.inventory.onions == original.inventory.onions);
    assert(restored.level == original.level);
    assert(restored.experience == original.experience);
    assert(restored.victories == original.victories);
    assert(restored.equipment[SLOT_HEAD].owned);
    assert(restored.equipment[SLOT_HEAD].equipped);
    assert(restored.hp == original.hp);
    remove(path);
}

static void test_summary_contains_shrek_era_inventory(void) {
    Player player = player_create("summary", (Stats){1, 2, 3, 4, 5});
    char buffer[512];
    player.inventory.onions = 3;
    player.inventory.wolfskins = 2;
    player_format_summary(&player, buffer, sizeof(buffer));
    assert(strstr(buffer, "Onions 3") != NULL);
    assert(strstr(buffer, "Wolfskins 2") != NULL);
}

int main(void) {
    test_difficulty_budget();
    test_enemy_templates_match_original();
    test_player_hp_and_equipment();
    test_shop_prices_match_original();
    test_level_progression_profiles();
    test_shrek_reward_is_onion();
    test_modern_block_reduces_damage();
    test_combat_attack_can_win_without_retaliation();
    test_save_round_trip();
    test_summary_contains_shrek_era_inventory();
    puts("All RPG regression tests passed.");
    return 0;
}
