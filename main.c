#include <stdio.h>

#include "hashmap.h"

int main(int argc, char const *argv[]) {
    HashMap hashmap = create_hashmap(4);
    add_hashmap(&hashmap, "1", 1);
    add_hashmap(&hashmap, "2", 2);

    pop_hashmap(&hashmap, "1");

    add_hashmap(&hashmap, "5", 5);

    size_t n = hash_func("10", 4);
    
    HashMapReturn a = get_hashmap(&hashmap, "133");

    free_hashmap(&hashmap);

    return 0;
}
