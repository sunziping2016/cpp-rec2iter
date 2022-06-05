#ifndef COROUTINE_H_INCLUDE
#define COROUTINE_H_INCLUDE

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
struct coroutine_exit {
};
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

#define coroutine_funcs(func) \
struct coroutine : future<coroutine, typeof(func)> { \
    using Future = future<coroutine, typeof(func)>; \
    explicit coroutine(const Future &rhs): Future(rhs) {} \
    explicit coroutine(Future &&rhs): Future(std::move(rhs)) {} \
}; \
template<typename T> \
coroutine make_coroutine(T &&v) { \
    return coroutine(std::forward<T>(v)); \
}

#endif