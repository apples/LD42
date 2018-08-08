#ifndef ARRAY2LD42_D_HPP
#define ARRAY2LD42_D_HPP

#include <vector>
#include <utility>

template <typename T>
class array2d {
public:
    array2d() = default;

    array2d(int rows, int cols) :
        n_rows(rows),
        n_cols(cols),
        data(rows*cols)
    {}

    array2d(const array2d& other) :
        n_rows(other.n_rows),
        n_cols(other.n_cols),
        data(other.data)
    {}

    array2d(array2d&& other) :
        n_rows(std::exchange(other.n_rows, 0)),
        n_cols(std::exchange(other.n_cols, 0)),
        data(std::move(other.data))
    {}

    array2d& operator=(const array2d& other) {
        n_rows = other.n_rows;
        n_cols = other.n_cols;
        data = other.data;
        return *this;
    }

    array2d& operator=(array2d&& other) {
        n_rows = std::exchange(other.n_rows, 0);
        n_cols = std::exchange(other.n_cols, 0);
        data = std::move(other.data);
        return *this;
    }

    int rows() const { return n_rows; }

    int cols() const { return n_cols; }

    T& at(int row, int col) { const auto& self = *this; return const_cast<T&>(self.at(row, col)); }

    const T& at(int row, int col) const { return data[row*cols() + col]; }

    void fill(const T& val) { std::fill(data.begin(), data.end(), val); }

private:
    int n_rows = 0;
    int n_cols = 0;
    std::vector<T> data;
};

template <typename T>
void from_json(const nlohmann::json& j, array2d<T>& t) {
    t = array2d<T>(j["rows"], j["cols"]);
    for (auto r = 0; r < t.rows(); ++r) {
        for (auto c = 0; c < t.cols(); ++c) {
            t.at(r, c) = j["data"][r][c];
        }
    }
}

template <typename T>
void to_json(nlohmann::json& j, const array2d<T>& t) {
    j["rows"] = t.rows();
    j["cols"] = t.cols();
    for (auto r = 0; r < t.rows(); ++r) {
        for (auto c = 0; c < t.cols(); ++c) {
            j["data"][r][c] = t.at(r, c);
        }
    }
}

#endif //ARRAY2LD42_D_HPP
