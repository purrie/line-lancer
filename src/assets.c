#include <raylib.h>
#include <raymath.h>

#include "math.h"
#include "assets.h"
#include "std.h"

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

Model generate_building_mesh(const Vector2 pos, const float size, const float layer) {
  Mesh mesh = {0};

  {
    mesh.vertexCount = 4;
    mesh.vertices = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);

    for (usize i = 0; i < mesh.vertexCount; i++) {
      mesh.vertices[i * 3 + 2] = layer;
    }

    mesh.vertices[0] = pos.x - size;
    mesh.vertices[1] = pos.y - size;

    mesh.vertices[3] = pos.x + size;
    mesh.vertices[4] = pos.y - size;

    mesh.vertices[6] = pos.x - size;
    mesh.vertices[7] = pos.y + size;

    mesh.vertices[9]  = pos.x + size;
    mesh.vertices[10] = pos.y + size;
  }

  {
    mesh.texcoords = MemAlloc(sizeof(float) * 2 * mesh.vertexCount);
    mesh.texcoords[0] = 0.0f;
    mesh.texcoords[1] = 0.0f;

    mesh.texcoords[2] = 1.0f;
    mesh.texcoords[3] = 0.0f;

    mesh.texcoords[4] = 0.0f;
    mesh.texcoords[5] = 1.0f;

    mesh.texcoords[6] = 1.0f;
    mesh.texcoords[7] = 1.0f;
  }

  {
    mesh.triangleCount = 2;
    mesh.indices = MemAlloc(sizeof(ushort) * 3 * mesh.triangleCount);

    mesh.indices[0] = 0;
    mesh.indices[1] = 2;
    mesh.indices[2] = 1;

    mesh.indices[3] = 1;
    mesh.indices[4] = 2;
    mesh.indices[5] = 3;
  }

  UploadMesh(&mesh, false);

  return LoadModelFromMesh(mesh);
}

Model generate_line_mesh(const ListLine lines, float thickness, ushort cap_resolution, const float layer) {
  Mesh mesh = {0};

  const float thickness_half = thickness * 0.5f;
  const usize points = lines.len + 1 + cap_resolution * 2;

  // generating vertex positions
  {
    mesh.vertexCount = points * 2;
    mesh.vertices = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);

    TraceLog(LOG_INFO, "  Generating verticle positions");
    for (usize i = 0; i < points; i++) {
      // modifying 2 vertex positions at a time, each with 3 components
      const usize vert = i * 2 * 3;
      Vector2 v, a;
      Vector2 line;

      if (i < cap_resolution) {
        usize index = 0;
        a = lines.items[index].a;
        line = Vector2Subtract(lines.items[index].b, lines.items[index].a);
        line = Vector2Normalize(line);

        float step = (float)(cap_resolution - i) / (float)cap_resolution;
        float thick = 1.0f - (step * step * 0.9f);

        Vector2 offset = Vector2Scale(line, step);
        offset = Vector2Scale(offset, thickness_half);

        a = Vector2Subtract(a, offset);
        line = Vector2Scale(line, thick);
      }

      else if (i >= lines.len + cap_resolution) {
        usize index = lines.len - 1;
        a = lines.items[index].b;
        line = Vector2Subtract(lines.items[index].b, lines.items[index].a);
        line = Vector2Normalize(line);

        float step = (float)(i - lines.len - cap_resolution) / (float)cap_resolution;
        float thick = 1.0f - (step * step * 0.9f);

        Vector2 offset = Vector2Scale(line, step);
        offset = Vector2Scale(offset, thickness_half);

        a = Vector2Add(a, offset);
        line = Vector2Scale(line, thick);
      }

      else {
        usize index = i - cap_resolution;
        a = lines.items[index].a;
        line = Vector2Subtract(lines.items[index].b, a);

        usize last = lines.len ;
        if (index < last && index > 0 ) {
          v = Vector2Subtract(lines.items[index - 1].b, lines.items[index - 1].a);
          line = Vector2Slerp(line, v, Vector2Zero(), 0.5f);
        }

        line = Vector2Normalize(line);
      }

      line = Vector2Scale(line, thickness_half);

      v = Vector2PerpCounter(line);
      v = Vector2Add(v, a);
      mesh.vertices[vert] = v.x;
      mesh.vertices[vert + 1] = v.y;
      mesh.vertices[vert + 2] = layer;

      v = Vector2Perp(line);
      v = Vector2Add(v, a);
      mesh.vertices[vert + 3] = v.x;
      mesh.vertices[vert + 4] = v.y;
      mesh.vertices[vert + 5] = layer;

    }
  }
  // resolving mesh collisions
  {
    // make sure none of the lines cross each other
    TraceLog(LOG_INFO, "  Resolving mesh collisions");

    usize cursor = 1;
    for (usize i = 1; i < points - 1; i++) {
      usize right_index = i * 2 * 3;
      usize left_index = (i - cursor) * 2 * 3;

      Line right = {
        .a = { .x = mesh.vertices[right_index], .y = mesh.vertices[right_index + 1]},
        .b = { .x = mesh.vertices[right_index + 3], .y = mesh.vertices[right_index + 4]}
      };
      Line left = {
        .a = { .x = mesh.vertices[left_index], .y = mesh.vertices[left_index + 1]},
        .b = { .x = mesh.vertices[left_index + 3], .y = mesh.vertices[left_index + 4]}
      };

      OptionalVector2 intersection = get_line_intersection(left, right);
      usize intersections = 1;
      Vector2 center = Vector2Zero();

      while (intersection.has_value) {
        if (Vector2DistanceSqr(left.a, intersection.value) > Vector2DistanceSqr(left.b, intersection.value)) {
          center = Vector2Add(center, left.a);
        }
        else {
          center = Vector2Add(center, left.b);
        }
        intersections ++;
        cursor ++;

        if (cursor <= i) {
          left_index = ( i - cursor ) * 2 * 3;
          left = (Line){
            .a = { .x = mesh.vertices[left_index], .y = mesh.vertices[left_index + 1]},
            .b = { .x = mesh.vertices[left_index + 3], .y = mesh.vertices[left_index + 4]}
          };
        }
        else {
          break;
        }
      }

      if (intersections > 1) {
        if (Vector2DistanceSqr(center, right.a) > Vector2DistanceSqr(center, right.b)) {
          center = Vector2Add(center, right.a);
          center.x /= (float)intersections;
          center.y /= (float)intersections;
          mesh.vertices[right_index] = center.x;
          mesh.vertices[right_index + 1] = center.y;
        }
        else {
          center = Vector2Add(center, right.b);
          center.x /= (float)intersections;
          center.y /= (float)intersections;
          mesh.vertices[right_index + 3] = center.x;
          mesh.vertices[right_index + 4] = center.y;
        }

        while (cursor --> 1) {
          left_index = ( i - cursor ) * 2 * 3;
          left = (Line){
            .a = { .x = mesh.vertices[left_index], .y = mesh.vertices[left_index + 1]},
            .b = { .x = mesh.vertices[left_index + 3], .y = mesh.vertices[left_index + 4]}
          };

          if (Vector2DistanceSqr(center, left.a) > Vector2DistanceSqr(center, left.b)) {
            mesh.vertices[left_index] = center.x;
            mesh.vertices[left_index + 1] = center.y;
          }
          else {
            mesh.vertices[left_index + 3] = center.x;
            mesh.vertices[left_index + 4] = center.y;
          }
        }
      }
    }
  }
  // generating triangles
  {
    TraceLog(LOG_INFO, "  Generating indices");
    mesh.triangleCount = lines.len * 2 + cap_resolution * 4;
    mesh.indices = MemAlloc(sizeof(ushort) * 3 * mesh.triangleCount);

    usize quads = mesh.triangleCount / 2;
    for (usize i = 0; i < quads; i++) {
      usize tris = i * 6;
      ushort verts = i * 2;
      mesh.indices[tris] = verts;
      mesh.indices[tris + 1] = verts + 2;
      mesh.indices[tris + 2] = verts + 1;

      mesh.indices[tris + 3] = verts + 1;
      mesh.indices[tris + 4] = verts + 2;
      mesh.indices[tris + 5] = verts + 3;
    }
  }

  UploadMesh(&mesh, false);
  return LoadModelFromMesh(mesh);
}

Model generate_area_mesh(const Area *const area, const float layer) {
  Mesh mesh = {0};
  const ListLine lines = area->lines;

  {
    mesh.vertexCount = lines.len;
    mesh.vertices    = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);

    for (usize i = 0; i < lines.len; i++) {
      usize vert = i * 3;
      mesh.vertices[vert]     = lines.items[i].a.x;
      mesh.vertices[vert + 1] = lines.items[i].a.y;
      mesh.vertices[vert + 2] = layer;
    }
  }

  {
    usize initial_cap  = lines.len * 2 * 3;
    ListUshort indices = listUshortInit(initial_cap, &MemAlloc, &MemFree);
    ListUsize points   = listUsizeInit(lines.len, &MemAlloc, &MemFree);
    usize index   = 0;
    usize counter = 0;

    for (usize i = 0; i < lines.len; i ++) {
      listUsizeAppend(&points, i);
    }

    while (points.len > 2) {
      usize index_m = (index + 1) % points.len;
      usize index_e = (index + 2) % points.len;

      usize point_ib = points.items[index];
      usize point_im = points.items[index_m];
      usize point_ie = points.items[index_e];

      Vector2 start = lines.items[point_ib].a;
      Vector2 end   = lines.items[point_ie].a;
      Vector2 mid   = Vector2Lerp(start, end, 0.5f);

      if (area_contains_point(area, mid) == false) {
        if (counter > lines.len) {
          TraceLog(LOG_ERROR, "Failed to generate mesh for area");
          listUshortDeinit(&indices);
          listUsizeDeinit(&points);
          MemFree(mesh.vertices);
          // TODO do proper error handling
          return (Model){0};
        }
        counter ++;
        index = index_m;
        continue;
      }
      counter = 0;

      listUshortAppend(&indices, point_ib);
      listUshortAppend(&indices, point_im);
      listUshortAppend(&indices, point_ie);
      listUsizeRemove (&points , index_m);
    }

    mesh.triangleCount = indices.len / 3;
    mesh.indices = MemAlloc(sizeof(ushort) * indices.len);
    copy_memory(mesh.indices, indices.items, sizeof(ushort) * indices.len);

    listUshortDeinit(&indices);
    listUsizeDeinit(&points);
  }

  UploadMesh(&mesh, false);
  return LoadModelFromMesh(mesh);
}

#define LAYER_MAP -0.3f
#define LAYER_PATH -0.2f
#define LAYER_BUILDING -0.1f

void generate_map_mesh(Map * map) {
  // TODO sizes and thickness will depend on the map size I imagine
  TraceLog(LOG_INFO, "Generating meshes");
  for (usize i = 0; i < map->paths.len; i++) {
    TraceLog(LOG_INFO, "  Generating path mesh #%d", i);
    map->paths.items[i].model = generate_line_mesh(map->paths.items[i].lines, 20.0f, 3, LAYER_PATH);
  }
  for (usize i = 0; i < map->regions.len; i++) {
    TraceLog(LOG_INFO, "Generating region #%d", i);
    TraceLog(LOG_INFO, "  Generating region mesh");
    Region * region = &map->regions.items[i];
    region->area.model = generate_area_mesh(&region->area, LAYER_MAP);

    TraceLog(LOG_INFO, "  Generating castle mesh");
    region->castle.model = generate_building_mesh(region->castle.position, 10.0f, LAYER_BUILDING);

    for (usize b = 0; b < region->buildings.len; b++) {
      TraceLog(LOG_INFO, "  Generating building mesh #%d", b);
      region->buildings.items[b].model = generate_building_mesh(region->buildings.items[b].position, 10.0f, LAYER_BUILDING);
    }
  }
}

void subdivide_map_paths(Map * map) {
  for(usize i = 0; i < map->paths.len; i++) {
    Vector2 a = map->paths.items[i].lines.items[0].a;
    Vector2 b = map->paths.items[i].lines.items[0].b;
    float depth = Vector2DistanceSqr(a, b);
    for (usize l = 1; l < map->paths.items[i].lines.len; l++) {
      a = map->paths.items[i].lines.items[l].a;
      b = map->paths.items[i].lines.items[l].b;
      float test = Vector2DistanceSqr(a, b);
      if (test < depth) depth = test;
    }
    depth = sqrtf(depth) * 0.25f;

    bevel_lines(&map->paths.items[i].lines, 5, depth, false);
  }

  for (usize i = 0; i < map->regions.len; i++) {
    Vector2 a = map->regions.items[i].area.lines.items[0].a;
    Vector2 b = map->regions.items[i].area.lines.items[0].b;
    float depth = Vector2DistanceSqr(a, b);
    for (usize l = 1; l < map->regions.items[i].area.lines.len; l++) {
      a = map->regions.items[i].area.lines.items[l].a;
      b = map->regions.items[i].area.lines.items[l].b;
      float test = Vector2DistanceSqr(a, b);
      if (test < depth) depth = test;
    }
    depth = sqrtf(depth) * 0.25f;

    bevel_lines(&map->regions.items[i].area.lines, 5, depth, true);
  }
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
  map_clamp(&map.value);
  subdivide_map_paths(&map.value);
  generate_map_mesh(&map.value);
  return map;

fail:
  TraceLog(LOG_ERROR, "Loading map %s failed", path);
  UnloadFileData(data);
  level_unload(&map.value);
  return map;
}

void level_unload(Map * map) {
    for (usize i = 0; i < map->regions.len; i++) {
        for (usize b = 0; b < map->regions.items[i].buildings.len; ++b) {
            UnloadModel(map->regions.items[i].buildings.items[b].model);
        }
        listBuildingDeinit(&map->regions.items[i].buildings);

        UnloadModel(map->regions.items[i].castle.model);

        UnloadModel(map->regions.items[i].area.model);
        listLineDeinit(&map->regions.items[i].area.lines);
    }

    for (usize i = 0; i < map->paths.len; i++) {
        UnloadModel(map->paths.items[i].model);
        listLineDeinit(&map->paths.items[i].lines);
    }

    listPathDeinit(&map->paths);
    listRegionDeinit(&map->regions);
}
