#include "constants.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if !defined (_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if defined (EMBEDED_ASSETS)
#include "packed_assets.h"
#endif

#include "mesh.h"
#include "math.h"
#include "assets.h"
#include "std.h"
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
void unload_ui (UiAssets * assets);

/* String Handling ***********************************************************/
void log_slice(TraceLogLevel log_level, char * text, StringSlice slice) {
    char s[slice.len + 1];
    copy_memory(s, slice.start, sizeof(char) * slice.len);
    s[slice.len] = '\0';
    TraceLog(log_level, "%s %s", text, s);
}
StringSlice make_slice_u(const uchar * from, usize start, usize end) {
    StringSlice s;
    s.start = (uchar*) from + start;
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
Result tokenize (char * string, char * match, usize * in_out_cursor, StringSlice * result) {
    usize start = *in_out_cursor;

    usize i = start;
    if (string[i] == 0) return FAILURE;
    {
        usize m = 0;
        while (match[m] && string[i]) {
            if (match[m] == string[i]) {
                start++;
                i++;
                m = 0;
            }
            else {
               m++;
            }
        }
    }
    while (string[i]) {
        usize m = 0;
        while (match[m]) {
            if (match[m] == string[i]) {
                goto found;
            }
            m++;
        }
        i++;
    }
    found:
    if (start == 0 && string[i] == 0) return FAILURE;
    result->start = (uchar*)&string[start];
    result->len = i - start;
    *in_out_cursor = i;
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
    #if defined(_WIN32) || defined(DEBUG) || defined(ANDROID)
    char * assets_path = "assets" PATH_SEPARATOR_STR;
    #else
    char * assets_path;
    char * data = getenv("XDG_DATA_HOME");
    if (NULL == data) {
        TraceLog(LOG_ERROR, "Failed to find XDG data path");
        char * home = getenv("HOME");
        if (NULL == home) TraceLog(LOG_FATAL, "Failed to find HOME path");

        usize len = string_length(home);
        char * rest = "/.local/share/line-lancer/";
        usize rest_len = string_length(rest);

        assets_path = temp_alloc(len + rest_len + 1);
        copy_memory(assets_path, home, len);
        copy_memory(assets_path + len, rest, rest_len);
        assets_path[len + rest_len] = 0;
    }
    else {
        usize len = string_length(data);
        char * rest = "/line-lancer/";
        usize rest_len = string_length(rest);
        assets_path = temp_alloc(len + rest_len + 1);
        copy_memory(assets_path, data, len);
        copy_memory(assets_path + len, rest, rest_len);
        assets_path[len + rest_len] = 0;
    }
    #endif
    usize assets_len = string_length(assets_path);

    usize target_len = string_length(target_folder);
    usize file_len = string_length(file);
    usize extra = 2;

    char * result = alloc(sizeof(char) * (assets_len + target_len + file_len + extra));
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
char * config_path (const char * file, Alloc alloc) {
    char * prefix;
    usize prefix_len;
    #ifdef _WIN32
    prefix = "./";
    prefix_len = string_length(prefix);
    #else
    char * conf = getenv("XDG_CONFIG_HOME");
    if (NULL == conf) {
        TraceLog(LOG_WARNING, "User does not have XDG_CONFIG_HOME set, falling back to default config path");
        char * home = getenv("HOME");
        if (NULL == home) TraceLog(LOG_FATAL, "Failed  to find home directory");
        usize home_len = string_length(home);
        char * rest = "/.config/line-lancer/";
        usize rest_len = string_length(rest);
        prefix = temp_alloc(home_len + rest_len + 1);
        copy_memory(prefix, home, home_len);
        copy_memory(prefix + home_len, rest, rest_len);
        prefix[home_len + rest_len] = 0;
    }
    else {
        usize conf_len = string_length(conf);
        char * rest = "/line-lancer/";
        usize rest_len = string_length(rest);
        prefix = temp_alloc(conf_len + rest_len + 1);
        copy_memory(prefix, conf, conf_len);
        copy_memory(prefix + conf_len, rest, rest_len);
        prefix[conf_len + rest_len] = 0;
    }
    prefix_len = string_length(prefix);
    if (! DirectoryExists(prefix)) {
        int status = mkdir(prefix, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status != 0) {
            TraceLog(LOG_ERROR, "Failed to create config directory");
        }
    }
    #endif

    usize file_len = string_length(file);
    char * result = alloc(file_len + prefix_len + 1);
    copy_memory(result, prefix, prefix_len);
    copy_memory(result + prefix_len, file, file_len);
    result[prefix_len + file_len] = 0;
    return result;
}
void assets_deinit (Assets * assets) {
    unload_animations(assets);
    for (usize i = 0; i < assets->maps.len; i++) {
        MemFree(assets->maps.items[i].name);
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
    unload_ui(&assets->ui);
    UnloadShader(assets->water_shader);
    UnloadShader(assets->outline_shader);
}
const unsigned char * load_asset (const char * path, int * len) {
  #if defined(EMBEDED_ASSETS)
  return load_data_from_path(path, len);
  #else
  return LoadFileData(path, len);
  #endif
}
void unload_asset (const unsigned char * data) {
  #if defined(EMBEDED_ASSETS)
  (void)data;
  #else
  UnloadFileData((unsigned char *)data);
  #endif
}
FilePathList load_map_paths () {
    #if defined(EMBEDED_ASSETS)
    return load_map_path_list();
    #else
    const char * path = asset_path("maps", "", &temp_alloc);
    return LoadDirectoryFiles(path);
    #endif
}
FilePathList load_unit_meta_paths () {
    #if defined(EMBEDED_ASSETS)
    return load_unit_meta_file_paths();
    #else
    const char * path = asset_path("units", "", &temp_alloc);
    return LoadDirectoryFilesEx(path, ".json", false);
    #endif
}
Texture load_texture (const char * path) {
  #if defined(EMBEDED_ASSETS)
  int len = 0;
  const unsigned char * data = load_asset(path, &len);
  if (NULL == data) return (Texture) {0};
  Image i = LoadImageFromMemory(GetFileExtension(path), data, len);
  Texture t = LoadTextureFromImage(i);
  UnloadImage(i);
  return t;
  #else
  return LoadTexture(path);
  #endif
}
Shader load_shader (const char * fs_path) {
  #if defined(EMBEDED_ASSETS)
  int len = 0;
  const char * data = (const char *) load_asset(fs_path, &len);
  return LoadShaderFromMemory(0, data);
  #else
  return LoadShader(0, fs_path);
  #endif
}
Sound load_sound (const char * path) {
    #if defined(EMBEDED_ASSETS)
    int len = 0;
    const unsigned char * data = load_asset(path, &len);
    if (NULL == data) return (Sound) {0};
    Wave w = LoadWaveFromMemory(GetFileExtension(path), data, len);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
    #else
    return LoadSound(path);
    #endif
}
void unload_file_paths (FilePathList list) {
    #if defined(EMBEDED_ASSETS)
    (void)list;
    #else
    UnloadDirectoryFiles(list);
    #endif
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
  Map         * map,
  const uchar * data,
  jsmntok_t   * tokens,
  usize         path_pos,
  Vector2       layer_offset
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
  Region      * region,
  const uchar * data,
  jsmntok_t   * tokens,
  usize         object_pos
) {
  if (tokens[object_pos].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Region object list isn't a list");
    return 0;
  }
  TraceLog(LOG_DEBUG, "Loading region objects");

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
      TraceLog(LOG_DEBUG, "Saving region");
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
      TraceLog(LOG_DEBUG, "Saving guardian at [%.3f, %.3f]", offset.x, offset.y);
      region->castle.position = offset;
    }

    else if (compare_literal(region_object_type, "node")) {
      TraceLog(LOG_DEBUG, "Saving building");
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
  Region      * region,
  const uchar * data,
  jsmntok_t   * tokens,
  usize        properties_index
) {
  if (tokens[properties_index].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Tried to read token properties from non-array");
    return 0;
  }
  TraceLog(LOG_DEBUG, "Loading region properties");

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
      usize value = 0;
      if (convert_slice_usize(v, &value)) {
        log_slice(LOG_WARNING, "We found player_id in a region but it contains invalid value:", v);
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
  Map         * map,
  const uchar * data,
  jsmntok_t   * tokens,
  usize         region_pos,
  Vector2       offset
) {
  if (tokens[region_pos].type != JSMN_OBJECT) {
    TraceLog(LOG_ERROR, "Expected object to read region from");
    return 0;
  }
  TraceLog(LOG_DEBUG, "Loading region");

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
  Map         * map,
  const uchar * data,
  jsmntok_t   * tokens,
  usize         regions_list,
  Vector2       layer_offset
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
  Map         * map,
  const uchar * data,
  jsmntok_t   * tokens,
  usize         current_token
) {
  if (tokens[current_token].type != JSMN_ARRAY) {
    TraceLog(LOG_ERROR, "Tried to load map layers from no-array");
    return 0;
  }
  TraceLog(LOG_DEBUG, "Loading map layers");

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
        log_slice(LOG_DEBUG, "Found :", s);
        collection = ++cursor;
        cursor = skip_tokens(tokens, cursor);
      }

      else if (compare_literal(s, "name")) {
        TraceLog(LOG_DEBUG, "Found type");
        type = ++cursor;
        cursor++;
      }

      else if (compare_literal(s, "x")) {
        TraceLog(LOG_DEBUG, "Found x offset");
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
        TraceLog(LOG_DEBUG, "Found y offset");
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
  const uchar * data;
  data = load_asset(path, &len);
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

  TraceLog(LOG_DEBUG, "Starting loading");

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
      usize value = 0;
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
      usize value = 0;
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
          usize value = 0;
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
  unload_asset(data);

  return SUCCESS;

fail:
  MemFree(tokens);
  unload_asset(data);
  map_deinit(result);
  return FAILURE;
}
Result load_levels (ListMap * maps) {
    FilePathList list = load_map_paths();

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

    unload_file_paths(list);
    return SUCCESS;

    abort:
    unload_file_paths(list);
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
        array[i] = load_texture(path);
        if (0 == array[i].format) {
            TraceLog(LOG_ERROR, "Failed to load particle %s", path);
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
        assets->neutral_castle = load_texture(path);
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
        assets->buildings[fac].castle = load_texture(path);
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
                buildable[index] = load_texture(path);
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
    assets->flag = load_texture(path);
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
    assets->ground_texture = load_texture(path);
    if (0 == assets->ground_texture.format) {
        TraceLog(LOG_ERROR, "Failed to load background");
        return FAILURE;
    }
    path = asset_path("backgrounds", "bridge.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Failed to allocate path for bridge");
        return FAILURE;
    }
    assets->bridge_texture = load_texture(path);
    if (0 == assets->bridge_texture.format) {
        TraceLog(LOG_ERROR, "Failed to load bridge texture");
        return FAILURE;
    }
    path = asset_path("backgrounds", "building-ground.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for building ground");
        return FAILURE;
    }
    assets->empty_building = load_texture(path);
    if (0 == assets->empty_building.format) {
        TraceLog(LOG_ERROR, "Failed to load empty building texture");
        return FAILURE;
    }
    path = asset_path("backgrounds", "water.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for water");
        return FAILURE;
    }
    assets->water_texture = load_texture(path);
    if (0 == assets->water_texture.format) {
        TraceLog(LOG_ERROR, "Failed to load water");
        return FAILURE;
    }
    path = asset_path("shaders", "water.fs", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for water shader");
        return FAILURE;
    }
    assets->water_shader = load_shader(path);
    if (NULL == assets->water_shader.locs) {
        TraceLog(LOG_ERROR, "Failed to load water shader");
        return FAILURE;
    }
    path = asset_path("shaders", "outline.fs", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for outline shader");
        return FAILURE;
    }
    assets->outline_shader = load_shader(path);
    if (NULL == assets->outline_shader.locs) {
        TraceLog(LOG_ERROR, "Failed to load outline shader");
        return FAILURE;
    }
    return SUCCESS;
}
Result load_ui (UiAssets * assets) {
    char * path = asset_path("ui", "background.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for ui");
        return FAILURE;
    }
    assets->background_box = load_texture(path);
    if (0 == assets->background_box.format) {
        TraceLog(LOG_ERROR, "Failed to load ui background box");
        return FAILURE;
    }
    assets->background_box_info = (NPatchInfo) {
        .source = { 0, 0, assets->background_box.width, assets->background_box.height },
        .layout = NPATCH_NINE_PATCH,
        .top    = 12,
        .bottom = 12,
        .left   = 12,
        .right  = 12,
    };

    path = asset_path("ui", "button.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for button");
        return FAILURE;
    }
    assets->button = load_texture(path);
    if (0 == assets->button.format) {
        TraceLog(LOG_ERROR, "Failed to load ui button texture");
        return FAILURE;
    }
    path = asset_path("ui", "button-click.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Failed to allocate path for button close");
        return FAILURE;
    }
    assets->button_press = load_texture(path);
    if (0 == assets->button_press.format) {
        TraceLog(LOG_ERROR, "Failed to load button press texture");
        return FAILURE;
    }
    assets->button_info = (NPatchInfo) {
        .source = { 0, 0, assets->button.width, assets->button.height },
        .layout = NPATCH_NINE_PATCH,
        .left   = 12,
        .right  = 12,
        .top    = 6,
        .bottom = 8,
    };
    path = asset_path("ui", "button-close.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for close button");
        return FAILURE;
    }
    assets->close = load_texture(path);
    if (0 == assets->close.format) {
        TraceLog(LOG_ERROR, "Failed to load ui close button texture");
        return FAILURE;
    }
    path = asset_path("ui", "button-close-click.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for close button click");
        return FAILURE;
    }
    assets->close_press = load_texture(path);
    if (0 == assets->close_press.format) {
        TraceLog(LOG_ERROR, "Failed to load ui close button click texture");
        return FAILURE;
    }
    path = asset_path("ui", "slider.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for slider");
        return FAILURE;
    }
    assets->slider = load_texture(path);
    if (0 == assets->slider.format) {
        TraceLog(LOG_ERROR, "Failed to load ui slider texture");
        return FAILURE;
    }
    path = asset_path("ui", "pointer.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for slider thumb");
        return FAILURE;
    }
    assets->slider_thumb = load_texture(path);
    if (0 == assets->slider_thumb.format) {
        TraceLog(LOG_ERROR, "Failed to load ui slider thumb texture");
        return FAILURE;
    }
    assets->slider_info = (NPatchInfo) {
        .source = { 0, 0, assets->slider.width, assets->slider.height },
        .layout = NPATCH_THREE_PATCH_HORIZONTAL,
        .left = 21,
        .right = 21,
    };
    path = asset_path("ui", "drop.png", &temp_alloc);
    if (NULL == path) {
        TraceLog(LOG_ERROR, "Temp allocator failed to allocate path for drop down");
        return FAILURE;
    }
    assets->drop = load_texture(path);
    if (0 == assets->button.format) {
        TraceLog(LOG_ERROR, "Failed to load ui dropdown texture");
        return FAILURE;
    }
    return SUCCESS;
}
void unload_ui (UiAssets * assets) {
    UnloadTexture(assets->background_box);
    UnloadTexture(assets->button);
    UnloadTexture(assets->button_press);
    UnloadTexture(assets->close);
    UnloadTexture(assets->close_press);
    UnloadTexture(assets->slider);
    UnloadTexture(assets->slider_thumb);
    UnloadTexture(assets->drop);
}
Result load_graphics (Assets * assets) {
    Result result = load_particles(assets->particles);
    if (result != SUCCESS) return result;
    result = load_buildings(assets);
    if (result != SUCCESS) return result;
    result = load_backgrounds(assets);
    if (result != SUCCESS) return result;
    result = load_ui(&assets->ui);
    return result;
}

/* Units *********************************************************************/
Result initialize_animation_data (Assets * assets) {
    for (usize f = 0; f <= FACTION_LAST; f++) {
        for (usize l = 0; l < UNIT_LEVELS; l++) {
            for (usize t = 0; t < UNIT_TYPE_COUNT; t++) {
                ListFrame frame = listFrameInit(15, perm_allocator());
                if (NULL == frame.items) return FAILURE;
                assets->animations.sets[f][t][l].frames = frame;
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
    FilePathList data = load_unit_meta_paths();
    for (usize i = 0; i < data.count; i++) {
        TraceLog(LOG_INFO, "Loading Animation from %s", data.paths[i]);

        int data_len;
        const uchar * text = load_asset(data.paths[i], &data_len);

        usize counter = string_length(data.paths[i]);
        StringSlice path = { (uchar*)data.paths[i], counter };
        split_backwards(path, &counter, '.'); // skip the extension

        StringSlice identifier = split_backwards(path, &counter, '-');
        if (NULL == identifier.start) {
            TraceLog(LOG_ERROR, "Couldn't find unit level identifier in %s", data.paths[i]);
            goto next_file;
        }
        usize level = 0;
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
        else if (compare_literal(identifier, "range")) {
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
        ListFrame * frames = &animations->frames;

        usize path_len = string_length(data.paths[i]);
        char * texture_path = temp_alloc(path_len);
        copy_memory(texture_path, data.paths[i], path_len - 4);
        copy_memory(texture_path + path_len - 4, "png", 3);
        texture_path[path_len - 1] = 0;
        animations->sprite_sheet = load_texture(texture_path);
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
            cursor++;
            if (tokens[cursor].type != JSMN_ARRAY) {
                TraceLog(LOG_ERROR, "Expected frames to be within an array");
                goto next_file;
            }

            cursor++;
            int frame_object_index = cursor;
            Rectangle rect = {0};
            usize duration = 0;
            cursor++;

            // loading frames
            while (cursor < token_count) {
                if (cursor >= token_count || tokens[cursor].parent != frame_object_index) {
                    // finished loading frame data, save it into the array
                    AnimationFrame new = {
                        .duration = duration * 0.001f,
                        .source = rect,
                    };
                    listFrameAppend(frames, new);

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

                if (compare_literal(key, "frame")) {
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
            } // loading frames

            // skipping to tags
            StringSlice key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
            while (! compare_literal(key, "meta")) {
                if (cursor >= token_count) {
                    TraceLog(LOG_ERROR, "Couldn't find sprite sheet meta data");
                    goto next_file;
                }
                cursor ++;
                key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
            }
            cursor += 2;
            key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
            while (! compare_literal(key, "frameTags")) {
                if (cursor >= token_count) {
                    TraceLog(LOG_ERROR, "Couldn't find sprite sheet frame tag data");
                    goto next_file;
                }
                cursor = skip_tokens(tokens, cursor);
                key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
            }

            // loading tags
            cursor += 2;
            while (cursor < token_count) {
                if (tokens[cursor].type == JSMN_OBJECT) {
                    cursor += 1;
                    continue;
                }
                key = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                if (compare_literal(key, "name")) {
                    cursor ++;
                    unsigned char tag_type = text[tokens[cursor].start];
                    usize from;
                    usize to;
                    cursor += 2;
                    StringSlice value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_usize(value, &from)) {
                        log_slice(LOG_ERROR, "Failed to load tag start", value);
                        goto next_file;
                    }
                    cursor += 2;
                    value = make_slice_u(text, tokens[cursor].start, tokens[cursor].end);
                    if (convert_slice_usize(value, &to)) {
                        log_slice(LOG_ERROR, "Failed to load tag end", value);
                        goto next_file;
                    }
                    assert((to < animations->frames.len) && "Animation frames out of range");
                    switch (tag_type) {
                        case 'i': {
                            animations->idle_start = from;
                            for (usize tt = from; tt <= to; tt++) {
                                animations->idle_duration += animations->frames.items[tt].duration;
                            }
                        } break;
                        case 'a': {
                            animations->attack_start = from;
                            for (usize tt = from; tt <= to; tt++) {
                                animations->attack_duration += animations->frames.items[tt].duration;
                            }
                        } break;
                        case 's': {
                            animations->attack_trigger = from;
                        } break;
                        case 'b': {
                            animations->cast_trigger = from;
                        } break;
                        case 'm': {
                            animations->walk_start = from;
                            for (usize tt = from; tt <= to; tt++) {
                                animations->walk_duration += animations->frames.items[tt].duration;
                            }
                        } break;
                        case 'c': {
                            animations->cast_start = from;
                            for (usize tt = from; tt <= to; tt++) {
                                animations->cast_duration += animations->frames.items[tt].duration;
                            }
                        } break;
                        default: {
                            value = make_slice_u(text, tokens[cursor - 4].start, tokens[cursor - 4].end);
                            log_slice(LOG_ERROR, "Received unrecognized tag:", value);
                        } goto next_file;
                    }
                    cursor ++;
                }
                else {
                  cursor = skip_tokens(tokens, cursor);
                }
            } // loading tags
        }

        next_file:
        unload_asset(text);
        temp_reset();
    }
    unload_file_paths(data);
    return SUCCESS;
}
void unload_animation_set (AnimationSet * set) {
    listFrameDeinit(&set->frames);
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

        #if defined(EMBEDED_ASSETS)
        int len = 0;
        const unsigned char * data = load_asset(path, &len);
        assets->faction_themes[i] = LoadMusicStreamFromMemory(".xm", data, len);
        #else
        assets->faction_themes[i] = LoadMusicStream(path);
        #endif

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

    #if defined(EMBEDED_ASSETS)
    int len = 0;
    const unsigned char * data = load_asset(path, &len);
    assets->main_theme = LoadMusicStreamFromMemory(".xm", data, len);
    #else
    assets->main_theme = LoadMusicStream(path);
    #endif

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
        Sound s = load_sound(path);
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
    // defaults
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
    settings->fullscreen = true;
    #endif

    #if !defined(ANDROID)
    char * path = config_path("settings.conf", temp_alloc);
    char * file = LoadFileText(path);
    if (NULL != file) {
        StringSlice key;
        usize cursor = 0;
        usize expected_lines = 5;
        while (expected_lines --> 0) {
            if (tokenize(file, "\n =", &cursor, &key)) {
                TraceLog(LOG_ERROR, "Failed to find a settings key");
                goto end;
            }
            if (compare_literal(key, "volume-master")) {
                if (tokenize(file, "\n =", &cursor, &key)) {
                    log_slice(LOG_ERROR, "failed to get settings value for", key);
                    goto end;
                }
                if (convert_slice_float(key, &settings->volume_master)) {
                    log_slice(LOG_ERROR, "failed to convert value to master volume:", key);
                    goto end;
                }
            }
            else if (compare_literal(key, "volume-music")) {
                if (tokenize(file, "\n =", &cursor, &key)) {
                    log_slice(LOG_ERROR, "failed to get settings value for", key);
                    goto end;
                }
                if (convert_slice_float(key, &settings->volume_music)) {
                    log_slice(LOG_ERROR, "failed to convert value to music volume:", key);
                    goto end;
                }
            }
            else if (compare_literal(key, "volume-sfx")) {
                if (tokenize(file, "\n =", &cursor, &key)) {
                    log_slice(LOG_ERROR, "failed to get settings value for", key);
                    goto end;
                }
                if (convert_slice_float(key, &settings->volume_sfx)) {
                    log_slice(LOG_ERROR, "failed to convert value to sfx volume:", key);
                    goto end;
                }
            }
            else if (compare_literal(key, "volume-ui")) {
                if (tokenize(file, "\n =", &cursor, &key)) {
                    log_slice(LOG_ERROR, "failed to get settings value for", key);
                    goto end;
                }
                if (convert_slice_float(key, &settings->volume_ui)) {
                    log_slice(LOG_ERROR, "failed to convert value to ui volume:", key);
                    goto end;
                }
            }
            else if (compare_literal(key, "fullscreen")) {
                if (tokenize(file, "\n =", &cursor, &key)) {
                    log_slice(LOG_ERROR, "Failed to get settings value for", key);
                    goto end;
                }
                usize t = 0;
                if (convert_slice_usize(key, &t)) {
                    log_slice(LOG_ERROR, "Failed to read value for full screen:", key);
                    goto end;
                }
                settings->fullscreen = t != 0;
            }
        }
        end:
        UnloadFileText(file);
    }
    #endif
    settings->theme = theme_setup();
    return SUCCESS;
}
Result save_settings (const Settings * settings) {
    #if defined(ANDROID)
    (void)settings;
    #else
    char * path = config_path("settings.conf", &temp_alloc);
    char data_buffer[1024];
    int len = snprintf(
      data_buffer,
      1024,
      "volume-master=%.2f\nvolume-music=%.2f\nvolume-sfx=%.2f\nvolume-ui=%.2f\nfullscreen=%d",
      settings->volume_master, settings->volume_music, settings->volume_sfx, settings->volume_ui,
      settings->fullscreen
    );
    if (len <= 0) {
        TraceLog(LOG_ERROR, "Failed to create data for settings");
        return FAILURE;
    }
    bool result = SaveFileData(path, data_buffer, len);
    if (! result) {
        TraceLog(LOG_ERROR, "Failed to save settings");
        return FAILURE;
    }
    #endif
    return SUCCESS;
}
