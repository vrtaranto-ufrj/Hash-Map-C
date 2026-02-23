#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_PRIME 0x00000100000001b3UL
#define FNV_OFFSET_BASIS 0xcbf29ce484222325UL

#define INITIAL_CAPACITY 16ULL
#define RESIZE_THRESHOLD 70UL
#define INITIAL_ARENA_CAPACITY 1024ULL

#define TOMBSTONE_FLAG 0b00000001
#define USED_FLAG 0b00000010

#define is_tombstone(item_flag) (((item_flag) & TOMBSTONE_FLAG) != 0)
#define is_used(item_flag) (((item_flag) & USED_FLAG) != 0)

#ifdef HASHMAP_BENCH
static size_t hashmap_bench_add_probes = 0;
static size_t hashmap_bench_get_probes = 0;
static size_t hashmap_bench_pop_probes = 0;
static size_t hashmap_bench_resizes = 0;
static size_t hashmap_bench_mallocs = 0;
static size_t hashmap_bench_frees = 0;
static size_t hashmap_bench_alloc_bytes = 0;
static int hashmap_bench_in_resize = 0;

static inline void hashmap_bench_reset(void) {
    hashmap_bench_add_probes = 0;
    hashmap_bench_get_probes = 0;
    hashmap_bench_pop_probes = 0;
    hashmap_bench_resizes = 0;
    hashmap_bench_mallocs = 0;
    hashmap_bench_frees = 0;
    hashmap_bench_alloc_bytes = 0;
    hashmap_bench_in_resize = 0;
}
#endif

typedef struct HashMapStruct HashMap;
typedef struct ItemStruct Item;
typedef struct HashMapReturnStruct HashMapReturn;

typedef struct ArenaStruct Arena;
typedef struct StringStruct String;
typedef size_t ArenaOffset;

struct ItemStruct {
    ArenaOffset key;
    int value;
    uint8_t flags;
};

struct HashMapStruct {
    Item *array;
    size_t len;
    size_t capacity;
    Arena *arena;
};

struct HashMapReturnStruct {
    int value;
    bool found;
};

struct ArenaStruct {
    char *memory;
    ArenaOffset capacity;
    ArenaOffset used;
};

struct StringStruct {
    const char *string;
    size_t len;
};

HashMap create_hashmap();
void free_hashmap(HashMap *hashmap);

void add_hashmap(HashMap *hashmap, const char *key, int value);
HashMapReturn get_hashmap(HashMap *hashmap, const char *key);
HashMapReturn pop_hashmap(HashMap *hashmap, const char *key);
void clear_hashmap(HashMap *hashmap);

bool _resize_needed(HashMap *hashmap);
void _resize_hashmap(HashMap *hashmap);
uint64_t _hash_func(const char *key, uint64_t capacity);
void _set_item(HashMap *hashmap, Item *item, const char *key, int value);
HashMap _alloc_hashmap(size_t capacity, Arena *arena);
void _free_hashmap(HashMap *hashmap, bool free_arena);

Arena _create_arena(size_t initial_capacity);
void _free_arena(Arena *arena);
void _clear_arena(Arena *arena);

bool _arena_resize(Arena *arena);
ArenaOffset _arena_alloc_string(Arena *arena, const char *str, size_t len);
String _arena_get_string(Arena *arena, size_t offset);
bool _arena_string_equals(Arena *arena, size_t offset, const char *key);

HashMap create_hashmap() { return _alloc_hashmap(INITIAL_CAPACITY, NULL); }

void free_hashmap(HashMap *hashmap) {
    _free_hashmap(hashmap, true);
}

void _free_hashmap(HashMap *hashmap, bool free_arena) {
    free(hashmap->array);
    if (free_arena) {
        _free_arena(hashmap->arena);
        free(hashmap->arena);
    }
#ifdef HASHMAP_BENCH
    if (hashmap->array) {
        hashmap_bench_frees++;
    }
#endif
    hashmap->len = 0;
    hashmap->capacity = 0;
    hashmap->array = NULL;
}

void add_hashmap(HashMap *hashmap, const char *key, int value) {
    if (_resize_needed(hashmap)) {
        _resize_hashmap(hashmap);
    }
    size_t index = _hash_func(key, hashmap->capacity);
    Item *first_tombstone = NULL;

    for (size_t i = 0; i < hashmap->capacity; i++) {
#ifdef HASHMAP_BENCH
        if (!hashmap_bench_in_resize) {
            hashmap_bench_add_probes++;
        }
#endif
        Item *item = &hashmap->array[(i + index) % hashmap->capacity];

        if (is_tombstone(item->flags)) {
            if (!first_tombstone) {
                first_tombstone = item;
            }
            continue;
        }

        if (is_used(item->flags)) {
            if (_arena_string_equals(hashmap->arena, item->key, key)) {
                _set_item(hashmap, item, key, value);
                return;
            }
            continue;
        }

        Item *target = first_tombstone ? first_tombstone : item;
        hashmap->len++;
        _set_item(hashmap, target, key, value);
        return;
    }

    if (first_tombstone) {
        hashmap->len++;
        _set_item(hashmap, first_tombstone, key, value);
    }
}

HashMapReturn get_hashmap(HashMap *hashmap, const char *key) {
    size_t index = _hash_func(key, hashmap->capacity);

    for (size_t i = 0; i < hashmap->capacity; i++) {
#ifdef HASHMAP_BENCH
        hashmap_bench_get_probes++;
#endif
        Item *item = &hashmap->array[(i + index) % hashmap->capacity];

        if (is_tombstone(item->flags)) {
            continue;
        }

        if (!is_used(item->flags)) {
            break;
        }

        if (_arena_string_equals(hashmap->arena, item->key, key)) {
            return (HashMapReturn){.found = true, .value = item->value};
        }
    }

    return (HashMapReturn){.found = false};
}

HashMapReturn pop_hashmap(HashMap *hashmap, const char *key) {
    size_t index = _hash_func(key, hashmap->capacity);

    for (size_t i = 0; i < hashmap->capacity; i++) {
#ifdef HASHMAP_BENCH
        hashmap_bench_pop_probes++;
#endif
        Item *item = &hashmap->array[(i + index) % hashmap->capacity];

        if (is_tombstone(item->flags)) {
            continue;
        }

        if (!is_used(item->flags)) {
            break;
        }

        if (_arena_string_equals(hashmap->arena, item->key, key)) {
            item->flags = TOMBSTONE_FLAG;

            HashMapReturn ret = {.found = true, .value = item->value};
#ifdef HASHMAP_BENCH
            hashmap_bench_frees++;
#endif
            hashmap->len--;

            return ret;
        }
    }

    return (HashMapReturn){.found = false};
}

void clear_hashmap(HashMap *hashmap) {
    _clear_arena(hashmap->arena);
    memset(hashmap->array, 0, sizeof(Item) * hashmap->capacity);
    hashmap->len = 0;
}

bool _resize_needed(HashMap *hashmap) {
    return hashmap->len * 100UL >= RESIZE_THRESHOLD * hashmap->capacity;
}

void _resize_hashmap(HashMap *hashmap) {
    HashMap old_hashmap = *hashmap;

#ifdef HASHMAP_BENCH
    hashmap_bench_resizes++;
    hashmap_bench_in_resize = 1;
#endif

    *hashmap = _alloc_hashmap(hashmap->capacity * 2, hashmap->arena);

    for (size_t i = 0; i < old_hashmap.capacity; i++) {
        Item *item = &old_hashmap.array[i];
        if (is_used(item->flags)) {
            add_hashmap(hashmap, _arena_get_string(hashmap->arena, item->key).string, item->value);
        }
    }

#ifdef HASHMAP_BENCH
    hashmap_bench_in_resize = 0;
#endif

    _free_hashmap(&old_hashmap, false);
}

uint64_t _hash_func(const char *key, uint64_t capacity) {
    uint64_t hash_value = FNV_OFFSET_BASIS;

    for (const char *c = key; *c != '\0'; c++) {
        hash_value ^= *c;
        hash_value *= FNV_PRIME;
    }

    return hash_value & (capacity - 1);
}

void _set_item(HashMap *hashmap, Item *item, const char *key, int value) {
    if (!is_used(item->flags)) {
        item->flags &= ~TOMBSTONE_FLAG;
        item->flags |= USED_FLAG;
        item->key = _arena_alloc_string(hashmap->arena, key, strlen(key));
    }

    item->value = value;
}

HashMap _alloc_hashmap(size_t capacity, Arena *arena) {
    Item *array = calloc(capacity, sizeof(Item));
    if (!arena) {
        arena = malloc(sizeof(Arena));
        *arena = _create_arena(INITIAL_ARENA_CAPACITY);
    }
#ifdef HASHMAP_BENCH
    if (array) {
        hashmap_bench_mallocs++;
        hashmap_bench_alloc_bytes += capacity * sizeof(Item);
    }
#endif
    return (HashMap){
        .capacity = capacity, .array = array, .len = 0, .arena = arena};
}

Arena _create_arena(size_t initial_capacity) {
    char *memory = malloc(initial_capacity);
    if (!memory) {
        return (Arena){0};
    }
    return (Arena){.capacity = initial_capacity, .used = 0, .memory = memory};
}

void _free_arena(Arena *arena) {
    free(arena->memory);
    arena->capacity = 0;
    arena->used = 0;
}

void _clear_arena(Arena *arena) { arena->used = 0; }

bool _arena_resize(Arena *arena) {
    char *new_memory = realloc(arena->memory, arena->capacity * 2);
    if (!new_memory) {
        return false;
    }
    arena->capacity *= 2;
    arena->memory = new_memory;

    return true;
}

ArenaOffset _arena_alloc_string(Arena *arena, const char *str, size_t len) {
    while (arena->used + len + 1 + sizeof(size_t) > arena->capacity) {
        if (!_arena_resize(arena)) {
            abort();
        }
    }
    size_t old_used = arena->used;

    *(size_t *)(arena->memory + old_used) = len;
    memcpy(arena->memory + old_used + sizeof(size_t), str, len + 1);

    arena->used += sizeof(len) + len + 1;

    return old_used;
}

String _arena_get_string(Arena *arena, ArenaOffset offset) {
    return (String){.len = *(size_t *)(arena->memory + offset),
                    .string = arena->memory + offset + sizeof(size_t)};
}

bool _arena_string_equals(Arena *arena, ArenaOffset offset, const char *key) {
    String str = _arena_get_string(arena, offset);
    size_t key_len = strlen(key);

    if (str.len != key_len)
        return false;

    return memcmp(str.string, key, key_len) == 0;
}
