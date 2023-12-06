#include "mesh.h"
#include <raymath.h>
#include "alloc.h"
#include "math.h"
#include "std.h"
#include "level.h"
#include "constants.h"

/* Utilities *****************************************************************/
Test is_clockwise (Vector2 a, Vector2 b, Vector2 c) {
  double val = 0.0;
  val = (b.x * c.y + a.x * b.y + a.y * c.x) - (a.y * b.x + b.y * c.x + a.x * c.y);
  return val > 0.0f ? YES : NO;
}
Test is_area_clockwise(const Area * area) {
  unsigned int cl = 0;
  unsigned int cc = 0;
  for (usize i = 0; i < area->lines.len; i++) {
    Vector2 a = area->lines.items[i].a;
    Vector2 b = area->lines.items[(i + 1) % area->lines.len].a;
    Vector2 c = area->lines.items[(i + 2) % area->lines.len].a;
    if (is_clockwise(a, b, c))
      cl ++;
    else
      cc ++;
  }
  return cl > cc ? YES : NO;
}

/* Generators ****************************************************************/
Model generate_line_mesh(const ListLine lines, float thickness, ushort cap_resolution, const float layer) {
  Mesh mesh = {0};

  const float thickness_half = thickness * 0.5f;
  const usize points = lines.len + 1 + cap_resolution * 2;
  const bool first_bot = lines.items[0].a.x < lines.items[lines.len - 1].b.x;

  // generating vertex positions
  {
    mesh.vertexCount = points * 2;
    mesh.vertices  = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);
    mesh.texcoords = MemAlloc(sizeof(float) * 2 * mesh.vertexCount);

    TraceLog(LOG_INFO, "  Generating verticle positions");
    for (usize i = 0; i < points; i++) {
      // modifying 2 vertex positions at a time, each with 3 components
      const usize vert = i * 2 * 3;
      Vector2 anchor_position;
      Vector2 line;

      if (i <= cap_resolution) {
        usize index = 0;
        anchor_position = lines.items[index].a;
        line = Vector2Subtract(lines.items[index].b, anchor_position);
        line = Vector2Normalize(line);

        if (cap_resolution > 0) {
          float step = (float)(cap_resolution - i) / (float)cap_resolution;
          float thick = 1.0f - (step * step * 0.9f);

          Vector2 offset = Vector2Scale(line, step);
          offset = Vector2Scale(offset, thickness_half);

          anchor_position = Vector2Subtract(anchor_position, offset);
          line = Vector2Scale(line, thick);
        }
      }

      else if (i >= lines.len + cap_resolution) {
        usize index = lines.len - 1;
        anchor_position = lines.items[index].b;
        line = Vector2Subtract(lines.items[index].b, lines.items[index].a); // extending past end
        line = Vector2Normalize(line);

        if (cap_resolution > 0) {
          float step = (float)(i - lines.len - cap_resolution) / (float)cap_resolution;
          float thick = 1.0f - (step * step * 0.9f);

          Vector2 offset = Vector2Scale(line, step);
          offset = Vector2Scale(offset, thickness_half);

          anchor_position = Vector2Add(anchor_position, offset);
          line = Vector2Scale(line, thick);
        }
      }

      else {
        usize index = i - cap_resolution;
        anchor_position = lines.items[index].a;
        line = Vector2Subtract(lines.items[index].b, anchor_position);

        usize last = lines.len ;
        if (index < last && index > 0 ) {
          Vector2 v = Vector2Subtract(lines.items[index - 1].b, lines.items[index - 1].a);
          line = Vector2Slerp(line, v, Vector2Zero(), 0.5f);
        }

        line = Vector2Normalize(line);
      }

      line = Vector2Scale(line, thickness_half);

      Vector2 top_pos = Vector2PerpCounter(line);
      Vector2 bot_pos = Vector2Perp(line);
      top_pos = Vector2Add(top_pos, anchor_position);
      bot_pos = Vector2Add(bot_pos, anchor_position);

      mesh.vertices[vert]     = top_pos.x;
      mesh.vertices[vert + 1] = top_pos.y;
      mesh.vertices[vert + 2] = layer;

      mesh.vertices[vert + 3] = bot_pos.x;
      mesh.vertices[vert + 4] = bot_pos.y;
      mesh.vertices[vert + 5] = layer;

      const usize uv = i * 2 * 2; // setting 2 points at a time, each having 2 elements
      float progress = (float)i / (float)(points - 1);
      mesh.texcoords[uv] = progress;
      mesh.texcoords[uv + 2] = progress;

      if (first_bot) {
        mesh.texcoords[uv + 1] = 1.0f;
        mesh.texcoords[uv + 3] = 0;
      }
      else {
        mesh.texcoords[uv + 1] = 0;
        mesh.texcoords[uv + 3] = 1.0f;
      }
    }
  }
  // resolving mesh collisions
  if (false){
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

      Vector2 intersection;
      Result intersects = line_intersection(left, right, &intersection);
      usize intersections = 1;
      Vector2 center = Vector2Zero();

      while (intersects == SUCCESS) {
        if (Vector2DistanceSqr(left.a, intersection) > Vector2DistanceSqr(left.b, intersection)) {
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
        intersects = line_intersection(left, right, &intersection);
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
Model generate_area_mesh(const Area * area, const float layer) {
  temp_reset();
  Mesh mesh = {0};
  const ListLine lines = area->lines;
  const Test clockwise = is_area_clockwise(area);
  {
    mesh.vertexCount = lines.len;
    mesh.vertices    = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);
    mesh.texcoords   = MemAlloc(sizeof(float) * 2 * mesh.vertexCount);

    for (usize i = 0; i < lines.len; i++) {
      usize vert = i * 3;
      mesh.vertices[vert]     = lines.items[i].a.x;
      mesh.vertices[vert + 1] = lines.items[i].a.y;
      mesh.vertices[vert + 2] = layer;
      usize uv = i * 2;
      mesh.texcoords[uv]     = lines.items[i].a.x * 0.01;
      mesh.texcoords[uv + 1] = lines.items[i].a.y * 0.01;
    }
  }

  {
    ListUshort indices = listUshortInit(3, perm_allocator());
    ListUsize points   = listUsizeInit(lines.len, perm_allocator());
    usize index   = 0;
    usize counter = 0;

    for (usize i = 0; i < lines.len; i ++) {
      listUsizeAppend(&points, i);
    }

    while (points.len > 2) {
      usize index_m = (index + (clockwise ? points.len - 1 : 1)) % points.len;
      usize index_e = (index + (clockwise ? points.len - 2 : 2)) % points.len;

      usize point_ib = points.items[index];
      usize point_im = points.items[index_m];
      usize point_ie = points.items[index_e];

      Vector2 start = lines.items[point_ib].a;
      Vector2 mid   = lines.items[point_im].a;
      Vector2 end   = lines.items[point_ie].a;

      Test tri_clockwise = is_clockwise(start, mid, end);
      Vector2 test_point = Vector2Lerp(start, end, 0.5f);
      Test outside = ! area_contains_point(area, test_point);
      Line test_line = {
        Vector2MoveTowards(start, end, 1.0f),
        Vector2MoveTowards(end, start, 1.0f)
      };
      Test cuts_through = area_line_intersects(area, test_line);
      Test invalid = tri_clockwise || outside || cuts_through;

      if (invalid) {
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
      if (index > index_m)
        index--;
    }

    mesh.triangleCount = indices.len / 3;
    TraceLog(LOG_INFO, "Built %d triangles", mesh.triangleCount);
    mesh.indices = MemAlloc(sizeof(ushort) * indices.len);
    copy_memory(mesh.indices, indices.items, sizeof(ushort) * indices.len);

    listUshortDeinit(&indices);
    listUsizeDeinit(&points);
  }

  UploadMesh(&mesh, false);
  return LoadModelFromMesh(mesh);
}
void generate_background_mesh (Map * map) {
    TraceLog(LOG_INFO, "  Building background [%zu, %zu]", map->width, map->height);
    Mesh mesh = {0};

    float left   = -(float)map->width;
    float top    = -(float)map->height;
    float right  = map->width * 2.0f;
    float bottom = map->height * 2.0f;

    mesh.vertexCount = 4;
    mesh.vertices    = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);

    mesh.vertices[0] = left;
    mesh.vertices[1] = top;
    mesh.vertices[2] = LAYER_BACKGROUND;

    mesh.vertices[3] = left;
    mesh.vertices[4] = bottom;
    mesh.vertices[5] = LAYER_BACKGROUND;

    mesh.vertices[6] = right;
    mesh.vertices[7] = bottom;
    mesh.vertices[8] = LAYER_BACKGROUND;

    mesh.vertices[9]  = right;
    mesh.vertices[10] = top;
    mesh.vertices[11] = LAYER_BACKGROUND;

    left   = 0;
    top    = 0;
    bottom = map->height * 0.01;
    right  = map->width * 0.01;

    mesh.texcoords    = MemAlloc(sizeof(float) * 2 * mesh.vertexCount);

    mesh.texcoords[0] = left;
    mesh.texcoords[1] = top;

    mesh.texcoords[2] = left;
    mesh.texcoords[3] = bottom;

    mesh.texcoords[4] = right;
    mesh.texcoords[5] = bottom;

    mesh.texcoords[6] = right;
    mesh.texcoords[7] = top;

    mesh.triangleCount = 2;
    mesh.indices = MemAlloc(sizeof(ushort) * 3 * mesh.triangleCount);
    mesh.indices[0] = 0;
    mesh.indices[1] = 1;
    mesh.indices[2] = 2;
    mesh.indices[3] = 0;
    mesh.indices[4] = 2;
    mesh.indices[5] = 3;

    UploadMesh(&mesh, false);
    map->background = LoadModelFromMesh(mesh);
}

/* Handlers ******************************************************************/
void generate_map_mesh(Map * map) {
  TraceLog(LOG_INFO, "Generating meshes");
  generate_background_mesh(map);
  for (usize i = 0; i < map->paths.len; i++) {
    TraceLog(LOG_INFO, "  Generating path mesh #%d", i);
    map->paths.items[i].model = generate_line_mesh(map->paths.items[i].lines, PATH_THICKNESS, 0, LAYER_PATH);
  }
  for (usize i = 0; i < map->regions.len; i++) {
    TraceLog(LOG_INFO, "Generating region #%d", i);
    TraceLog(LOG_INFO, "  Generating region mesh");
    Region * region = &map->regions.items[i];
    region->area.model = generate_area_mesh(&region->area, LAYER_MAP);
    region->area.outline = generate_line_mesh(region->area.lines, PATH_THICKNESS * 0.1f, 3, LAYER_MAP_OUTLINE);
  }
}
