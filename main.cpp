#include <cstdio>
#include <variant>
#include <tuple>
#include <functional>
#include <stack>
#include <cassert>

#include "coroutine.h"

void move(char get, int n, char put) {
    static int k = 1;
    std::printf("%2d:%3d # %c---%c\n", k, n, get, put);
    if (k++ % 3 == 0)
        std::printf("\n");
}

// 原版汉诺塔
//int hanoi(int n, char x, char y, char z) {
//    if (n == 1)
//        move(x, 1, z);
//    else {
//        hanoi(n - 1, x, z, y);
//        move(x, n, z);
//        hanoi(n - 1, y, x, z);
//    }
//    return 0;
//}

struct coroutine;
coroutine hanoi(int state, int n, char x, char y, char z);
coroutine_funcs(hanoi);

// 协程版汉诺塔
// TODO: 返回值、局部变量、间接调用、函数对象、参数修改
coroutine hanoi(int state, int n, char x, char y, char z) {
    coroutine_begin();
    if (n == 1)
        move(x, 1, z);
    else {
        coroutine_call(hanoi, n - 1, x, z, y);
        move(x, n, z);
        coroutine_call(hanoi, n - 1, y, x, z);
    }
    coroutine_end();
}

int main() {
    coroutine_run(hanoi, 3, 'A', 'B', 'C');
}

//
//int main() {
//    int n, counter;
//    printf("Input the number of diskes：");
//    scanf("%d", &n);
//    printf("\n");
//    counter = hanoi(n, 'A', 'B', 'C');
//    return 0;
//}