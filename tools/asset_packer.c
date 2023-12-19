#include <raylib.h>
#include <stdio.h>

int main (int argc, char ** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [Asset Folder Path, ..Paths]\n", argv[0]);
        fprintf(stderr, "  Asset packer will read data from provided files and convert them into a C header file\n");
        fprintf(stderr, "  Resulting header file will be directed to stdout");
        return 1;
    }
    SetTraceLogLevel(LOG_NONE);

    printf("#ifndef ASSET_PACKER_H_\n");
    printf("#define ASSET_PACKER_H_\n");
    printf("#include <raylib.h>\n");
    printf("const unsigned char * load_data_from_path (const char * path, int * len) {\n");
    for (size_t i = 1; i < argc; i++) {
        int len = 0;
        unsigned char * data = LoadFileData(argv[i], &len);
        if (len <= 0) {
            UnloadFileData(data);
            continue;
        }
        printf("  if (TextIsEqual(path, \"%s\")) {\n", argv[i]);
        printf("    static const unsigned char ret[%d] = { ", len+1);
        for (size_t d = 0; d < len; d++) {
            printf("%d, ", data[d]);
        }
        printf("0 };\n");
        printf("    *len = %d;\n", len);
        printf("    return ret;\n");
        printf("  }\n");
        UnloadFileData(data);
    }
    printf("  return (void*)0;\n}\n");

    printf("FilePathList load_map_path_list () {\n");
    FilePathList maps = LoadDirectoryFiles("assets/maps");
    printf("  static char * maps[%d] = {\n", maps.count);
    for (size_t i = 0; i < maps.count; i++) {
        printf("    \"%s\",\n", maps.paths[i]);
    }
    printf("  };\n");
    printf("  FilePathList ret = { .capacity = %d, .count = %d, .paths = maps };\n", maps.count, maps.count);
    printf("  return ret;\n}\n");
    UnloadDirectoryFiles(maps);

    printf("FilePathList load_unit_meta_file_paths () {\n");
    FilePathList units = LoadDirectoryFilesEx("assets/units", ".json", false);
    printf("  static char * paths[%d] = {\n", units.count);
    for (size_t i = 0; i < units.count; i++) {
        printf("    \"%s\", \n", units.paths[i]);
    }
    printf("  };\n");
    printf("  FilePathList ret = { .capacity = %d, .count = %d, .paths = paths };\n", units.count, units.count);
    printf("  return ret;\n}\n");
    UnloadDirectoryFiles(units);

    printf("#endif // ASSET_PACKER_H_");
    return 0;
}
