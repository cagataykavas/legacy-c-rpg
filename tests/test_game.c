#include "game.h"
#include <assert.h>
#include <stdio.h>

static void test_player_hp_and_leveling(void) {
    Player p = player_create((Stats){2, 2, 3, 1, 1});
    assert(player_max_hp(&p) == 180);
    player_add_experience(&p, 70);
    assert(p.level == 2);
    assert(p.stats.strength == 3);
    assert(p.stats.stamina == 4);
    assert(p.stats.magic == 2);
    assert(p.next_level_xp == 110);
}

static void test_block_reduces_damage(void) {
    Enemy zombie = {ENEMY_ZOMBIE, "Zombie", 200, 4, 10, 50};
    Rng a, b;
    rng_seed(&a, 1234);
    rng_seed(&b, 1234);
    int normal = enemy_attack_damage(&zombie, &a, false);
    int blocked = enemy_attack_damage(&zombie, &b, true);
    assert(blocked == (normal + 1) / 2);
}

static void test_seeded_enemy_roll_is_repeatable(void) {
    Rng a, b;
    rng_seed(&a, 42);
    rng_seed(&b, 42);
    for (int i = 0; i < 20; ++i) {
        assert(roll_enemy(&a).kind == roll_enemy(&b).kind);
    }
}

int main(void) {
    test_player_hp_and_leveling();
    test_block_reduces_damage();
    test_seeded_enemy_roll_is_repeatable();
    puts("all tests passed");
    return 0;
}
