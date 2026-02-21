#include <stdio.h>

#include "hashmap.h"

int main(int argc, char const *argv[]) {
    HashMap hashmap = create_hashmap(16);
    add_hashmap(&hashmap, "10", 1);
    add_hashmap(&hashmap, "133", 19);

    HashMapReturn a = get_hashmap(&hashmap, "11");

    uint64_t b = hash_func("Ola", 16);

    return 0;
}
