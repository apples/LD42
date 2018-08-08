//
// Created by Jeramy on 7/22/2015.
//

#ifndef SUSHI_COMMON_HPP
#define SUSHI_COMMON_HPP

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <utility>

#define SUSHI_CAT_IMPL(A,B) A##B
#define SUSHI_CAT(A,B) SUSHI_CAT_IMPL(A,B)
#define SUSHI_DEFER auto SUSHI_CAT(_defer_var_, __LINE__) = ::sushi::_defer{} + [&]()

/// Sushi
namespace sushi {

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

/// Allows a value type to be used as a Nullable.
template<typename T>
struct fake_nullable {
    T value;

    fake_nullable() : value() { }

    fake_nullable(std::nullptr_t) : fake_nullable() { }

    fake_nullable(T v) : value(std::move(v)) { }

    operator T() const { return value; }

    bool operator==(const fake_nullable& other) const { return value == other.value; }

    bool operator!=(const fake_nullable& other) const { return value != other.value; }
};

/// A unique handle to an OpenGL resource.
template<typename Deleter>
using unique_gl_resource = std::unique_ptr<typename Deleter::pointer, Deleter>;

/// Loads an entire file into memory.
/// The file is loaded line-by-line into a vector.
/// \param fname File name.
/// \return All lines in the file, or nothing if the file cannot be opened.
inline std::vector<std::string> load_file(const std::string& fname) {
    std::ifstream file(fname);
    std::string line;
    std::vector<std::string> rv;
    while (std::getline(file, line)) {
        line += "\n";
        rv.push_back(line);
    }
    return rv;
}

/// A simple value wrapper for storing a const reference.
template<typename T>
class const_reference_wrapper {
    const T* ptr = nullptr;
public:
    const_reference_wrapper() = default;

    const_reference_wrapper(const T& t) : ptr(&t) { }

    const T& get() const { return *ptr; }
};

}

#endif //SUSHI_COMMON_HPP
