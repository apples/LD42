#ifndef LD42_RESOURCE_CACHE_HPP
#define LD42_RESOURCE_CACHE_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <map>

template <typename T, typename K = std::string>
class resource_cache {
public:
    using factory_function = std::function<std::shared_ptr<T>(const K&)>;

    template <typename F>
    using require_is_not_factory = std::enable_if_t<!std::is_convertible<std::decay_t<F>, factory_function>::value>*;

    resource_cache() = default;
    resource_cache(const resource_cache&) = delete;
    resource_cache(resource_cache&) = delete;
    resource_cache(resource_cache&&) = default;
    resource_cache& operator=(const resource_cache&) = delete;
    resource_cache& operator=(resource_cache&&) = default;

    resource_cache(factory_function f) : factory(std::move(f)) {}

    template <typename F>
    resource_cache(F&& f, require_is_not_factory<F> = {}) : factory(make_factory(std::forward<F>(f))) {}

    std::shared_ptr<T> get(const K& k) {
        auto& ptr = cache[k];
        if (!ptr) {
            ptr = factory(k);
        }
        return ptr;
    }

    void clear() {
        cache.clear();
    }

    std::shared_ptr<T> reload(const K& k) {
        auto& ptr = cache[k];
        ptr = factory(k);
        return ptr;
    }

private:
    template <typename F>
    static factory_function make_factory(F&& f) {
        return [f=std::forward<F>(f)](const K& k) {
            return std::make_shared<T>(f(k));
        };
    }

    factory_function factory;
    std::map<K, std::shared_ptr<T>> cache;
};

#endif //LD42_RESOURCE_CACHE_HPP

