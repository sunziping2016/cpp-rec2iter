#include <cstdio>
#include <variant>
#include <tuple>
#include <functional>
#include <stack>
#include <cassert>

#define coroutine_begin() switch (state) { case 0:
#define coroutine_call(func, ...) { \
    state = __LINE__;         \
    return make_coroutine(std::make_tuple(__LINE__,\
        std::make_tuple(func, std::make_tuple(__VA_ARGS__)))); \
    case __LINE__:; }
#define coroutine_end() default:; } return make_coroutine(coroutine_exit());

template<typename Sig>
struct signature;
template<typename R, typename ...Args>
struct signature<R(int, Args...)> {
    using params = std::tuple<Args...>;
};
struct coroutine_exit {};
template<typename Coroutine, typename ...Fun>
using future = std::variant<
        coroutine_exit, // coroutine exit
        std::tuple< // coroutine sub-call
                int, // coroutine caller state
                std::variant<std::tuple<
                        Fun *, // callee
                        typename signature<Fun>::params // callee params
                >...>
        >
>;
template<typename Future>
using frame = std::variant_alternative_t<1, typename Future::Future>;

template<typename Future, std::size_t Index = std::variant_size_v<std::tuple_element_t<1, frame<Future>>>>
struct executor {
    using Frame = frame<Future>;

    static Future execute(Frame &frame) {
        if (std::get<1>(frame).index() == Index - 1) {
            auto &[func, params] = std::get<Index - 1>(std::get<1>(frame));
            auto state = std::get<0>(frame);
            return std::apply(func, std::tuple_cat(std::make_tuple(state), params));
        }
        return executor<Future, Index - 1>().execute(frame);
    }
};

template<typename Future>
struct executor<Future, 0> {
    using Frame = frame<Future>;

    static Future execute(Frame &frame) {
        assert(false);
    }
};

template<typename Future, typename ...Args>
void coroutine_run(Future (*func)(int, Args...), Args... args) {
    using Frame = frame<Future>;
    std::stack<Frame> stack;
    stack.push(std::make_tuple(0, std::make_tuple(func, std::make_tuple(args...))));
    while (!stack.empty()) {
        auto &top = stack.top();
        auto result = executor<Future>().execute(top);
        if (result.index() == 0) {
            stack.pop();
        } else {
            auto &[state, frame] = std::get<1>(result);
            std::get<0>(top) = state;
            stack.push(std::make_tuple(0, std::move(frame)));
        }
    }
}

struct coroutine : future<coroutine, coroutine(int, int, char, char, char)> {
    using Future = future<coroutine, coroutine(int, int, char, char, char)>;
    explicit coroutine(const Future &rhs): Future(rhs) {}
    explicit coroutine(Future &&rhs): Future(std::move(rhs)) {}
};
template<typename T>
coroutine make_coroutine(T &&v) {
    return coroutine(std::forward<T>(v));
}

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