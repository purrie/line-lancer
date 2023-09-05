#include "types.h"
#include "std.h"

implementList(ushort, Ushort)
implementList(usize, Usize)
implementList(Vector2, Vector2)
implementList(Region, Region)
implementList(Path, Path)
implementList(Building, Building)
implementList(Line, Line)
implementList(PlayerData, PlayerData)
implementList(Map, Map)
implementList(MagicEffect, MagicEffect)
implementList(Attack, Attack)
implementList(Path*, PathP)

implementList(Unit*, Unit)
implementList(Region*, RegionP)

char * faction_to_string (FactionType faction) {
    switch (faction) {
        case FACTION_KNIGHTS: return "Knights";
        case FACTION_MAGES: return "Mages";
    }
    TraceLog(LOG_ERROR, "Attempted to stringify unhandled faction");
    return "Error: Unhandled faction type";
}

char * building_type_to_string (BuildingType type) {
    switch (type) {
        case BUILDING_EMPTY:
            return "Empty Building";
        case BUILDING_RESOURCE:
            return "Resource";
        case BUILDING_ARCHER:
            return "Archery";
        case BUILDING_FIGHTER:
            return "Fighters";
        case BUILDING_SUPPORT:
            return "Support";
        case BUILDING_SPECIAL:
            return "Special";
    }
    return "Invalid building type (unhandled)";
}
