#include <cstdio>
#include <stdio.h>
#include <vector>

void foo() {
    std::vector<int> x{1, 2, 3, 4, 5};
    printf("Inside foo\n");
}

void bar() {
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
