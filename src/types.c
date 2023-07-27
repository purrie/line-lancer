#include "types.h"
#include "std.h"

implementList(ushort, Ushort)
implementList(usize, Usize)
implementList(Vector2, Vector2)
implementList(Region, Region)
implementList(Path, Path)
implementList(PathEntry, PathEntry)
implementList(Building, Building)
implementList(Line, Line)
implementList(PathBridge, PathBridge)
implementList(Bridge, Bridge)
implementList(PlayerData, PlayerData)
implementList(Map, Map)
implementList(MagicEffect, MagicEffect)

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
