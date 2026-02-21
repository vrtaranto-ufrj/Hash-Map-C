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
    HashMapReturn return_value = {0};

    ListNode *node = hashmap->array[index];

    if (!node) {
        return return_value;
    }

    if (node->key == key) {
        hashmap->array[index] = node->next;
        return_value.found = true;
        return_value.value = node->value;
        free(node);
        
        return return_value;
    }

    ListNode *lastnode = node;
    node = node->next;
    
    for (; node; node = node->next) {
        if (node->key == key) {
            lastnode->next = node->next;
            return_value.found = true;
            return_value.value = node->value;
            free(node);

            break;
        }
        lastnode = node;
    }

    return return_value;
}