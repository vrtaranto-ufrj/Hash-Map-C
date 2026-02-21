#include <stdio.h>

#include "hashmap.h"

int main(int argc, char const *argv[]) {
    printf("Olá, mundo!\n");

    HashMap hashmap = create_hashmap(1);
    add_hashmap(&hashmap, 10, 1);
    add_hashmap(&hashmap, 133, 19);

    HashMapReturn a = get_hashmap(&hashmap, 10);

    return 0;
}
