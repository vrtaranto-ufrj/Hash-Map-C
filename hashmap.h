#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define FNV_PRIME        0x00000100000001b3UL
#define FNV_OFFSET_BASIS 0xcbf29ce484222325UL

typedef struct HashMapStruct HashMap;
typedef struct ListNodeStruct ListNode;
typedef struct HashMapReturnStruct HashMapReturn;

struct ListNodeStruct {
    char* key;
    int value;
    ListNode* next;
};

struct HashMapStruct {
    size_t len;
    ListNode** array;
};

struct HashMapReturnStruct {
    int value;
    bool found;
};

HashMap create_hashmap(size_t len) {
    return (HashMap) {
        .len = len,
            .array = calloc(len, sizeof(ListNode*))
    };
}

ListNode* alloc_list_node(const char* key, int value) {
    ListNode* node = malloc(sizeof(ListNode));
    size_t key_len = strlen(key);

    node->key = malloc(sizeof(char) * key_len + 1);
    strcpy(node->key, key);
    node->value = value;
    node->next = NULL;

    return node;
}

uint64_t hash_func(const char* key, uint64_t len) {
    uint64_t hash_value = FNV_OFFSET_BASIS;

    for (uint8_t c = 0; c < strlen(key); c++) {
        hash_value ^= key[c];
        hash_value *= FNV_PRIME;
    }

    return hash_value & (len - 1);
}

void add_hashmap(HashMap* hashmap, const char* key, int value) {
    size_t index = hash_func(key, hashmap->len);

    for (ListNode* node = hashmap->array[index]; node; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            return;
        }
    }

    ListNode* newnode = alloc_list_node(key, value);
    newnode->next = hashmap->array[index];
    hashmap->array[index] = newnode;
}

HashMapReturn get_hashmap(HashMap* hashmap, const char* key) {
    size_t index = hash_func(key, hashmap->len);

    for (ListNode* node = hashmap->array[index]; node; node = node->next) {
        if (strcmp(node->key, key) == 0) {
            return (HashMapReturn) {
                .found = true,
                    .value = node->value
            };
        }
    }

    return (HashMapReturn) {
        .found = false
    };
}

HashMapReturn pop_hashmap(HashMap* hashmap, const char* key) {
    size_t index = hash_func(key, hashmap->len);

    for (ListNode** pp = &hashmap->array[index]; *pp; pp = &(*pp)->next) {
        ListNode* node = *pp;
        if (strcmp(node->key, key) == 0) {
            *pp = node->next;
            free(node);

            return (HashMapReturn) {
                .found = true,
                    .value = node->value
            };
        }
    }

    return (HashMapReturn) {
        .found = false
    };
}
