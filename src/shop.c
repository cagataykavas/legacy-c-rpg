#include "game.h"

#include <stddef.h>

static const EquipmentPiece CATALOG[RPG_EQUIPMENT_SLOTS] = {
    {SLOT_HEAD, "Wolfskin Headguard", 150, 1, 2, false, false},
    {SLOT_CHEST, "Wolfskin Chestplate", 500, 4, 5, false, false},
    {SLOT_LEGS, "Wolfskin Leggings", 350, 3, 3, false, false},
    {SLOT_BOOTS, "Wolfskin Boots", 250, 2, 1, false, false},
};

const EquipmentPiece *shop_catalog(size_t *count) {
    if (count != NULL) {
        *count = RPG_EQUIPMENT_SLOTS;
    }
    return CATALOG;
}

PurchaseResult shop_purchase_armor(Player *player, EquipmentSlot slot) {
    EquipmentPiece *owned;
    const EquipmentPiece *item;

    if (player == NULL || slot < SLOT_HEAD || slot > SLOT_BOOTS) {
        return (PurchaseResult){false, "Invalid armor selection."};
    }

    item = &CATALOG[(size_t)slot];
    owned = player_equipment_mut(player, slot);
    if (owned == NULL) {
        return (PurchaseResult){false, "Invalid equipment slot."};
    }
    if (owned->owned) {
        return (PurchaseResult){false, "You already own that armor piece."};
    }
    if (player->inventory.gold < item->gold_cost) {
        return (PurchaseResult){false, "Not enough gold."};
    }
    if (player->inventory.wolfskins < item->wolfskin_cost) {
        return (PurchaseResult){false, "Not enough wolfskins."};
    }

    player->inventory.gold -= item->gold_cost;
    player->inventory.wolfskins -= item->wolfskin_cost;
    *owned = *item;
    owned->owned = true;
    owned->equipped = false;
    return (PurchaseResult){true, "Armor purchased."};
}
