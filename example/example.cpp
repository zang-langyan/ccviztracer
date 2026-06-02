/* Copyright (C) 2026 zang-langyan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>. */
#include <cstdio>
#include <stdio.h>
#include <vector>

void foo() {
    std::vector<int> x{1, 2, 3, 4, 5};
    printf("Inside foo\n");
}

void bar() {
    int n = 100;
    do {} while (n-- > 0);
    printf("Inside bar\n");
}


int fibonacci(int n) {
    static std::vector<int> memo(n + 1, -1);
    if (memo[n] != -1) return memo[n];
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}


int main() {
    printf("Starting main\n");
    foo();
    bar();
    int n = 10;
    printf("fibonacci %d: %d\n", n, fibonacci(n));
    return 0;
}
