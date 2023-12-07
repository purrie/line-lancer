#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#include "mesh.h"
#include "math.h"
#include "assets.h"
#include "std.h"
#include "constants.h"
#include "alloc.h"
#include "ui.h"
#include "animation.h"

#define JSMN_PARENT_LINKS
#include "../vendor/jsmn.h"

typedef struct {
    uchar * start;
    usize   len;
} StringSlice;

void unload_animations (Assets * assets);

/* String Handling ***********************************************************/
void log_slice(TraceLogLevel log_level, char * text, StringSlice slice) {
    char s[slice.len + 1];
    copy_memory(s, slice.start, sizeof(char) * slice.len);
    s[slice.len] = '\0';
    TraceLog(log_level, "%s %s", text, s);
}
StringSlice make_slice_u(uchar * from, usize start, usize end) {
    StringSlice s;
    s.start = from + start;
    s.len = end - start;
    return s;
}
StringSlice split_backwards(StringSlice text, usize * in_out_from, uchar split) {
    usize from = *in_out_from;
    if (text.len <= from) {
        from = text.len;
    }
    StringSlice result = {0};
    if (from == 0) {
        return result;
    }

    int offset = 0;
    while (from --> 0) {
        if (text.start[from] == PATH_SEPARATOR) {
            offset = 1;
            break;
        }
        if (text.start[from] == split) {
            offset = 1;
            break;
        }
    }
    result.start = &text.start[from + offset];
    result.len = *in_out_from - from;
    if (from > 0)
      *in_out_from = from - 1;
    else
      *in_out_from = from;

    return result;
}
bool compare_literal(StringSlice slice, const char * literal) {
    usize i = 0;
    for(; i < slice.len; i++) {
        if (literal[i] == '\0') {
            return false;
        }
        if (slice.start[i] != literal[i]) {
            return false;
        }
    }
    return literal[i] == '\0';
}
Result convert_slice_usize(StringSlice slice, usize * value) {
    bool first = true;
    for (usize i = 0; i < slice.len; i++) {
        if (slice.start[i] >= '0' && slice.start[i] <= '9') {
            if (first == false) {
                *value *= 10;
                *value += slice.start[i] - '0';
            } else {
                *value = slice.start[i] - '0';
                first = false;
            }
        } else {
            return FAILURE;
        }
    }
    return SUCCESS;
}
Result convert_slice_float(StringSlice slice, float * value) {
    int dot = -1;
    bool neg = false;
    bool first = true;
    for (usize i = 0; i < slice.len; i++) {
        uchar c = slice.start[i];
        if (c >= '0' && c <= '9') {
            float digit = (float)(c - '0');
            if (dot >= 0) {
                usize dot_spot = i - dot;
                float scalar = 1.0f;
                for (usize p = 0; p < dot_spot; p++) {
                    scalar *= 10.0f;
                }
                *value += digit / scalar;
            }
            else if (first == false) {
                *value *= 10.0f;
                *value += digit;
            }
            else {
                *value = digit;
            }
            first = false;
        }
        else if (c == '.') {
            if (dot >= 0) {
                return FAILURE;
            } else {
                dot = i;
            }
        }
        else if (c == '-' && i == 0) {
            neg = true;
        }
        else {
            return FAILURE;
        }
    }
    if (neg) { *value *= -1.0f; }
    return SUCCESS;
}

/* Asset Management **********************************************************/
char * file_name_from_path (char * path, Alloc alloc) {
  usize end = string_length(path);
  while (end --> 0) {
    if (path[end] == '.') {
      break;
    }
  }
  usize start = end;
  while (start --> 0) {
    if (path[start] == PATH_SEPARATOR) {
      start ++;
      break;
    }
  }
  if (start == 0 || end == 0) {
    return NULL;
  }
  usize len = end - start;
  char * name = alloc(len + 1);
  if (name == NULL) {
    return NULL;
  }
  copy_memory(name, &path[start], len);
  name[len] = '\0';

  return name;
}
char * asset_path (const char * target_folder, const char * file, Alloc alloc) {
    #ifdef RELEASE
    TODO apply path from OS appropriate place
    #else
    char * assets_path = "assets/";
    usize assets_len = string_length(assets_path);
    #endif
    usize target_len = string_length(target_folder);
    usize file_len = string_length(file);

    char * result = alloc(sizeof(char) * (assets_len + target_len + file_len + 2));
    if (NULL == result) {
        return NULL;
    }

    copy_memory(result, assets_path, assets_len);
    copy_memory(&result[assets_len], target_folder, target_len);
    result[assets_len + target_len] = PATH_SEPARATOR;
    copy_memory(&result[assets_len + target_len + 1], file, file_len);
    result[assets_len + target_len + file_len + 1] = 0;
    return result;
}
void assets_deinit (Assets * assets) {
    unload_animations(assets);
    for (usize i = 0; i < assets->maps.len; i++) {
        map_deinit(&assets->maps.items[i]);
    }
    listMapDeinit(&assets->maps);
    assets->maps = (ListMap){0};
    for (usize i = 0; i <= FACTION_LAST; i++) {
        UnloadMusicStream(assets->faction_themes[i]);
    }
    UnloadMusicStream(assets->main_theme);

    for (usize i = 0; i < assets->sound_effects.len; i++) {
        UnloadSound(assets->sound_effects.items[i].sound);
    }
    listSFXDeinit(&assets->sound_effects);
    UnloadTexture(assets->empty_building);
    UnloadTexture(assets->ground_texture);
    UnloadTexture(assets->water_texture);
    UnloadTexture(assets->bridge_texture);
    UnloadShader(assets->water_shader);
    UnloadShader(assets->outline_shader);
}

/* Json Handling *************************************************************/
usize skip_tokens(jsmntok_t *tokens, usize from) {
    usize skip = tokens[from].size;
    while (skip --> 0) {
        skip += tokens[++from].size;
    }
    return from + 1;
}

/* Map Loading ***************************************************************/
bool load_paths(
  Map        * map,
  uchar      * data,
  jsmntok_t  * tokens,
  usize        path_pos,
  Vector2      layer_offset
) {
  if (tokens[path_pos].type != JSMN_ARRAY) {
    StringSlice s = make_slice_u(data, tokens[path_pos].start, tokens[path_pos].end);
    log_slice(LOG_ERROR, "Paths objects isn't an array = ", s);
    return 0;
  }
  TraceLog(LOG_INFO, "Loading paths");

  usize cursor = path_pos + 1;
  usize path_count = tokens[path_pos].size;
  map->paths = listPathInit(path_count, perm_allocator());

  while (path_count --> 0) {
    Vector2 offset = layer_offset;
    ListLine lines = listLineInit(5, perm_allocator());
    usize id = 0;

    usize elements = tokens[cursor].size;
    cursor ++;
    while (elements --> 0) {
      if (tokens[cursor].type != JSMN_STRING) {
        TraceLog(LOG_ERROR, "Path element key isn't a string");
        listLineDeinit(&lines);
        goto fail;
      }

      StringSlice key = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
      if (compare_literal(key, "polyline")) {
        cursor ++;
        usize points = tokens[cursor].size;

        bool hasv = false;
        Vector2 a;
        cursor ++;
        while (points --> 0) {
          usize coord_count = tokens[cursor].size;
          cursor ++;

          Vector2 point = {0};
          while ( coord_count --> 0 ) {
            char c = data[tokens[cursor].start];
            cursor ++;
            StringSlice num = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);

            float value = 0;
            if(convert_slice_float(num, &value)) {
              log_slice(LOG_ERROR, "Failed to convert path point", num);
              listLineDeinit(&lines);
              goto fail;
            }

            if (c == 'x') {
              point.x = value;
            } else {
              point.y = value;
            }
            cursor ++;
          }

          if (hasv) {
            Line l = { .a = a, .b = point };
            listLineAppend(&lines, l);
          } else {
            hasv = true;
          }

          a = point;
        }
      }

      else if (compare_literal(key, "id")) {
        cursor++;
        StringSlice val = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
        if (convert_slice_usize(val, &id)) {
          log_slice(LOG_ERROR, "Failed to convert id to a number:", val);
          listLineDeinit(&lines);
          goto fail;
        }
        cursor++;
      }

      else if (compare_literal(key, "x") || compare_literal(key, "y")) {
        cursor ++;
        StringSlice val = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
        float value = 0;
        if(convert_slice_float(val, &value)) {
          log_slice(LOG_ERROR, "Failed to convert path offset", val);
          listLineDeinit(&lines);
          goto fail;
        }

        if (key.start[0] == 'x') {
          offset.x += value;
        } else if (key.start[0] == 'y'){
          offset.y += value;
        }
        cursor ++;
      }

      else {
        cursor = skip_tokens(tokens, cursor);
      }
    }

    for (usize i = 0; i < lines.len; i++) {
      lines.items[i].a.x += offset.x;
      lines.items[i].a.y += offset.y;
      lines.items[i].b.x += offset.x;
      lines.items[i].b.y += offset.y;
    }
    Path p = { .lines = lines, .path_id = id };
    listPathAppend(&map->paths, p);
  }

  return true;

  fail:
  TraceLog(LOG_ERROR, "Failed to load paths");
  listPathDeinit(&map->paths);
  return false;
}

usize load_region_objects(
  Region    * region,
  uchar     * data,
  jsmntok_t * tokens,
  usize       object_pos
) {
  if (tokens[object_pos].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Region object list isn't a list");
    return 0;
  }
  TraceLog(LOG_INFO, "Loading region objects");

  usize cursor = object_pos + 1;
  usize children = tokens[object_pos].size;
  while (children --> 0) {
    if (tokens[cursor].type != JSMN_OBJECT) {
      TraceLog(LOG_ERROR, "Region object isn't an object");
      return 0;
    }

    usize objects = tokens[cursor].size;
    usize type = 0;
    usize polygon = 0;
    Vector2 offset = {0};

    cursor ++;
    while (objects --> 0) {
      if (tokens[cursor].type != JSMN_STRING) {
        TraceLog(LOG_ERROR, "Region object key isn't a string");
        return 0;
      }

      StringSlice key = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
      if (compare_literal(key, "polygon")) {
        polygon = ++cursor;
      }

      else if (compare_literal(key, "type")) {
        type = ++cursor;
      }

      else if (compare_literal(key, "x") || compare_literal(key, "y")) {
        cursor ++;
        StringSlice num = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
        float value;
        if(convert_slice_float(num, &value)) {
          TraceLog(LOG_ERROR, "Couldn't parse region object coordinate");
          return 0;
        }
        if (key.start[0] == 'x') {
          offset.x += value;
        }
        else {
          offset.y += value;
        }
      }
      cursor = skip_tokens(tokens, cursor);
    }

    StringSlice region_object_type = make_slice_u(data, tokens[type].start, tokens[type].end);
    if (compare_literal(region_object_type, "region")) {
      TraceLog(LOG_INFO, "Saving region");
      usize number_of_points = tokens[polygon].size;
      ListLine area = listLineInit(number_of_points, perm_allocator());

      usize point = polygon + 1;
      bool hasv = false;
      Vector2 a = {0};
      while (number_of_points --> 0) {
        Vector2 v = offset;
        usize points = tokens[point].size;
        point ++;

        while (points --> 0) {
          uchar c = data[tokens[point].start];
          point ++;
          StringSlice num = make_slice_u(data, tokens[point].start, tokens[point].end);
          float value;
          if (convert_slice_float(num, &value)) {
            log_slice(LOG_ERROR, "Couldn't convert region polygon x point", num);
            listLineDeinit(&area);
            return 0;
          }

          if (c == 'x') {
            v.x += value;
          }
          else if (c == 'y') {
            v.y += value;
          }
          else {
            TraceLog(LOG_ERROR, "Encountered unexpected coordinate in region polygon");
            listLineDeinit(&area);
            return 0;
          }
          point ++;
        }

        if (hasv) {
          Line l = { .a = a, .b = v };
          listLineAppend(&area, l);
        } else {
          hasv = true;
        }
        a = v;
      }

      Vector2 b = area.items[0].a;
      Line l = { .a = a, .b = b };
      listLineAppend(&area, l);
      region->area.lines = area;
    }

    else if (compare_literal(region_object_type, "guard")) {
      TraceLog(LOG_INFO, "Saving guardian at [%.3f, %.3f]", offset.x, offset.y);
      region->castle.position = offset;
    }

    else if (compare_literal(region_object_type, "node")) {
      TraceLog(LOG_INFO, "Saving building");
      Building b = {0};
      b.position = offset;
      listBuildingAppend(&region->buildings, b);
    }

    else {
      log_slice(LOG_ERROR, "Unrecognized region object", region_object_type);
      listLineDeinit(&region->area.lines);
      return 0;
    }
  }

  return cursor;
}

usize load_region_properties(
  Region    * region,
  uchar     * data,
  jsmntok_t * tokens,
  usize       properties_index
) {
  if (tokens[properties_index].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Tried to read token properties from non-array");
    return 0;
  }
  TraceLog(LOG_INFO, "Loading region properties");

  usize children = tokens[properties_index].size;
  usize cursor = properties_index + 1;
  while (children --> 0) {
    usize value_index = 0;
    usize name = 0;

    usize property_children = tokens[cursor].size;
    cursor ++;
    while (property_children --> 0) {
      StringSlice s = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
      if (compare_literal(s, "name")) {
        name = ++cursor;
      }
      else if (compare_literal(s, "value")) {
        value_index = ++cursor;
      }
      cursor = skip_tokens(tokens, cursor);
    }

    StringSlice s = make_slice_u(data, tokens[name].start, tokens[name].end);
    if (compare_literal(s, "player_id")) {
      StringSlice v = make_slice_u(data, tokens[value_index].start, tokens[value_index].end);
      usize value;
      if (convert_slice_usize(v, &value)) {
        value = 0;
      }
      region->player_id = value;
    }

    else {
      log_slice(LOG_WARNING, "Got unrecognized layer property", s);
    }
  }

  return cursor;
}

usize load_region(
  Map       * map,
  uchar     * data,
  jsmntok_t * tokens,
  usize       region_pos,
  Vector2     offset
) {
  if (tokens[region_pos].type != JSMN_OBJECT) {
    TraceLog(LOG_ERROR, "Expected object to read region from");
    return 0;
  }
  TraceLog(LOG_INFO, "Loading region");

  Region region = {0};
  region.buildings = listBuildingInit(5, perm_allocator());
  region.paths = listPathPInit(5, perm_allocator());

  usize children_count = tokens[region_pos].size;
  region_pos ++;

  while (children_count --> 0) {
    if (tokens[region_pos].type != JSMN_STRING) {
        TraceLog(LOG_ERROR, "Region key wasn't a string");
        goto fail;
    }

    StringSlice key_name = make_slice_u(data, tokens[region_pos].start, tokens[region_pos].end);

    if (compare_literal(key_name, "properties")) {
      region_pos = load_region_properties(&region, data, tokens, region_pos + 1);
      if (region_pos == 0) {
        TraceLog(LOG_ERROR, "Failed to load region properties");
        goto fail;
      }
    }
    else if (compare_literal(key_name, "id")) {
      region_pos ++;
      StringSlice value = make_slice_u(data, tokens[region_pos].start, tokens[region_pos].end);
      usize id = 0;
      if (convert_slice_usize(value, &id)) {
        log_slice(LOG_ERROR, "Expected region id to be a valid number, got:", value);
        goto fail;
      }
      region.region_id = id;
      region_pos++;
    }
    else if (compare_literal(key_name, "x") || compare_literal(key_name, "y")) {
      region_pos ++;
      StringSlice s = make_slice_u(data, tokens[region_pos].start, tokens[region_pos].end);
      float value = 0.0f;
      if (convert_slice_float(s, &value)) {
        TraceLog(LOG_ERROR, "Failed to convert region offset position");
        goto fail;
      }
      if (s.start[0] == 'x') {
        offset.x += value;
      } else {
        offset.y += value;
      }
      region_pos ++;
    }
    else if (compare_literal(key_name, "objects")) {
      region_pos = load_region_objects(&region, data, tokens, region_pos + 1);
      if (region_pos == 0) {
        TraceLog(LOG_ERROR, "Failed to load region objects");
        goto fail;
      }
    }
    else {
      region_pos = skip_tokens(tokens, region_pos);
    }
  }

  for (usize i = 0; i < region.buildings.len; i++) {
    region.buildings.items[i].position.x += offset.x;
    region.buildings.items[i].position.y += offset.y;
  }
  for (usize i = 0; i < region.area.lines.len; i++) {
    region.area.lines.items[i].a.x += offset.x;
    region.area.lines.items[i].a.y += offset.y;
    region.area.lines.items[i].b.x += offset.x;
    region.area.lines.items[i].b.y += offset.y;
  }
  region.castle.position.x += offset.x;
  region.castle.position.y += offset.y;

  listRegionAppend(&map->regions, region);

  return region_pos;

  fail:
  listBuildingDeinit(&region.buildings);
  listPathPDeinit(&region.paths);
  return 0;
}

bool load_regions(
  Map        * map,
  uchar      * data,
  jsmntok_t  * tokens,
  usize        regions_list,
  Vector2      layer_offset
) {
  if (tokens[regions_list].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Non array has been passed to load regions");
    return false;
  }
  TraceLog(LOG_INFO, "Loading regions");

  usize cursor = regions_list + 1;
  usize children = tokens[regions_list].size;
  map->regions = listRegionInit(children, perm_allocator());

  while (children --> 0) {
    cursor = load_region(map, data, tokens, cursor, layer_offset);
    if (cursor == 0) {
      return false;
    }
  }

  return true;
}

usize load_map_layers(
  Map       * map,
  uchar     * data,
  jsmntok_t * tokens,
  usize       current_token
) {
  if (tokens[current_token].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Tried to load map layers from no-array");
    return 0;
  }
  TraceLog(LOG_INFO, "Loading map layers");

  usize cursor = current_token + 1;
  usize layer_count = tokens[current_token].size;
  while (layer_count --> 0) {
    usize collection = 0;
    usize type = 0;
    Vector2 offset = {0};

    usize children_count = tokens[cursor++].size;
    while (children_count --> 0) {

      if (tokens[cursor].type != JSMN_STRING) {
        StringSlice s = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
        log_slice(LOG_ERROR, "Encountered unexpected token:", s);
        return 0;
      }

      StringSlice s = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);

      if (compare_literal(s, "layers") || compare_literal(s, "objects")) {
        log_slice(LOG_INFO, "Found :", s);
        collection = ++cursor;
        cursor = skip_tokens(tokens, cursor);
      }

      else if (compare_literal(s, "name")) {
        TraceLog(LOG_INFO, "Found type");
        type = ++cursor;
        cursor++;
      }

      else if (compare_literal(s, "x")) {
        TraceLog(LOG_INFO, "Found x offset");
        cursor++;
        StringSlice num =
          make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
        float value = 0.0f;
        if (convert_slice_float(num, &value)) {
          log_slice(LOG_ERROR,
                    "Failed to convert float for layer x offset from:", num);
          return 0;
        }

        offset.x = value;
        cursor++;
      }

      else if (compare_literal(s, "y")) {
        TraceLog(LOG_INFO, "Found y offset");
        cursor++;
        StringSlice num =
          make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
        float value = 0.0f;
        if (convert_slice_float(num, &value)) {
          log_slice(LOG_ERROR,
                    "Failed to convert float for layer y offset from:", num);
          return 0;
        }

        offset.y = value;
        cursor++;
      }

      else {
        cursor = skip_tokens(tokens, cursor);
      }

    }

    if (collection == 0 || type == 0) {
      TraceLog(LOG_ERROR, "Failed to read map layer");
      return 0;
    }

    StringSlice layer_type =
      make_slice_u(data, tokens[type].start, tokens[type].end);

    if (compare_literal(layer_type, "Regions")) {
      bool r = load_regions(map, data, tokens, collection, offset);
      if (r == false) {
        TraceLog(LOG_ERROR, "Failed to load regions");
        return 0;
      }
      TraceLog(LOG_INFO, "Saved regions");
    }

    else if(compare_literal(layer_type, "Paths")) {
      bool r = load_paths(map, data, tokens, collection, offset);
      if (r == false) {
        TraceLog(LOG_ERROR, "Failed to load paths");
        return 0;
      }
      TraceLog(LOG_INFO, "Saved paths");
    }
  }
  return cursor;
}

Result load_level(Map * result, char * path) {
  TraceLog(LOG_INFO, "Loading map: %s", path);

  int     len;
  uchar * data;
  data = LoadFileData(path, &len);
  if (len <= 0) {
    TraceLog(LOG_ERROR, "Failed to open map file %s", path);
    return FAILURE;
  }

  jsmn_parser json_parser;
  jsmn_init(&json_parser);

  const usize token_len = jsmn_parse(&json_parser, (char *)(data), len, NULL, 0);
  jsmntok_t * tokens = MemAlloc(sizeof(jsmntok_t) * token_len);
  clear_memory(tokens, sizeof(jsmntok_t) * token_len);
  jsmn_init(&json_parser);

  jsmn_parse(&json_parser, (char *)(data), len, tokens, token_len);

  if (tokens[0].type != JSMN_OBJECT) {
    TraceLog(LOG_ERROR,
             "Map %s has invalid format. Expected root to be an object", path);
    goto fail;
  }

  usize cursor = 1;

  TraceLog(LOG_INFO, "Starting loading");

  while (cursor < token_len) {
    if (tokens[cursor].type != JSMN_STRING) {
      if (tokens[cursor].type == JSMN_UNDEFINED) {
        TraceLog(LOG_INFO, "Finished loading map");
        break;
      }
      StringSlice s = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
      log_slice(LOG_ERROR, "Got unexpected result:", s);
      goto fail;
    }

    StringSlice map_key = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
    if (compare_literal(map_key, "height") || compare_literal(map_key, "tileheight")) {
      cursor ++;
      StringSlice num = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
      usize value;
      if (convert_slice_usize(num, &value)) {
        TraceLog(LOG_ERROR, "Failed to parse map height");
        goto fail;
      }

      if (result->height == 0) {
        result->height = value;
      } else {
        result->height *= value;
      }
      cursor ++;
    }

    else if (compare_literal(map_key, "width") || compare_literal(map_key, "tilewidth")) {
      cursor ++;
      StringSlice num = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
      usize value;
      if (convert_slice_usize(num, &value)) {
        TraceLog(LOG_ERROR, "Failed to parse map width");
        goto fail;
      }

      if (result->width == 0) {
        result->width = value;
      } else {
        result->width *= value;
      }
      cursor ++;
    }

    else if (compare_literal(map_key, "layers")) {
      cursor = load_map_layers(result, data, tokens, cursor + 1);
    }

    else if (compare_literal(map_key, "properties")) {
      cursor ++;
      usize properties_count = tokens[cursor].size;
      cursor ++;
      while (properties_count --> 0) {
        usize name = 0;
        usize value_index = 0;
        usize count = tokens[cursor].size;
        cursor ++;
        while (count --> 0) {
          StringSlice s = make_slice_u(data, tokens[cursor].start, tokens[cursor].end);
          if (compare_literal(s, "name")) {
            name = ++cursor;
          }
          else if (compare_literal(s, "value")) {
            value_index = ++cursor;
          }
          cursor = skip_tokens(tokens, cursor);
        }

        StringSlice property_type = make_slice_u(data, tokens[name].start, tokens[name].end);
        if (compare_literal(property_type, "player_count")) {
          StringSlice num = make_slice_u(data, tokens[value_index].start, tokens[value_index].end);
          usize value;
          if (convert_slice_usize(num, &value)) {
            log_slice(LOG_ERROR, "Couldn't parse player count:", num);
            goto fail;
          }
          // need one space for neutral faction
          if (value >= PLAYERS_MAX) {
            TraceLog(LOG_ERROR, "Map %s has too many players", path);
            goto fail;
          }
          result->player_count = value;
        }
        else if (compare_literal(property_type, "name")) {
          usize name_len = tokens[value_index].end - tokens[value_index].start;
          result->name = MemAlloc(name_len + 1);
          copy_memory(result->name, &data[tokens[value_index].start], name_len);
          result->name[name_len] = '\0';
        }

        else {
          log_slice(LOG_WARNING, "Unrecognized map property:", property_type);
        }
      }
    }

    else {
      cursor = skip_tokens(tokens, cursor);
    }

    if (cursor == 0) {
      goto fail;
    }
  }

  MemFree(tokens);
  UnloadFileData(data);

  return SUCCESS;

fail:
  MemFree(tokens);
  UnloadFileData(data);
  map_deinit(result);
  return FAILURE;
}
Result load_levels (ListMap * maps) {
    FilePathList list;
    list = LoadDirectoryFiles("./assets/maps");

    if (list.count == 0) {
        TraceLog(LOG_FATAL, "No maps present in assets folder");
        goto abort;
    }

    usize i = 0;

    while (i < list.count) {
        // Load the map
        if(listMapAppend(maps, (Map){0})) {
            TraceLog(LOG_ERROR, "Failed to allocate space for map: #%zu: %s", i, list.paths[i]);
            goto abort;
        }
        if (load_level(&maps->items[i], list.paths[i])) {
            TraceLog(LOG_ERROR, "Failed to load map %s", list.paths[i]);
            maps->len -= 1;
            temp_reset();
            i++;
            continue;
        }
        TraceLog(LOG_INFO, "Loaded map file: %s", list.paths[i]);

        temp_reset();
        i ++;
    }

    if (maps->len == 0) {
        TraceLog(LOG_FATAL, "Failed to load any map successfully");
        goto abort;
    }

    UnloadDirectoryFiles(list);
    return SUCCESS;

    abort:
    UnloadDirectoryFiles(list);
    return FAILURE;
}

/* Graphics ******************************************************************/
Result load_particles (Texture2D * array) {
    const char * paths[PARTICLE_LAST + 1] = {
        "arrow.png",
        "fireball.png",
        "fist.png",
        "plus.png",
        "slash-1.png",
        "slash-2.png",
        "symbol.png",
        "thunderbolt.png",
        "tornado.png",
    };

    for (int i = 0; i <= PARTICLE_LAST; i++) {
        char * path = asset_path("particles", paths[i], &temp_alloc);
        if (NULL == path) {
            TraceLog(LOG_ERROR, "Temp allocator ran out of memory for joining paths");
            return FAILURE;
        }
        array[i] = LoadTexture(path);
        if (0 == array[i].format) {
            TraceLog(LOG_ERROR, "Failed to load particle %s", paths[i]);
            return FAILURE;
        }
        temp_free(path);
    }
    return SUCCESS;
}
Result load_buildings (Assets * assets) {
    const char * factions[FACTION_LAST + 1] = {
        "knight",
        "mage"
    };
    const char * types[5] = {
        "melee",
        "range",
        "support",
        "special",
        "money",
    };
    {
        char * path = asset_path("buildings", "neutral-castle.png", &temp_alloc);
        if (NULL == path) {
            TraceLog(LOG_ERROR, "Temp allocator ran out of memory for joining paths");
            return FAILURE;
        }
        assets->neutral_castle = LoadTexture(path);
        if (0 == assets->neutral_castle.format) {
            TraceLog(LOG_ERROR, "Failed to load neutral castle texture");
            return FAILURE;
        }
        temp_free(path);
    }
    usize fac = FACTION_LAST + 1;
    char name[64];
    while (fac --> 0) {
        snprintf(name, 64, "%s-castle.png", factions[fac]);
        char * path = asset_path("buildings", name, &temp_alloc);
        if (NULL == path) {
            TraceLog(LOG_ERROR, "Failed to allocate path for %s", name);
            return FAILURE;
        }
        assets->buildings[fac].castle = LoadTexture(path);
        if (0 == assets->buildings[fac].castle.format) {
            TraceLog(LOG_ERROR, "Failed to load %s", path);
            return FAILURE;
        }
        usize build_count = 5;
        Texture2D * buildable = &assets->buildings[fac].fighter[0];
        while (build_count --> 0) {
            usize level_count = BUILDING_MAX_LEVEL;
            while (level_count --> 0) {
                usize index = build_count * BUILDING_MAX_LEVEL + level_count;
                snprintf(name, 64, "%s-%s-%zu.png", factions[fac], types[build_count], level_count + 1);
                path = asset_path("buildings", name, &temp_alloc);
                if (NULL == path) {
                    TraceLog(LOG_ERROR, "Failed to allocate path for %s", name);
                    return FAILURE;
                }
                buildable[index] = LoadTexture(path);
                if (0 == buildable[index].format) {
                    TraceLog(LOG_ERROR, "Failed to load texture: %s", path);
                    return FAILURE;
                }
                temp_free(path);
            }
        }
    }
    char * path = asset_path("buildings", "flag.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for flag path");
        return FAILURE;
    }
    assets->flag = LoadTexture(path);
    if (0 == assets->flag.format) {
        TraceLog(LOG_ERROR, "Failed to load flag texture");
        return FAILURE;
    }
    return SUCCESS;
}
Result load_backgrounds (Assets * assets) {
    char * path = asset_path("backgrounds", "grass.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for backgrounds");
        return FAILURE;
    }
    assets->ground_texture = LoadTexture(path);
    if (0 == assets->ground_texture.format) {
        TraceLog(LOG_ERROR, "Failed to load background");
        return FAILURE;
    }
    path = asset_path("backgrounds", "bridge.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Failed to allocate path for bridge");
        return FAILURE;
    }
    assets->bridge_texture = LoadTexture(path);
    if (0 == assets->bridge_texture.format) {
        TraceLog(LOG_ERROR, "Failed to load bridge texture");
        return FAILURE;
    }
    path = asset_path("backgrounds", "building-ground.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for building ground");
        return FAILURE;
    }
    assets->empty_building = LoadTexture(path);
    if (0 == assets->empty_building.format) {
        TraceLog(LOG_ERROR, "Failed to load empty building texture");
        return FAILURE;
    }
    path = asset_path("backgrounds", "water.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for water");
        return FAILURE;
    }
    assets->water_texture = LoadTexture(path);
    if (0 == assets->water_texture.format) {
        TraceLog(LOG_ERROR, "Failed to load water");
        return FAILURE;
    }
    path = asset_path("shaders", "water.fs", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for water shader");
        return FAILURE;
    }
    assets->water_shader = LoadShader(0, path);
    if (NULL == assets->water_shader.locs) {
        TraceLog(LOG_ERROR, "Failed to load water shader");
        return FAILURE;
    }
    path = asset_path("shaders", "outline.fs", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for outline shader");
        return FAILURE;
    }
    assets->outline_shader = LoadShader(0, path);
    if (NULL == assets->outline_shader.locs) {
        TraceLog(LOG_ERROR, "Failed to load outline shader");
        return FAILURE;
    }
    return SUCCESS;
}
Result load_graphics (Assets * assets) {
    Result result = load_particles(assets->particles);
    if (result != SUCCESS) return result;
    result = load_buildings(assets);
    if (result != SUCCESS) return result;
    result = load_backgrounds(assets);
    return result;
}

/* Units *********************************************************************/
Result initialize_animation_set (AnimationSet * set) {
    set->attack = listFrameInit(10, perm_allocator());
    set->idle = listFrameInit(10, perm_allocator());
    set->walk = listFrameInit(10, perm_allocator());
    set->cast = listFrameInit(10, perm_allocator());

    if (NULL == set->attack.items) return FAILURE;
    if (NULL == set->idle.items) return FAILURE;
    if (NULL == set->walk.items) return FAILURE;
    if (NULL == set->cast.items) return FAILURE;
    return SUCCESS;
}
Result initialize_animation_data (Assets * assets) {
    for (usize f = 0; f <= FACTION_LAST; f++) {
        for (usize l = 0; l < UNIT_LEVELS; l++) {
            for (usize t = 0; t < UNIT_TYPE_COUNT; t++) {
                initialize_animation_set(&assets->animations.sets[f][t][l]);
            }
        }
    }
    return SUCCESS;
}
Result load_animations (Assets * assets) {
    clear_memory(&assets->animations, sizeof(Animations));
    if (initialize_animation_data(assets)) {
        return FAILURE;
    }
    temp_reset();
    FilePathList data = LoadDirectoryFilesEx("assets" PATH_SEPARATOR_STR "units", ".json", false);
    for (usize i = 0; i < data.count; i++) {
        TraceLog(LOG_INFO, "Loading Animation from %s", data.paths[i]);

        int data_len;
        uchar * text = LoadFileData(data.paths[i], &data_len);

        usize counter = string_length(data.paths[i]);
        StringSlice path = { (uchar*)data.paths[i], counter };
        split_backwards(path, &counter, '.'); // skip the extension

        StringSlice identifier = split_backwards(path, &counter, '-');
        if (NULL == identifier.start) {
            TraceLog(LOG_ERROR, "Couldn't find unit level identifier in %s", data.paths[i]);
            goto next_file;
        }
        usize level;
        if (convert_slice_usize(identifier, &level)) {
            TraceLog(LOG_ERROR, "Expected a number to be last part of unit animation path in %s", data.paths[i]);
            log_slice(LOG_ERROR, "  Instead found", identifier);
            goto next_file;
        }
        if (level == 0 || level > UNIT_LEVELS) {
            TraceLog(LOG_ERROR, "Unit range is incorrect, expected from 1 to %d", UNIT_LEVELS);
            goto next_file;
        }

        identifier = split_backwards(path, &counter, '-');
        if (NULL == identifier.start) {
            TraceLog(LOG_ERROR, "Couldn't find unit type in %s", data.paths[i]);
            goto next_file;
        }
        UnitActiveType unit_type;
        if (compare_literal(identifier, "melee")) {
            unit_type = UNIT_TYPE_FIGHTER;
        }
        else if (compare_literal(identifier, "archer")) {
            unit_type = UNIT_TYPE_ARCHER;
        }
        else if (compare_literal(identifier, "support")) {
            unit_type = UNIT_TYPE_SUPPORT;
        }
        else if (compare_literal(identifier, "special")) {
            unit_type = UNIT_TYPE_SPECIAL;
        }
        else {
            TraceLog(LOG_ERROR, "Failed to get unit identifier in %s", data.paths[i]);
            log_slice(LOG_ERROR, "  Expected unit identifier, got ", identifier);
            goto next_file;
        }

        identifier = split_backwards(path, &counter, '-');
        if (NULL == identifier.start) {
            TraceLog(LOG_ERROR, "Couldn't find faction type in %s", data.paths[i]);
            goto next_file;
        }
        FactionType faction;
        if (compare_literal(identifier, "knight")) {
            faction = FACTION_KNIGHTS;
        }
        else if (compare_literal(identifier, "mage")) {
            faction = FACTION_MAGES;
        }
        else {
            TraceLog(LOG_ERROR, "Failed to get unit faction in %s", data.paths[i]);
            log_slice(LOG_ERROR, "  Expected faction type, got ", identifier);
            goto next_file;
        }

        AnimationSet * animations = &assets->animations.sets[faction][unit_type][level - 1];

        usize path_len = string_length(data.paths[i]);
        char * texture_path = temp_alloc(path_len);
        copy_memory(texture_path, data.paths[i], path_len - 4);
        copy_memory(texture_path + path_len - 4, "png", 3);
        texture_path[path_len - 1] = 0;
        animations->sprite_sheet = LoadTexture(texture_path);
        if (animations->sprite_sheet.format == 0) {
            TraceLog(LOG_ERROR, "Missing sprite sheet at %s", texture_path);
            goto next_file;
        }

        jsmn_parser json;
        jsmn_init(&json);
        usize token_count = jsmn_parse(&json, (char*)text, data_len, NULL, 0);
        jsmntok_t * tokens = temp_alloc(sizeof(jsmntok_t) * token_count);
        if (NULL == tokens) {
            TraceLog(LOG_ERROR, "Failed to allocate memory for reading %s", data.paths[i]);
            goto next_file;
        }

        jsmn_init(&json);
        jsmn_parse(&json, (char*)text, data_len, tokens, token_count);

        usize cursor = 1;

        while (cursor < token_count) {
            if (tokens[cursor].type != JSMN_STRING) {
                if (tokens[cursor].type == JSMN_UNDEFINED) {
                  TraceLog(LOG_INFO, "Finished loading %s", data.paths[i]);
                  break;
                }
                StringSlice s = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                log_slice(LOG_ERROR, "Got unexpected result:", s);
                goto next_file;
            }
            StringSlice json_key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
            if (! compare_literal(json_key, "frames")) {
                cursor = skip_tokens(tokens, cursor);
                continue;
            }
            int end = tokens[cursor].parent;
            cursor++;
            if (tokens[cursor].type != JSMN_ARRAY) {
                TraceLog(LOG_ERROR, "Expected frames to be within an array");
                goto next_file;
            }

            cursor++;
            int frame_object_index = cursor;
            AnimationType type;
            Rectangle rect;
            usize duration;
            cursor++;

            while (cursor < token_count && tokens[cursor].parent != end) {
                if (cursor >= token_count || tokens[cursor].parent != frame_object_index) {
                    // finished loading frame data, save it into the array
                    ListFrame * frames;
                    switch (type) {
                        case ANIMATION_IDLE:
                            frames = &animations->idle; break;
                        case ANIMATION_ATTACK:
                            frames = &animations->attack; break;
                        case ANIMATION_WALK:
                            frames = &animations->walk; break;
                        case ANIMATION_CAST:
                            frames = &animations->cast; break;
                    }

                    bool the_same = false;
                    if (frames->len > 0 && RectangleEquals(rect, frames->items[frames->len - 1].source)) {
                        the_same = true;
                    }

                    if (the_same) {
                        frames->items[frames->len - 1].duration += duration * 0.001f;
                    }
                    else {
                        AnimationFrame new = {
                            .duration = duration * 0.0001f,
                            .source = rect,
                        };
                        listFrameAppend(frames, new);
                    }

                    // continue to next object frame or break if there is no more objects
                    if (tokens[cursor].type == JSMN_OBJECT) {
                        frame_object_index = cursor;
                        cursor++;
                        continue;
                    }
                    else {
                        break;
                    }
                }

                // read animation frame data
                StringSlice key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                if (tokens[cursor].type != JSMN_STRING) {
                    log_slice(LOG_ERROR, "Keyframe data key name expected, got", key);
                    goto next_file;
                }

                if (compare_literal(key, "filename")) {
                    cursor++;
                    StringSlice value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (compare_literal(value, "idle")) {
                        type = ANIMATION_IDLE;
                    }
                    else if (compare_literal(value, "walk")) {
                        type = ANIMATION_WALK;
                    }
                    else if (compare_literal(value, "attack")) {
                        type = ANIMATION_ATTACK;
                    }
                    else if (compare_literal(value, "cast")) {
                        type = ANIMATION_CAST;
                    }
                    else {
                        log_slice(LOG_ERROR, "Invalid animation type:", value);
                        goto next_file;
                    }
                    cursor++;
                }
                else if (compare_literal(key, "frame")) {
                    cursor += 3;
                    StringSlice value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_float(value, &rect.x)) {
                        log_slice(LOG_ERROR, "Failed to get frame x value from", value);
                        goto next_file;
                    }
                    cursor += 2;
                    value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_float(value, &rect.y)) {
                        log_slice(LOG_ERROR, "Failed to get frame y value from", value);
                        goto next_file;
                    }
                    cursor += 2;
                    value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_float(value, &rect.width)) {
                        log_slice(LOG_ERROR, "Failed to get frame width from", value);
                        goto next_file;
                    }
                    cursor += 2;
                    value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_float(value, &rect.height)) {
                        log_slice(LOG_ERROR, "Failed to get frame height from", value);
                        goto next_file;
                    }
                    cursor ++;
                }
                else if (compare_literal(key, "duration")) {
                    cursor ++;
                    StringSlice value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_usize(value, &duration)) {
                        log_slice(LOG_ERROR, "Failed to get frame duration from", value);
                        goto next_file;
                    }
                    cursor ++;
                }
                else {
                    cursor = skip_tokens(tokens, cursor);
                }
            }
        }

        next_file:
        UnloadFileData((unsigned char*)text);
        temp_reset();
    }
    UnloadDirectoryFiles(data);
    return SUCCESS;
}
void unload_animation_set (AnimationSet * set) {
    listFrameDeinit(&set->idle);
    listFrameDeinit(&set->walk);
    listFrameDeinit(&set->attack);
    listFrameDeinit(&set->cast);
    UnloadTexture(set->sprite_sheet);
}
void unload_animations (Assets * assets) {
    for (usize f = 0; f <= FACTION_LAST; f++) {
        for (usize l = 0; l < UNIT_LEVELS; l++) {
            for (usize t = 0; t < UNIT_TYPE_COUNT; t++) {
                unload_animation_set(&assets->animations.sets[f][t][l]);
            }
        }
    }
}

/* Sounds ********************************************************************/
Result load_music (Assets * assets) {
    TraceLog(LOG_INFO, "Loading Music");
    const char * paths[FACTION_LAST + 1] = {
        "knights.xm",
        "mages.xm",
    };

    for (int i = 0; i <= FACTION_LAST; i++) {
        char * path = asset_path("music", paths[i], &temp_alloc);
        TraceLog(LOG_INFO, "  Loading %s", path);
        if (NULL == path) {
            TraceLog(LOG_ERROR, "Temp allocator ran out of memory for joining music paths");
            return FAILURE;
        }
        assets->faction_themes[i] = LoadMusicStream(path);
        if (assets->faction_themes[i].frameCount == 0) {
            TraceLog(LOG_ERROR, "Failed to load music from %s", path);
            return FAILURE;
        }
        temp_free(path);
    }

    char * path = asset_path("music", "title.xm", &temp_alloc);
    TraceLog(LOG_INFO, "  Loading %s", path);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator ran out of memory for joining title music path");
        return FAILURE;
    }
    assets->main_theme = LoadMusicStream(path);
    if (assets->main_theme.frameCount == 0) {
        TraceLog(LOG_ERROR, "Failed to load main theme");
        return FAILURE;
    }
    temp_free(path);

    TraceLog(LOG_INFO, "Music loaded");
    return SUCCESS;
}
typedef struct {
    char * name;
    SoundEffectType kind;
} SoundLoader;
Result load_sound_effects (Assets * assets) {
    SoundLoader loader[] = {
        { "hurt-man.wav", SOUND_HURT_HUMAN },
        { "hurt-old.wav", SOUND_HURT_HUMAN_OLD },
        { "hurt-knight.wav", SOUND_HURT_KNIGHT },
        { "hurt-golem.wav", SOUND_HURT_GOLEM },
        { "hurt-gremlin.wav", SOUND_HURT_GREMLIN },
        { "hurt-genie.wav", SOUND_HURT_GENIE },
        { "hurt-castle.wav", SOUND_HURT_CASTLE },

        { "attack-sword.wav", SOUND_ATTACK_SWORD },
        { "attack-bow.wav", SOUND_ATTACK_BOW },
        { "attack-holy.wav", SOUND_ATTACK_HOLY },
        { "attack-knight.wav", SOUND_ATTACK_KNIGHT },
        { "attack-golem.wav", SOUND_ATTACK_GOLEM },
        { "attack-fireball.wav", SOUND_ATTACK_FIREBALL },
        { "attack-tornado.wav", SOUND_ATTACK_TORNADO },
        { "attack-thunder.wav", SOUND_ATTACK_THUNDER },

        { "magic-healing.wav", SOUND_MAGIC_HEALING },
        { "magic-weakness.wav", SOUND_MAGIC_WEAKNESS },

        { "ui-build.wav", SOUND_BUILDING_BUILD },
        { "ui-demolish.wav", SOUND_BUILDING_DEMOLISH },
        { "ui-upgrade.wav", SOUND_BUILDING_UPGRADE },

        { "ui-flag-up.wav", SOUND_FLAG_UP },
        { "ui-flag-down.wav", SOUND_FLAG_DOWN },

        { "ui-region-gain.wav", SOUND_REGION_CONQUERED },
        { "ui-region-loss.wav", SOUND_REGION_LOST },

        { "ui-click.wav", SOUND_UI_CLICK },
        {0},
    };
    assets->sound_effects = listSFXInit(26, perm_allocator());
    if (NULL == assets->sound_effects.items) {
        TraceLog(LOG_FATAL, "Failed to allocate memory for sound effects");
        return FATAL;
    }
    usize i = 0;
    while (loader[i].name != NULL) {
        char * path = asset_path("sfx", loader[i].name, &temp_alloc);
        Sound s = LoadSound(path);
        if (s.frameCount == 0) {
            TraceLog(LOG_ERROR, "Failed to load sound");
            i ++;
            continue;
        }
        SoundEffect se = { loader[i].kind, s };
        if (listSFXAppend(&assets->sound_effects, se)) {
            TraceLog(LOG_ERROR, "Failed to add sound effect %s", loader[i].name);
            UnloadSound(s);
        }
        i++;
    }
    return SUCCESS;
}

float GetMasterVolume(void);

/* Settings ******************************************************************/
Result load_settings (Settings * settings) {
    // TODO load the settings from file
    #ifdef DEBUG
    settings->volume_master = 0.1;
    settings->volume_music = 0.0;
    settings->volume_sfx = 0.5;
    settings->volume_ui = 0.5;
    #else
    settings->volume_master = 1.0;
    settings->volume_music = 0.5;
    settings->volume_sfx = 0.5;
    settings->volume_ui = 0.5;
    #endif
    settings->theme = theme_setup();
    return SUCCESS;
}
Result save_settings (const Settings * settings) {
    (void)settings;
    // TODO implement settings saving to file
    return SUCCESS;
}
