#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_PRIME 0x00000100000001b3UL
#define FNV_OFFSET_BASIS 0xcbf29ce484222325UL

#define INITIAL_CAPACITY 16ULL
#define RESIZE_THRESHOLD 70UL

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

struct ItemStruct {
    char *key;
    int value;
    uint8_t flags;
};

struct HashMapStruct {
    Item *array;
    size_t len;
    size_t capacity;
};

struct HashMapReturnStruct {
    int value;
    bool found;
};

struct ArenaStruct {
    char *memory;
    size_t capacity;
    size_t used;
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
void _set_item(Item *item, const char *key, int value);
HashMap _alloc_hashmap(size_t capacity);

Arena _create_arena(size_t initial_capacity);
void _free_arena(Arena *arena);
void _clear_arena(Arena *arena);

bool _arena_resize(Arena *arena);
size_t _arena_alloc_string(Arena *arena, const char *str, size_t len);
String _arena_get_string(Arena *arena, size_t offset);
bool _arena_string_equals(Arena *arena, size_t offset, const char *key);

HashMap create_hashmap() { return _alloc_hashmap(INITIAL_CAPACITY); }

void free_hashmap(HashMap *hashmap) {
    for (size_t i = 0; i < hashmap->capacity; i++) {
        Item *item = &hashmap->array[i];

        if (is_used(item->flags)) {
            free(item->key);
#ifdef HASHMAP_BENCH
            hashmap_bench_frees++;
#endif
        }
    }

    free(hashmap->array);
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
            if (strcmp(item->key, key) == 0) {
                _set_item(item, key, value);
                return;
            }
            continue;
        }

        Item *target = first_tombstone ? first_tombstone : item;
        hashmap->len++;
        _set_item(target, key, value);
        return;
    }

    if (first_tombstone) {
        hashmap->len++;
        _set_item(first_tombstone, key, value);
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

        if (strcmp(item->key, key) == 0) {
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

        if (strcmp(item->key, key) == 0) {
            item->flags = TOMBSTONE_FLAG;

            HashMapReturn ret = {.found = true, .value = item->value};
            free(item->key);
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
    for (size_t i = 0; i < hashmap->capacity; i++) {
        Item *item = &hashmap->array[i];

        if (is_used(item->flags)) {
            free(item->key);
#ifdef HASHMAP_BENCH
            hashmap_bench_frees++;
#endif
        }
    }

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

    *hashmap = _alloc_hashmap(hashmap->capacity * 2);

    for (size_t i = 0; i < old_hashmap.capacity; i++) {
        Item *item = &old_hashmap.array[i];
        if (is_used(item->flags)) {
            add_hashmap(hashmap, item->key, item->value);
        }
    }

#ifdef HASHMAP_BENCH
    hashmap_bench_in_resize = 0;
#endif

    free_hashmap(&old_hashmap);
}

uint64_t _hash_func(const char *key, uint64_t capacity) {
    uint64_t hash_value = FNV_OFFSET_BASIS;

    for (const char *c = key; *c != '\0'; c++) {
        hash_value ^= *c;
        hash_value *= FNV_PRIME;
    }

    return hash_value & (capacity - 1);
}

void _set_item(Item *item, const char *key, int value) {
    if (!is_used(item->flags)) {
        item->flags &= ~TOMBSTONE_FLAG;
        item->flags |= USED_FLAG;
        size_t key_len = strlen(key) + 1;
        item->key = malloc(key_len);
#ifdef HASHMAP_BENCH
        if (item->key) {
            hashmap_bench_mallocs++;
            hashmap_bench_alloc_bytes += key_len;
        }
#endif
        strcpy(item->key, key);
    }

    item->value = value;
}

HashMap _alloc_hashmap(size_t capacity) {
    Item *array = calloc(capacity, sizeof(Item));
#ifdef HASHMAP_BENCH
    if (array) {
        hashmap_bench_mallocs++;
        hashmap_bench_alloc_bytes += capacity * sizeof(Item);
    }
#endif
    return (HashMap){.capacity = capacity, .array = array, .len = 0};
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
    arena->capacity *= 2;
    char *new_memory = realloc(arena->memory, arena->capacity);
    if (!new_memory) {
        return false;
    }
    arena->memory = new_memory;

    return true;
}

size_t _arena_alloc_string(Arena *arena, const char *str, size_t len) {
    if (arena->used + len + sizeof(size_t) > arena->capacity) {
        if (!_arena_resize(arena)) {
            abort();
        }
    }
    size_t old_used = arena->used;

    *(size_t *)(arena->memory + old_used) = len;
    memcpy(arena->memory + old_used + sizeof(size_t), str, len);

    arena->used += sizeof(len) + len;

    return old_used;
}

static inline String _arena_get_string(Arena *arena, size_t offset) {
    return (String){.len = *(size_t *)(arena->memory + offset),
                    .string = arena->memory + offset + sizeof(size_t)};
}

bool _arena_string_equals(Arena *arena, size_t offset, const char *key) {
    String str = _arena_get_string(arena, offset);
    size_t key_len = strlen(key);

    if (str.len != key_len)
        return false;

    return memcmp(str.string, key, key_len) == 0;
}

bool _string_equals(String *string, const char *key) {
    size_t key_len = strlen(key);

    if (string->len != key_len) {
        return false;
    }

    return memcmp(string->string, key, key_len) == 0;
}
