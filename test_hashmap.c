#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

static int tests_failed = 0;
static int tests_run = 0;

#define EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            tests_failed++; \
        } \
    } while (0)

#define EXPECT_EQ_INT(actual, expected) \
    do { \
        int _a = (actual); \
        int _e = (expected); \
        if (_a != _e) { \
            printf("FAIL %s:%d: %s == %s (got %d, expected %d)\n", \
                   __FILE__, __LINE__, #actual, #expected, _a, _e); \
            tests_failed++; \
        } \
    } while (0)

#define EXPECT_EQ_SIZE(actual, expected) \
    do { \
        size_t _a = (actual); \
        size_t _e = (expected); \
        if (_a != _e) { \
            printf("FAIL %s:%d: %s == %s (got %zu, expected %zu)\n", \
                   __FILE__, __LINE__, #actual, #expected, _a, _e); \
            tests_failed++; \
        } \
    } while (0)

static char *make_key(const char *prefix, int n) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%d", prefix, n);
    char *out = malloc(strlen(buf) + 1);
    if (!out) {
        return NULL;
    }
    strcpy(out, buf);
    return out;
}

static int find_colliding_keys(size_t capacity, int count, char **out_keys, size_t *out_index) {
    size_t target_index = (size_t)-1;
    int found = 0;

    for (int i = 0; i < 100000 && found < count; i++) {
        char *key = make_key("k", i);
        if (!key) {
            return 0;
        }
        size_t idx = _hash_func(key, capacity);
        if (target_index == (size_t)-1) {
            target_index = idx;
            out_keys[found++] = key;
        } else if (idx == target_index) {
            out_keys[found++] = key;
        } else {
            free(key);
        }
    }

    if (found == count) {
        if (out_index) {
            *out_index = target_index;
        }
        return 1;
    }

    for (int i = 0; i < found; i++) {
        free(out_keys[i]);
    }
    return 0;
}

static void free_keys(char **keys, int count) {
    for (int i = 0; i < count; i++) {
        free(keys[i]);
    }
}

static void test_add_basic_insert_and_get(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    add_hashmap(&map, "alpha", 10);
    EXPECT_EQ_SIZE(map.len, 1);

    HashMapReturn ret = get_hashmap(&map, "alpha");
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 10);

    free_hashmap(&map);
}

static void test_add_update_existing_key(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    add_hashmap(&map, "alpha", 10);
    add_hashmap(&map, "alpha", 20);

    EXPECT_EQ_SIZE(map.len, 1);
    HashMapReturn ret = get_hashmap(&map, "alpha");
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 20);

    free_hashmap(&map);
}

static void test_add_used_diff_continue(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[2];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 2, keys, &index));

    add_hashmap(&map, keys[0], 1);
    add_hashmap(&map, keys[1], 2);

    EXPECT_EQ_SIZE(map.len, 2);
    HashMapReturn r1 = get_hashmap(&map, keys[0]);
    HashMapReturn r2 = get_hashmap(&map, keys[1]);
    EXPECT_TRUE(r1.found);
    EXPECT_TRUE(r2.found);

    free_keys(keys, 2);
    free_hashmap(&map);
}

static void test_add_tombstone_reuse(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[3];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 3, keys, &index));

    add_hashmap(&map, keys[0], 1);
    add_hashmap(&map, keys[1], 2);
    pop_hashmap(&map, keys[0]);

    add_hashmap(&map, keys[2], 3);

    EXPECT_EQ_SIZE(map.len, 2);
    EXPECT_TRUE(is_used(map.array[index].flags));
    EXPECT_TRUE(strcmp(map.array[index].key, keys[2]) == 0);

    free_keys(keys, 3);
    free_hashmap(&map);
}

static void test_add_multiple_tombstones_keep_first(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[4];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 4, keys, &index));

    add_hashmap(&map, keys[0], 1);
    add_hashmap(&map, keys[1], 2);
    add_hashmap(&map, keys[2], 3);

    pop_hashmap(&map, keys[0]);
    pop_hashmap(&map, keys[1]);

    add_hashmap(&map, keys[3], 4);

    EXPECT_TRUE(is_used(map.array[index].flags));
    EXPECT_TRUE(strcmp(map.array[index].key, keys[3]) == 0);
    EXPECT_TRUE(is_tombstone(map.array[(index + 1) % map.capacity].flags));

    free_keys(keys, 4);
    free_hashmap(&map);
}

static void test_add_only_tombstones_no_empty(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    for (size_t i = 0; i < map.capacity; i++) {
        map.array[i].flags = TOMBSTONE_FLAG;
    }

    add_hashmap(&map, "ghost", 9);

    HashMapReturn ret = get_hashmap(&map, "ghost");
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 9);
    EXPECT_EQ_SIZE(map.len, 1);

    free_hashmap(&map);
}

static void test_get_tombstone_continue_and_found(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[2];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 2, keys, &index));

    add_hashmap(&map, keys[0], 1);
    add_hashmap(&map, keys[1], 2);
    pop_hashmap(&map, keys[0]);

    HashMapReturn ret = get_hashmap(&map, keys[1]);
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 2);

    free_keys(keys, 2);
    free_hashmap(&map);
}

static void test_get_unused_break_not_found(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    HashMapReturn ret = get_hashmap(&map, "missing");
    EXPECT_TRUE(!ret.found);

    free_hashmap(&map);
}

static void test_get_used_diff_continue(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[2];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 2, keys, &index));

    add_hashmap(&map, keys[0], 11);
    add_hashmap(&map, keys[1], 22);

    HashMapReturn ret = get_hashmap(&map, keys[1]);
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 22);

    free_keys(keys, 2);
    free_hashmap(&map);
}

static void test_pop_used_equal(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    add_hashmap(&map, "alpha", 1);
    HashMapReturn ret = pop_hashmap(&map, "alpha");
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 1);
    EXPECT_EQ_SIZE(map.len, 0);

    HashMapReturn ret2 = get_hashmap(&map, "alpha");
    EXPECT_TRUE(!ret2.found);

    free_hashmap(&map);
}

static void test_pop_used_diff_continue(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[2];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 2, keys, &index));

    add_hashmap(&map, keys[0], 1);
    add_hashmap(&map, keys[1], 2);

    HashMapReturn ret = pop_hashmap(&map, keys[1]);
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 2);

    HashMapReturn check = get_hashmap(&map, keys[0]);
    EXPECT_TRUE(check.found);
    EXPECT_EQ_INT(check.value, 1);

    free_keys(keys, 2);
    free_hashmap(&map);
}

static void test_pop_tombstone_continue(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    char *keys[2];
    size_t index = 0;
    EXPECT_TRUE(find_colliding_keys(map.capacity, 2, keys, &index));

    add_hashmap(&map, keys[0], 1);
    add_hashmap(&map, keys[1], 2);
    pop_hashmap(&map, keys[0]);

    HashMapReturn ret = pop_hashmap(&map, keys[1]);
    EXPECT_TRUE(ret.found);
    EXPECT_EQ_INT(ret.value, 2);

    free_keys(keys, 2);
    free_hashmap(&map);
}

static void test_pop_unused_break(void) {
    tests_run++;
    HashMap map = _alloc_hashmap(16);

    HashMapReturn ret = pop_hashmap(&map, "missing");
    EXPECT_TRUE(!ret.found);
    EXPECT_EQ_SIZE(map.len, 0);

    free_hashmap(&map);
}

int main(void) {
    test_add_basic_insert_and_get();
    test_add_update_existing_key();
    test_add_used_diff_continue();
    test_add_tombstone_reuse();
    test_add_multiple_tombstones_keep_first();
    test_add_only_tombstones_no_empty();
    test_get_tombstone_continue_and_found();
    test_get_unused_break_not_found();
    test_get_used_diff_continue();
    test_pop_used_equal();
    test_pop_used_diff_continue();
    test_pop_tombstone_continue();
    test_pop_unused_break();

    if (tests_failed == 0) {
        printf("All tests passed (%d).\n", tests_run);
        return 0;
    }

    printf("Tests failed: %d of %d\n", tests_failed, tests_run);
    return 1;
}
