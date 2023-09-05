#include "mesh.h"
#include <raymath.h>
#include "alloc.h"
#include "math.h"
#include "std.h"
#include "level.h"
#include "constants.h"

Model generate_building_mesh(const Vector2 pos, const float b_size, const float layer) {
  Mesh mesh = {0};

  {
    usize vcount = 4;
    mesh.vertexCount = vcount;
    mesh.vertices = MemAlloc(sizeof(float) * 3 * mesh.vertexCount);

    for (usize i = 0; i < vcount; i++) {
      mesh.vertices[i * 3 + 2] = layer;
    }
    float half_size = b_size * 0.5f;

    mesh.vertices[0] = pos.x - half_size;
    mesh.vertices[1] = pos.y - half_size;

    mesh.vertices[3] = pos.x + half_size;
    mesh.vertices[4] = pos.y - half_size;

    mesh.vertices[6] = pos.x - half_size;
    mesh.vertices[7] = pos.y + half_size;

    mesh.vertices[9]  = pos.x + half_size;
    mesh.vertices[10] = pos.y + half_size;
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

Test is_clockwise (Vector2 a, Vector2 b, Vector2 c) {
  double val = 0.0;
  val = (b.x * c.y + a.x * b.y + a.y * c.x) - (a.y * b.x + b.y * c.x + a.x * c.y);
  return val > 0.0f ? YES : NO;
}

Test is_area_clockwise(Area *const area) {
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

Model generate_area_mesh(Area *const area, const float layer) {
  temp_reset();
  Mesh mesh = {0};
  const ListLine lines = area->lines;
  const Test clockwise = is_area_clockwise(area);
  if (clockwise)
    TraceLog(LOG_INFO, "  Area is clockwise");
  else
    TraceLog(LOG_INFO, "  Area is counter-clockwise");
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

void generate_map_mesh(Map * map) {
  // TODO sizes and thickness will depend on the map size I imagine
  TraceLog(LOG_INFO, "Generating meshes");
  for (usize i = 0; i < map->paths.len; i++) {
    TraceLog(LOG_INFO, "  Generating path mesh #%d", i);
    map->paths.items[i].model = generate_line_mesh(map->paths.items[i].lines, (float)PATH_THICKNESS, 3, LAYER_PATH);
  }
  float b_size = building_size();
  for (usize i = 0; i < map->regions.len; i++) {
    TraceLog(LOG_INFO, "Generating region #%d", i);
    TraceLog(LOG_INFO, "  Generating region mesh");
    Region * region = &map->regions.items[i];
    region->area.model = generate_area_mesh(&region->area, LAYER_MAP);

    for (usize b = 0; b < region->buildings.len; b++) {
      TraceLog(LOG_INFO, "  Generating building mesh #%d", b);
      region->buildings.items[b].model = generate_building_mesh(region->buildings.items[b].position, b_size, LAYER_BUILDING);
    }
  }
}
