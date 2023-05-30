#include <raylib.h>

#include "assets.h"

#define JSMN_PARENT_LINKS
#include "../vendor/jsmn.h"

usize skip_tokens(jsmntok_t *tokens, usize from) {
  usize skip = tokens[from].size;
  while (skip --> 0) {
    skip += tokens[++from].size;
  }
  return from + 1;
}

bool load_paths(
  Map        * map,
  uchar      * data,
  jsmntok_t  * tokens,
  usize        path_pos,
  Vector2      layer_offset
) {
  if (tokens[path_pos].type != JSMN_ARRAY) {
    StringSlice s = make_slice(data, tokens[path_pos].start, tokens[path_pos].end);
    log_slice(LOG_ERROR, "Paths objects isn't an array = ", s);
    return 0;
  }
  TraceLog(LOG_INFO, "Loading paths");

  usize cursor = path_pos + 1;
  usize path_count = tokens[path_pos].size;
  map->paths = listPathInit(path_count, &MemAlloc, &MemFree);

  while (path_count --> 0) {
    Vector2 offset = layer_offset;
    ListLine lines = listLineInit(5, &MemAlloc, &MemFree);

    usize elements = tokens[cursor].size;
    cursor ++;
    while (elements --> 0) {
      if (tokens[cursor].type != JSMN_STRING) {
        TraceLog(LOG_ERROR, "Path element key isn't a string");
        listLineDeinit(&lines);
        goto fail;
      }

      StringSlice key = make_slice(data, tokens[cursor].start, tokens[cursor].end);
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
            StringSlice num = make_slice(data, tokens[cursor].start, tokens[cursor].end);
            OptionalFloat f = convert_slice_float(num);
            if (f.has_value == false) {
              log_slice(LOG_ERROR, "Failed to convert path point", num);
              listLineDeinit(&lines);
              return false;
            }

            if (c == 'x') {
              point.x = f.value;
            } else {
              point.y = f.value;
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

      else if (compare_literal(key, "x") || compare_literal(key, "y")) {
        cursor ++;
        StringSlice val = make_slice(data, tokens[cursor].start, tokens[cursor].end);
        OptionalFloat f = convert_slice_float(val);
        if (f.has_value == false) {
          log_slice(LOG_ERROR, "Failed to convert path offset", val);
          listLineDeinit(&lines);
          return false;
        }

        if (key.start[0] == 'x') {
          offset.x += f.value;
        } else if (key.start[0] == 'y'){
          offset.y += f.value;
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
    Path p = { .lines = lines };
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

      StringSlice key = make_slice(data, tokens[cursor].start, tokens[cursor].end);
      if (compare_literal(key, "polygon")) {
        polygon = ++cursor;
      }

      else if (compare_literal(key, "type")) {
        type = ++cursor;
      }

      else if (compare_literal(key, "x") || compare_literal(key, "y")) {
        cursor ++;
        StringSlice num = make_slice(data, tokens[cursor].start, tokens[cursor].end);
        OptionalFloat f = convert_slice_float(num);
        if (f.has_value == false) {
          TraceLog(LOG_ERROR, "Couldn't parse region object coordinate");
          return 0;
        }
        if (key.start[0] == 'x') {
          offset.x += f.value;
        }
        else {
          offset.y += f.value;
        }
      }
      cursor = skip_tokens(tokens, cursor);
    }

    StringSlice region_object_type = make_slice(data, tokens[type].start, tokens[type].end);
    if (compare_literal(region_object_type, "region")) {
      TraceLog(LOG_INFO, "Saving region");
      usize number_of_points = tokens[polygon].size;
      ListLine area = listLineInit(number_of_points, &MemAlloc, &MemFree);

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
          StringSlice num = make_slice(data, tokens[point].start, tokens[point].end);
          OptionalFloat f = convert_slice_float(num);
          if (f.has_value == false) {
            log_slice(LOG_ERROR, "Couldn't convert region polygon x point", num);
            listLineDeinit(&area);
            return 0;
          }
          if (c == 'x') {
            v.x += f.value;
          }
          else if (c == 'y') {
            v.y += f.value;
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

    else if (compare_literal(region_object_type, "castle")) {
      TraceLog(LOG_INFO, "Saving castle at [%.3f, %.3f]", offset.x, offset.y);
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
    usize value = 0;
    usize name = 0;

    usize property_children = tokens[cursor].size;
    cursor ++;
    while (property_children --> 0) {
      StringSlice s = make_slice(data, tokens[cursor].start, tokens[cursor].end);
      if (compare_literal(s, "name")) {
        name = ++cursor;
      }
      else if (compare_literal(s, "value")) {
        value = ++cursor;
      }
      cursor = skip_tokens(tokens, cursor);
    }

    StringSlice s = make_slice(data, tokens[name].start, tokens[name].end);
    if (compare_literal(s, "player_id")) {
      StringSlice v = make_slice(data, tokens[value].start, tokens[value].end);
      region->player_owned = convert_slice_usize(v);
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
  region.buildings = listBuildingInit(5, &MemAlloc, &MemFree);

  usize children_count = tokens[region_pos].size;
  region_pos ++;

  while (children_count --> 0) {
    if (tokens[region_pos].type != JSMN_STRING) {
        TraceLog(LOG_ERROR, "Region key wasn't a string");
        goto fail;
    }

    StringSlice s = make_slice(data, tokens[region_pos].start, tokens[region_pos].end);

    if (compare_literal(s, "properties")) {
      region_pos = load_region_properties(&region, data, tokens, region_pos + 1);
      if (region_pos == 0) {
        TraceLog(LOG_ERROR, "Failed to load region properties");
        goto fail;
      }
    }
    else if (compare_literal(s, "x") || compare_literal(s, "y")) {
      region_pos ++;
      StringSlice s = make_slice(data, tokens[region_pos].start, tokens[region_pos].end);
      OptionalFloat f = convert_slice_float(s);
      if (f.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to convert region offset position");
        goto fail;
      }
      if (s.start[0] == 'x') {
        offset.x += f.value;
      } else {
        offset.y += f.value;
      }
      region_pos ++;
    }
    else if (compare_literal(s, "objects")) {
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
        StringSlice s = make_slice(data, tokens[cursor].start, tokens[cursor].end);
        log_slice(LOG_ERROR, "Encountered unexpected token:", s);
        return 0;
      }

      StringSlice s = make_slice(data, tokens[cursor].start, tokens[cursor].end);

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
          make_slice(data, tokens[cursor].start, tokens[cursor].end);
        OptionalFloat x = convert_slice_float(num);

        if (x.has_value) {
          offset.x = x.value;
        } else {
          log_slice(LOG_ERROR,
                    "Failed to convert float for layer x offset from:", num);
          return 0;
        }
        cursor++;
      }

      else if (compare_literal(s, "y")) {
        TraceLog(LOG_INFO, "Found y offset");
        cursor++;
        StringSlice num =
          make_slice(data, tokens[cursor].start, tokens[cursor].end);
        OptionalFloat y = convert_slice_float(num);

        if (y.has_value) {
          offset.y = y.value;
        } else {
          log_slice(LOG_ERROR,
                    "Failed to convert float for layer y offset from:", num);
          return 0;
        }
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
      make_slice(data, tokens[type].start, tokens[type].end);

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

OptionalMap load_level(char *path) {
  TraceLog(LOG_INFO, "Loading map: %s", path);

  OptionalMap map = {0};
  uint    len;
  uchar * data;
  data = LoadFileData(path, &len);
  if (len == 0) {
    TraceLog(LOG_ERROR, "Failed to open map file %s", path);
    return map;
  }

  jsmn_parser json_parser;
  jsmn_init(&json_parser);

  // TODO make this dynamic to support large map sizes and complexity
  const usize token_len = 1024;
  jsmntok_t tokens[token_len];
  clear_memory(tokens, sizeof(jsmntok_t) * token_len);

  size_t tokenlen =
      jsmn_parse(&json_parser, (char *)(data), len, tokens, token_len);

  if (tokens[0].type != JSMN_OBJECT) {
    TraceLog(LOG_ERROR,
             "Map %s has invalid format. Expected root to be an object", path);
    goto fail;
  }

  map.value.regions = listRegionInit (10, &MemAlloc, &MemFree);
  map.value.paths   = listPathInit   (10, &MemAlloc, &MemFree);

  usize fields = tokens[0].size;
  usize cursor = 2;

  TraceLog(LOG_INFO, "Starting loading");

  while (cursor < token_len) {
    if (tokens[cursor].type != JSMN_STRING) {
      if (tokens[cursor].type == JSMN_UNDEFINED) {
        TraceLog(LOG_INFO, "Finished loading map");
        break;
      }
      StringSlice s = make_slice(data, tokens[cursor].start, tokens[cursor].end);
      log_slice(LOG_ERROR, "Got unexpected result:", s);
      goto fail;
    }

    StringSlice map_key = make_slice(data, tokens[cursor].start, tokens[cursor].end);
    if (compare_literal(map_key, "height") || compare_literal(map_key, "tileheight")) {
      cursor ++;
      StringSlice num = make_slice(data, tokens[cursor].start, tokens[cursor].end);
      OptionalUsize v = convert_slice_usize(num);
      if (v.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to parse map height");
        goto fail;
      }
      if (map.value.height == 0) {
        map.value.height = v.value;
      } else {
        map.value.height *= v.value;
      }
      cursor ++;
    }

    else if (compare_literal(map_key, "width") || compare_literal(map_key, "tilewidth")) {
      cursor ++;
      StringSlice num = make_slice(data, tokens[cursor].start, tokens[cursor].end);
      OptionalUsize v = convert_slice_usize(num);
      if (v.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to parse map width");
        goto fail;
      }
      if (map.value.width == 0) {
        map.value.width = v.value;
      } else {
        map.value.width *= v.value;
      }
      cursor ++;
    }

    else if (compare_literal(map_key, "layers")) {
      cursor = load_map_layers(&map.value, data, tokens, cursor + 1);
    }

    else if (compare_literal(map_key, "properties")) {
      cursor ++;
      usize properties_count = tokens[cursor].size;
      cursor ++;
      while (properties_count --> 0) {
        usize name = 0;
        usize value = 0;
        usize count = tokens[cursor].size;
        cursor ++;
        while (count --> 0) {
          StringSlice s = make_slice(data, tokens[cursor].start, tokens[cursor].end);
          if (compare_literal(s, "name")) {
            name = ++cursor;
          }
          else if (compare_literal(s, "value")) {
            value = ++cursor;
          }
          cursor = skip_tokens(tokens, cursor);
        }

        StringSlice property_type = make_slice(data, tokens[name].start, tokens[name].end);
        if (compare_literal(property_type, "player_count")) {
          StringSlice num = make_slice(data, tokens[value].start, tokens[value].end);
          OptionalUsize n = convert_slice_usize(num);
          if (n.has_value == false) {
            log_slice(LOG_ERROR, "Couldn't parse player count:", num);
            goto fail;
          }
          map.value.player_count = n.value;
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

  UnloadFileData(data);
  TraceLog(LOG_INFO, "Loading map %s succeeded", path);
  map.has_value = true;
  return map;

fail:
  TraceLog(LOG_ERROR, "Loading map %s failed", path);
  UnloadFileData(data);
  map_deinit(&map.value);
  return map;
}
