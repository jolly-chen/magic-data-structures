// SoA structure with an AoS interface using P2996 reflection and P3294 token
// injection. Each array in the SoA is allocated in a contiguous storage
// container.
// Run here: https://godbolt.org/z/P613hh8MG

#include <concepts>
#include <experimental/meta>
#include <iostream>
#include <span>
#include <type_traits>

using namespace std::literals::string_view_literals;

template <class T> static constexpr bool is_span_v = requires {
  requires std::same_as<std::decay_t<T>, std::span<typename std::decay_t<T>::value_type>>;
};

// https://stackoverflow.com/a/60491447
template <class ContainerType>
concept Container = is_span_v<ContainerType> || requires(ContainerType a, const ContainerType b) {
  requires std::regular<ContainerType>;
  requires std::swappable<ContainerType>;
  requires std::destructible<typename ContainerType::value_type>;
  requires std::same_as<typename ContainerType::reference, typename ContainerType::value_type &>;
  requires std::same_as<typename ContainerType::const_reference, const typename ContainerType::value_type &>;
  requires std::forward_iterator<typename ContainerType::iterator>;
  requires std::forward_iterator<typename ContainerType::const_iterator>;
  requires std::signed_integral<typename ContainerType::difference_type>;
  requires std::same_as<typename ContainerType::difference_type,
                        typename std::iterator_traits<typename ContainerType::iterator>::difference_type>;
  requires std::same_as<typename ContainerType::difference_type,
                        typename std::iterator_traits<typename ContainerType::const_iterator>::difference_type>;
  { a.begin() } -> std::same_as<typename ContainerType::iterator>;
  { a.end() } -> std::same_as<typename ContainerType::iterator>;
  { b.begin() } -> std::same_as<typename ContainerType::const_iterator>;
  { b.end() } -> std::same_as<typename ContainerType::const_iterator>;
  { a.cbegin() } -> std::same_as<typename ContainerType::const_iterator>;
  { a.cend() } -> std::same_as<typename ContainerType::const_iterator>;
  { a.size() } -> std::same_as<typename ContainerType::size_type>;
  { a.max_size() } -> std::same_as<typename ContainerType::size_type>;
  { a.empty() } -> std::same_as<bool>;
};

consteval auto type_is_container(std::meta::info r) -> bool {
  return extract<bool>(std::meta::substitute(^Container, {
                                                             r}));
}

///
// Print utilities
///

template <typename T> void print_container(T &v) {
  std::cout << "{";
  for (size_t i = 0; i < v.size(); i++) {
    if (i != 0)
      std::cout << ", ";

    if constexpr (Container<typename T::value_type>) {
      print_container(v[i]);
    } else {
      std::cout << v[i];
    }
  }
  std::cout << "}\n";
}

template <typename T> void print_container_addr(T &v) {
  std::cout << "{";
  for (size_t i = 0; i < v.size(); i++) {
    if (i != 0)
      std::cout << ", ";

    if constexpr (Container<typename T::value_type>) {
      print_container_addr(v[i]);
    } else {
      std::cout << (long long)&v[i];
    }
  }
  std::cout << "}\n";
}

///
// Magic Data Structure
///
namespace mds {
consteval auto get_scalar_type(std::meta::info t) -> std::meta::info {
  if (type_is_container(t)) {
    return get_scalar_type(template_arguments_of(t)[0]);
  }
  return t;
}

consteval auto gen_sov_members(std::meta::info t) -> void {
  for (auto member : nonstatic_data_members_of(t)) {
    auto vec_member = ^{
      \id("_"sv, name_of(member))
    };

    auto type = get_scalar_type(type_of(member));
    if (type_is_container(type_of(member))) {
      queue_injection(^{
        std::vector<sov_metadata> \id("_"sv, name_of(member), "_md"sv);
      });
    }

    queue_injection(^{
      std::span<typename[:\(type):]> \tokens(vec_member);
    });
  }
}

consteval auto gen_sor_members(std::meta::info t) -> void {
  for (auto member : nonstatic_data_members_of(t)) {
    if (type_is_container(type_of(member))) {
      queue_injection(^{
        const std::span<typename[:\(get_scalar_type(type_of(member))):]>  \id(name_of(member));
      });
    } else {
      queue_injection(^{
        const typename[:\(type_of(member)):] & \id(name_of(member));
      });
    }
  }
}

template <typename T, size_t Alignment> class vector {
private:
  std::vector<std::byte> storage;
  size_t _size; // Number of elements

public: // internal data public for debugging
  struct sov_metadata {
    size_t offset, size;
    friend std::ostream &operator<<(std::ostream &os, const sov_metadata &obj) {
      return os << "{" << obj.offset << ", " << obj.size << "}";
    }
  };
  consteval { gen_sov_members(^T); }

  std::vector<size_t> byte_sizes; // Size of each SoV including alignment padding

  struct aos_view {
    consteval { gen_sor_members(^T); }
  };

  // Helper function to compute aligned size
  constexpr inline size_t align_size(size_t size, size_t alignment) {
    return ((size + alignment - 1) / alignment) * alignment;
  }

  // Compute the number of bytes needed for each storage vector and the total number of storage bytes.
  template <std::meta::info Member>
  auto compute_sizes(const std::initializer_list<T> data, size_t &size, size_t &byte_size) -> void {
    if constexpr (type_is_container(type_of(Member))) {
      using vec_type = typename[:type_of(Member):] ::value_type;
      for (auto &elem : data) {
        auto n_elements = elem.[:Member:].size();
        consteval {
          queue_injection(^{
            \id("_"sv, name_of(Member), "_md"sv).push_back({.offset = size, .size = n_elements});
          });
        }
        byte_size += align_size(sizeof(vec_type[n_elements]), Alignment);
        size += n_elements;
      }
    } else {
      byte_size = align_size(sizeof(typename[:type_of(Member):][data.size()]), Alignment);
      size = data.size();
    }

    std::cout << "_" << name_of(Member) << " = " << size << " elements in " << byte_size << " bytes\n";
  }

public:
  vector(std::initializer_list<T> data) {
    auto n_members = [:std::meta::reflect_value(nonstatic_data_members_of(^T).size()):];
    _size = data.size();

    std::vector<size_t> sizes(n_members);
    byte_sizes.resize(n_members);
    size_t total_byte_size = 0;
    size_t m_idx = 0;
    [:expand(nonstatic_data_members_of(^T)):] >> [&]<auto e> {
      compute_sizes<e>(data, sizes[m_idx], byte_sizes[m_idx]);
      total_byte_size += byte_sizes[m_idx++];
    };

    storage.resize(total_byte_size);
    std::cout << "storage of " << total_byte_size << " bytes in total\n\n";

    // Loop over storage vectors
    size_t offset = 0;
    m_idx = 0;
    [:expand(nonstatic_data_members_of(^T)):] >> [&]<auto e> {
      // reading sizes directly in queue injection doesn't seem to work?
      // results in a "cannot capture sizes" error
      auto sov_size = sizes[m_idx];

      consteval {
        auto name = name_of(e);
        auto type = get_scalar_type(type_of(e));

        // Assign required bytes to storage vector e.g.,
        //    _x = std::span(reinterpret_cast<double*>(storage.data()) + offset,
        //                   sizes[m_idx]);
        queue_injection(^{
          \id("_"sv, name) = std::span(reinterpret_cast<[: \(type):] *>(storage.data() + offset), sov_size);
        });
      }
      offset += byte_sizes[m_idx++];

      // Fill storage spans
      size_t e_idx = 0;
      for (auto &elem : data) {
        if constexpr (type_is_container(type_of(e))) {
          using vec_type = typename[:type_of(e):] ::value_type;

          for (size_t i = 0; i < elem.[:e:].size(); i++) {
            consteval {
              // e.g, new (&_a[e_idx]) double(elem.x);
              queue_injection(^{
                new (&\id("_"sv, name_of(e))[e_idx]) vec_type(elem.[:e:][i]);
              });
            }
            e_idx++;
          }
        } else {
          consteval {
            // e.g, new (&_x[e_idx]) double(elem.x);
            queue_injection(^{
              new (&\id("_"sv, name_of(e))[e_idx]) decltype(elem.[:e:])(elem.[:e:]);
            });
          }
          e_idx++;
        }
      }
    };
  }

  auto size() const -> std::size_t { return _size; }

  auto operator[](std::size_t m_idx) const -> aos_view {
    consteval {
      // gather references to sov elements
      std::meta::list_builder member_data_tokens{};
      for (auto member : nonstatic_data_members_of(^T)) {
        auto name = name_of(member);
        auto sov_name = ^{
          \id("_"sv, name)
        };

        if (type_is_container(type_of(member))) {
          auto md_name = ^{
            \id("_"sv, name, "_md"sv)
          };
          member_data_tokens += ^{
            .\id(name) = \tokens(sov_name).subspan(\tokens(md_name)[m_idx].offset, \tokens(md_name)[m_idx].size)
          };
        } else {
          member_data_tokens += ^{
            .\id(name) = \tokens(sov_name)[m_idx]
          };
        }
      }

      // Injects:
      //     return aos_view(.x = _x[idx], .v = _v.subspan(_v_md[idx].offset, _v_md[idx].size));
      queue_injection(^{
        return aos_view{\tokens(member_data_tokens)};
      });
    }
  }
};
} // namespace mds

// dummy
struct data {
  double x;
  std::vector<int> v;
  // std::vector<std::vector<double>> p;
};

int main() {
  data e1 = {0, {100, 101, 102, 103}};
  data e2 = {4, {200}};
  data e3 = {8, {300, 301}};

  mds::vector<data, 64> maos = {e1, e2, e3};

  std::cout << "maos.size = " << maos.size() << "\n";
  for (size_t i = 0; i != maos.size(); ++i) {
    std::cout << "maos[" << i << "] = (";

    [:expand(nonstatic_data_members_of(^decltype(maos[i]))):] >> [&]<auto e> {
      std::cout << name_of(e) << ": ";
      if constexpr (type_is_container(type_of(e))) {
        print_container(maos[i].[:e:]);
      } else {
        std::cout << maos[i].[:e:] << ", ";
      }
    };

    std::cout << ")\taddr = ";

    [:expand(nonstatic_data_members_of(^decltype(maos[i]))):] >> [&]<auto e> {
      std::cout << name_of(e) << ": ";
      if constexpr (type_is_container(type_of(e))) {
        print_container_addr(maos[i].[:e:]);
      } else {
        std::cout << (long long)&maos[i].[:e:] << ", ";
      }
    };
  }

  std::cout << "\n";

  //// print underlying data ////

  // Edison Design Group C/C++ Front End, version 6.6 (Jul 29 2024 17:25:25)
  // - accessible_members_of is undefined
  // - nonstatic_data_members_of doesn't accept filters
  [:expand(members_of(^mds::vector<data, 64>, std::meta::is_nonstatic_data_member, std::meta::is_accessible)
           ):] >> [&]<auto e> {
    std::cout << name_of(e) << " = ";
    print_container(maos.[:e:]);
    std::cout << "\taddr = ";
    print_container_addr(maos.[:e:]);
  };

  return 0;
}
