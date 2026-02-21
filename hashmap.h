#pragma once

#include <stdlib.h>
#include <stdbool.h>

#define M 5013

typedef struct HashMapStruct HashMap;
typedef struct ListNodeStruct ListNode;
typedef struct HashMapReturnStruct HashMapReturn;

struct ListNodeStruct {
    int key;
    int value;
    ListNode *next;
};

struct HashMapStruct {
    size_t len;
    ListNode **array;
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

ListNode *alloc_list_node(int key, int value) {
    ListNode *node = malloc(sizeof(ListNode));    
    node->key = key;
    node->value = value;
    node->next = NULL;

    return node;
}

size_t hash_func(int key, size_t len) {
    return key % M % len;
}

void add_hashmap(HashMap *hashmap, int key, int value) {
    size_t index = hash_func(key, hashmap->len);

    for (ListNode *node = hashmap->array[index]; node; node = node->next) {
        if (node->key == key) {
            node->value = value;
            return;
        }
    }
    
    ListNode *newnode = alloc_list_node(key, value);
    newnode->next = hashmap->array[index];
    hashmap->array[index] = newnode;
}

HashMapReturn get_hashmap(HashMap *hashmap, int key) {
    size_t index = hash_func(key, hashmap->len);

    for (ListNode *node = hashmap->array[index]; node; node = node->next) {
        if (node->key == key) {
            return (HashMapReturn){
                .found = true,
                .value = node->value
            };
        }
    }

    return (HashMapReturn){
        .found = false
    };
}

HashMapReturn pop_hashmap(HashMap *hashmap, int key) {
    size_t index = hash_func(key, hashmap->len);

    for (ListNode **pp = &hashmap->array[index]; *pp; pp = &(*pp)->next) {
        ListNode *node = *pp;
        if (node->key == key) {
            *pp = node->next;
            free(node);

            return (HashMapReturn){
                .found = true,
                .value = node->value
            };
        }
    }

    return (HashMapReturn){
        .found = false
    };
}