#include <stdio.h>

#include "hashmap.h"

int main(void) {
    HashMap hashmap = create_hashmap();
    add_hashmap(&hashmap, "1", 1);
    add_hashmap(&hashmap, "2", 2);
    add_hashmap(&hashmap, "5", 5);
    add_hashmap(&hashmap, "6", 6);

    pop_hashmap(&hashmap, "1");

    // HashMapReturn a = get_hashmap(&hashmap, "5");

    clear_hashmap(&hashmap);

    free_hashmap(&hashmap);

    Arena a = _create_arena(100);
    size_t si = _arena_alloc_string(&a, "Ola", sizeof("Ola"));

    return 0;
}
