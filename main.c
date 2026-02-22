#include <stdio.h>

#include "hashmap.h"

int main(int argc, char const *argv[]) {
    HashMap hashmap = create_hashmap(8);
    add_hashmap(&hashmap, "1", 1);
    add_hashmap(&hashmap, "2", 2);
    add_hashmap(&hashmap, "5", 5);
    add_hashmap(&hashmap, "6", 6);

    pop_hashmap(&hashmap, "1");


    size_t n = hash_func("10", 4);
    
    HashMapReturn a = get_hashmap(&hashmap, "5");

    free_hashmap(&hashmap);

    return 0;
}
