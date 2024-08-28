#include <iostream>
#include <span>
#include <vector>

using namespace std::literals::string_view_literals;

template <class T> static constexpr bool is_vector_v = requires {
  requires std::same_as<std::decay_t<T>, std::vector<typename std::decay_t<T>::value_type>>;
};

template <typename T> void print_vector(T &v) {
  std::cout << "{";
  for (size_t i = 0; i < v.size(); i++) {
    if (i != 0)
      std::cout << ", ";

    if constexpr (is_vector_v<typename T::value_type>) {
      print_vector(v[i]);
    } else {
      std::cout << v[i];
    }
  }
  std::cout << "}";
}

template <typename T> void print_vector_addr(T &v) {
  std::cout << "{";
  for (size_t i = 0; i < v.size(); i++) {
    if (i != 0)
      std::cout << ", ";

    if constexpr (is_vector_v<typename T::value_type>) {
      print_vector(v[i]);
    } else {
      std::cout << (long long)&v[i];
    }
  }
  std::cout << "}";
}

namespace mds {

template <typename T, size_t Alignment> class vector {
private:
  std::vector<std::byte> storage;

  struct sov_metadata {
    size_t offset, size;
    friend std::ostream &operator<<(std::ostream &os, const sov_metadata &obj) {
      return os << "{" << obj.offset << ", " << obj.size << "}";
    }
  };

  size_t _size; // Number of elements

  // SoV
  std::span<double> _x;
  std::span<int> _v;

  std::vector<size_t> byte_sizes; // Size of each SoV including alignment padding
  std::vector<sov_metadata> _v_md;

  struct aos_view {
    const double &x;
    const std::span<int> v;
  };

  // Helper function to compute aligned size
  constexpr inline size_t align_size(size_t size, size_t alignment) {
    return ((size + alignment - 1) / alignment) * alignment;
  }

public:
  vector(std::initializer_list<T> data) {
    constexpr size_t n_members = 4;
    _size = data.size();

    byte_sizes.resize(n_members);
    std::vector<size_t> sizes(n_members);
    size_t total_byte_size = 0;

    // Compute SoV sizes
    size_t m_idx = 0;
    byte_sizes[m_idx] = align_size(sizeof(double[_size]), Alignment); // _x
    sizes[m_idx] = _size;
    total_byte_size += byte_sizes[m_idx++];

    for (auto &elem : data) { // _v
      auto n_elements = elem.v.size();
      _v_md.push_back({.offset = sizes[m_idx], .size = n_elements});
      byte_sizes[m_idx] += align_size(sizeof(int[n_elements]), Alignment);
      sizes[m_idx] += n_elements;
    }
    total_byte_size += byte_sizes[m_idx++];

    storage.resize(total_byte_size);
    std::cout << "storage of " << total_byte_size << " bytes in total\n\n";

    // Loop over storage vectors
    size_t offset = 0;
    m_idx = 0;

    _x = std::span(reinterpret_cast<double *>(storage.data() + offset), sizes[m_idx]);
    offset += byte_sizes[m_idx++];
    size_t e_idx = 0;
    for (auto &elem : data) {
      new (&_x[e_idx]) double(elem.x);
      e_idx++;
    }

    _v = std::span(reinterpret_cast<int *>(storage.data() + offset), sizes[m_idx]);
    offset += byte_sizes[m_idx++];
    e_idx = 0;
    for (auto &elem : data) {
      for (size_t i = 0; i < elem.v.size(); i++) {
        new (&_v[e_idx]) int(elem.v[i]);
        e_idx++;
      }
    }
  }

  auto size() const -> std::size_t { return _size; }

  auto operator[](std::size_t idx) const -> aos_view {
    return aos_view(_x[idx], _v.subspan(_v_md[idx].offset, _v_md[idx].size));
  }
};
} // namespace mds

struct data {
  double x;
  std::vector<int> v;
  // std::vector<std::vector<float>> m; // TODO
};

int main() {
  data e1 = {0, {100, 101, 102, 103}};
  data e2 = {4, {200}};
  data e3 = {8, {300, 301}};

  mds::vector<data, 64> maos = {e1, e2, e3};

  std::cout << "maos.size = " << maos.size() << "\n";
  for (size_t i = 0; i != maos.size(); ++i) {
    std::cout << "maos[" << i << "] = ( x:" << maos[i].x << ", a: {";
    print_vector(maos[i].v);
    std::cout << " )\n";
  }

  for (size_t i = 0; i != maos.size(); ++i) {
    std::cout << "maos[" << i << "] = ( x:" << (long long)&maos[i].x << ", a: {";
    print_vector_addr(maos[i].v);
    std::cout << " )\n";
  }
}