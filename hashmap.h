#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define FNV_PRIME          0x00000100000001b3UL
#define FNV_OFFSET_BASIS   0xcbf29ce484222325UL

#define RESIZE_THRSHOLD    70UL

#define TOMBSTONE_FLAG     0b00000001
#define USED_FLAG          0b00000010

#define is_tombstone(item_flag) (((item_flag) & TOMBSTONE_FLAG) != 0)
#define is_used(item_flag)      (((item_flag) & USED_FLAG) != 0)


typedef struct HashMapStruct HashMap;
typedef struct ItemStruct Item;
typedef struct HashMapReturnStruct HashMapReturn;

struct ItemStruct {
    char* key;
    int value;
    uint8_t flags;
};

struct HashMapStruct {
    Item* array;
    size_t len;
    size_t capacity;
};

struct HashMapReturnStruct {
    int value;
    bool found;
};

HashMap create_hashmap(size_t capacity);
void free_hashmap(HashMap* hashmap);

void add_hashmap(HashMap* hashmap, const char* key, int value);
HashMapReturn get_hashmap(HashMap* hashmap, const char* key);
HashMapReturn pop_hashmap(HashMap* hashmap, const char* key);

bool _resize_needed(HashMap* hashmap);
void _resize_hashmap(HashMap* hashmap);
uint64_t _hash_func(const char* key, uint64_t capacity);
void _set_item(Item* item, const char* key, int value);

HashMap create_hashmap(size_t capacity) {
    return (HashMap) {
        .capacity = capacity,
            .array = calloc(capacity, sizeof(Item)),
            .len = 0
    };
}

void free_hashmap(HashMap* hashmap) {
    for (size_t i = 0; i < hashmap->capacity; i++) {
        Item* item = &hashmap->array[i];

        if (is_used(item->flags)) {
            free(item->key);
        }

    }

    free(hashmap->array);
    hashmap->len = 0;
    hashmap->capacity = 0;
    hashmap->array = NULL;
}

bool _resize_needed(HashMap* hashmap) {
    return hashmap->len * 100UL >= RESIZE_THRSHOLD * hashmap->capacity;
}

void _resize_hashmap(HashMap* hashmap) {
    HashMap old_hashmap = *hashmap;

    *hashmap = create_hashmap(hashmap->capacity * 2);

    for (size_t i = 0; i < old_hashmap.capacity; i++) {
        Item* item = &old_hashmap.array[i];
        if (is_used(item->flags)) {
            add_hashmap(hashmap, item->key, item->value);
        }
    }

    free_hashmap(&old_hashmap);
}


uint64_t _hash_func(const char* key, uint64_t capacity) {
    uint64_t hash_value = FNV_OFFSET_BASIS;

    for (uint8_t c = 0; c < strlen(key); c++) {
        hash_value ^= key[c];
        hash_value *= FNV_PRIME;
    }

    return hash_value & (capacity - 1);
}

void add_hashmap(HashMap* hashmap, const char* key, int value) {
    if (_resize_needed(hashmap)) {
        _resize_hashmap(hashmap);
    }
    hashmap->len++;

    size_t index = _hash_func(key, hashmap->capacity);

    for (size_t i = 0; i < hashmap->capacity; i++) {
        Item* item = &hashmap->array[(i + index) % hashmap->capacity];

        if (is_tombstone(item->flags)) {
            continue;
        }

        if (!is_used(item->flags) || strcmp(item->key, key) == 0) {
            _set_item(item, key, value);
        }
    }
}

HashMapReturn get_hashmap(HashMap* hashmap, const char* key) {
    size_t index = _hash_func(key, hashmap->capacity);

    for (size_t i = 0; i < hashmap->capacity; i++) {
        Item* item = &hashmap->array[(i + index) % hashmap->capacity];

        if (is_tombstone(item->flags)) {
            continue;
        }

        if (!is_used(item->flags)) {
            break;
        }

        if (strcmp(item->key, key) == 0) {
            return (HashMapReturn) {
                .found = true,
                    .value = item->value
            };
        }
    }

    return (HashMapReturn) {
        .found = false
    };
}

HashMapReturn pop_hashmap(HashMap* hashmap, const char* key) {
    size_t index = _hash_func(key, hashmap->capacity);
    hashmap->len--;

    for (size_t i = 0; i < hashmap->len; i++) {
        Item* item = &hashmap->array[(i + index) % hashmap->capacity];

        if (is_tombstone(item->flags)) {
            break;
        }

        if (strcmp(item->key, key) == 0) {
            item->flags = TOMBSTONE_FLAG;

            HashMapReturn ret = {
                .found = true,
                    .value = item->value
            };
            free(item->key);

            return ret;
        }
    }

    return (HashMapReturn) {
        .found = false
    };
}

void _set_item(Item* item, const char* key, int value) {
    if (!is_used(item->flags)) {
        item->flags |= USED_FLAG;
        item->key = malloc(strlen(key) + 1);
        strcpy(item->key, key);
    }

    item->value = value;
}
