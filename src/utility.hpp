#ifndef LD42_UTILITY_HPP
#define LD42_UTILITY_HPP

#include <type_traits>
#include <utility>
#include <vector>

#define EMBER_CAT_IMPL(A,B) A##B
#define EMBER_CAT(A,B) EMBER_CAT_IMPL(A,B)
#define EMBER_DEFER auto EMBER_CAT(_defer_, __LINE__) = ::utility::_defer{} + [&]()->void

namespace utility {

template <typename... Ts>
struct type_list {};

struct _defer {
    template <typename F>
    struct _defer_impl {
        F func;
        bool active;
        _defer_impl(F f) : func(std::move(f)), active(true) {}
        _defer_impl(_defer_impl&& other) : func(std::move(other.func)), active(std::exchange(other.active,false)) {}
        ~_defer_impl() noexcept(false) { if (active) func(); }
    };
    template <typename F>
    _defer_impl<F> operator+(F f) { return {std::move(f)}; }
};

template <typename... Ts>
struct overload_impl;

template <typename T>
struct overload_impl<T> : T {
    using T::operator();
    overload_impl() = delete;
    overload_impl(overload_impl&) = default;
    overload_impl(overload_impl const&) = default;
    overload_impl(overload_impl&&) = default;
    overload_impl& operator=(overload_impl const&) = default;
    overload_impl& operator=(overload_impl&&) = default;
    template <typename U>
    overload_impl(U&& u) : T(std::forward<U>(u)) {}
};
template <typename T, typename... Ts>
struct overload_impl<T,Ts...> : T, overload_impl<Ts...> {
    using T::operator();
    using overload_impl<Ts...>::operator();
    overload_impl() = delete;
    overload_impl(overload_impl&) = default;
    overload_impl(overload_impl const&) = default;
    overload_impl(overload_impl&&) = default;
    overload_impl& operator=(overload_impl const&) = default;
    overload_impl& operator=(overload_impl&&) = default;
    template <typename U, typename... Us>
    overload_impl(U&& u, Us&&... us) : T(std::forward<U>(u)), overload_impl<Ts...>(std::forward<Us>(us)...) {}
};

template <typename... Ts>
overload_impl<std::decay_t<Ts>...> overload(Ts&&... ts) {
    return overload_impl<std::decay_t<Ts>...>(std::forward<Ts>(ts)...);
}

template <typename T>
class reversed_impl {
public:
    reversed_impl(T t) : c(std::forward<T>(t)) {}

    auto begin() { return std::rbegin(c); }
    auto end() { return std::rend(c); }
    auto rbegin() { return std::begin(c); }
    auto rend() { return std::end(c); }

    auto begin() const { return std::rbegin(c); }
    auto end() const { return std::rend(c); }
    auto rbegin() const { return std::begin(c); }
    auto rend() const { return std::end(c); }

private:
    T c;
};

template <typename T>
auto reversed(T&& t) {
    return reversed_impl<T>(std::forward<T>(t));
}

template <typename... Ts>
auto vectorify(Ts&&... ts) {
    std::vector<std::common_type_t<Ts...>> rv;
    rv.reserve(sizeof...(ts));
    (rv.push_back(std::forward<Ts>(ts)), ...);
    return rv;
}

} //namespace utility

#endif //LD42_UTILITY_HPP
