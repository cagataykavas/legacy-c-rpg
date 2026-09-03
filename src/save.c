#include "game.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t version;
    char name[RPG_MAX_NAME];
    Stats base_stats;
    Inventory inventory;
    int level;
    int experience;
    int next_level_xp;
    int hp;
    int victories;
    int retreats;
    int deaths;
    uint8_t owned[RPG_EQUIPMENT_SLOTS];
    uint8_t equipped[RPG_EQUIPMENT_SLOTS];
} SaveRecord;

static SaveRecord player_to_record(const Player *player) {
    SaveRecord record;
    size_t index;
    memset(&record, 0, sizeof(record));
    record.version = RPG_SAVE_VERSION;
    snprintf(record.name, sizeof(record.name), "%s", player->name);
    record.base_stats = player->base_stats;
    record.inventory = player->inventory;
    record.level = player->level;
    record.experience = player->experience;
    record.next_level_xp = player->next_level_xp;
    record.hp = player->hp;
    record.victories = player->victories;
    record.retreats = player->retreats;
    record.deaths = player->deaths;
    for (index = 0; index < RPG_EQUIPMENT_SLOTS; ++index) {
        record.owned[index] = player->equipment[index].owned ? 1U : 0U;
        record.equipped[index] = player->equipment[index].equipped ? 1U : 0U;
    }
    return record;
}

static bool record_is_valid(const SaveRecord *record) {
    size_t index;
    if (record->version != RPG_SAVE_VERSION) {
        return false;
    }
    if (record->level < 1 || record->next_level_xp < 70 || record->experience < 0) {
        return false;
    }
    if (record->inventory.gold < 0 || record->inventory.wolfskins < 0 ||
        record->inventory.onions < 0 || record->inventory.switchblades < 0) {
        return false;
    }
    if (record->base_stats.strength < 0 || record->base_stats.agility < 0 ||
        record->base_stats.stamina < 0 || record->base_stats.luck < 0 ||
        record->base_stats.magic < 0) {
        return false;
    }
    for (index = 0; index < RPG_EQUIPMENT_SLOTS; ++index) {
        if (record->equipped[index] != 0U && record->owned[index] == 0U) {
            return false;
        }
    }
    return true;
}

bool player_save(const Player *player, const char *path) {
    FILE *file;
    SaveRecord record;
    if (player == NULL || path == NULL || path[0] == '\0') {
        return false;
    }
    record = player_to_record(player);
    file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    if (fwrite(&record, sizeof(record), 1U, file) != 1U) {
        fclose(file);
        return false;
    }
    return fclose(file) == 0;
}

bool player_load(Player *player, const char *path) {
    FILE *file;
    SaveRecord record;
    Player restored;
    size_t index;
    if (player == NULL || path == NULL || path[0] == '\0') {
        return false;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    if (fread(&record, sizeof(record), 1U, file) != 1U) {
        fclose(file);
        return false;
    }
    if (fclose(file) != 0 || !record_is_valid(&record)) {
        return false;
    }

    restored = player_create(record.name, record.base_stats);
    restored.inventory = record.inventory;
    restored.level = record.level;
    restored.experience = record.experience;
    restored.next_level_xp = record.next_level_xp;
    restored.victories = record.victories;
    restored.retreats = record.retreats;
    restored.deaths = record.deaths;
    for (index = 0; index < RPG_EQUIPMENT_SLOTS; ++index) {
        restored.equipment[index].owned = record.owned[index] != 0U;
        restored.equipment[index].equipped = record.equipped[index] != 0U;
    }
    restored.hp = record.hp;
    if (restored.hp < 0) {
        restored.hp = 0;
    }
    if (restored.hp > player_max_hp(&restored)) {
        restored.hp = player_max_hp(&restored);
    }
    *player = restored;
    return true;
}
