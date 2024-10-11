#include <iostream>

int main() {
    int i = 1;
    char *p = (char *)&i;

    if (*p == 1) {
        std::cout << "Little Endian" << std::endl;
    } else {
        std::cout << "Big Endian" << std::endl;
    }

    return 0;
}