// SoA structure with an AoS interface using P2996 reflection and P3294 token injection.
// Each array in the SoA is allocated in a contiguous storage container.
// Run via https://godbolt.org/z/Gh4n4oz1c

#include <experimental/meta>
#include <iostream>
#include <span>

namespace mds {

// template <typename Test, template <typename...> class Ref>
// struct is_specialization : std::false_type {};

// template <template <typename...> class Ref, typename... Args>
// struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

using namespace std::literals;

consteval auto gen_sov_members(std::meta::info t) -> void {
    for (auto member : nonstatic_data_members_of(t)) {
        auto vec_member = ^{
          \id("_"sv, name_of(member))
        };

        queue_injection(^{
          std::span<typename[:\(type_of(member)):]> \tokens(vec_member);
        });
    }
}

consteval auto gen_sor_members(std::meta::info t) -> void {
    for (auto member : nonstatic_data_members_of(t)) {
        queue_injection(^{
          const typename[:\(type_of(member)):] & \id(name_of(member));
        });
    }
}

template <typename T, size_t Alignment>
class vector {
    // ------------ generate -----------
    //   private:
    //      std::vector<std::byte> storage;
    //      std::span<double> _x, _y, _z, _value;
    //      struct aos_view {
    //          double &x, &y, &z, &value;
    //      }
   private:
    std::vector<std::byte> storage;

    consteval { gen_sov_members(^T); }
    std::vector<size_t> sizes;  // Number of data elements per data vector

    struct aos_view {
        consteval { gen_sor_members(^T); }
    };

    // Helper function to compute aligned size
    constexpr inline size_t align_size(size_t size, size_t alignment) {
        return ((size + alignment - 1) / alignment) * alignment;
    }

   public:
    vector(std::initializer_list<T> data) {
        auto n_members = [:std::meta::reflect_value(nonstatic_data_members_of(^T).size()):];

        size_t total_size = 0;
        std::vector<size_t> byte_sizes(n_members);
        sizes.reserve(n_members);

        // Compute the number of bytes needed for each storage vector and the
        // total number of storage bytes.
        size_t m_idx = 0;
        [:expand(nonstatic_data_members_of(^T)):] >> [&]<auto e> {
            // TODO: (jagged) vector members
            // if constexpr (is_specialization<typename[:type_of(e):], std::vector>::value) {
            //     for (auto elem : data) {
            //         byte_sizes[m_idx] += align_size(
            //             elem.[:e:].size() * sizeof([:type_of(e):] ::value_type), Alignment);
            //         sizes[m_idx] += elem.[:e:].size();
            //     }
            // } else {
            byte_sizes[m_idx] = align_size(data.size() * sizeof(typename[:type_of(e):]), Alignment);
            sizes[m_idx] = data.size();
            // }

            std::cout << "_" << name_of(e) << " = " << sizes[m_idx] << " elements in "
                      << byte_sizes[m_idx] << " bytes\n";
            total_size += byte_sizes[m_idx];
            m_idx++;
        };

        storage.resize(total_size);
        std::cout << "storage of " << total_size << " bytes in total\n\n";

        // Loop over storage vectors
        size_t offset = 0;
        m_idx = 0;
        [:expand(nonstatic_data_members_of(^T)):] >> [&, this]<auto e> {
            consteval {
                auto name = name_of(e);
                auto type = type_of(e);

                // Assign required bytes to storage vector
                // e.g., _x = std::span(reinterpret_cast<double*>(storage.data()),
                //                      byte_sizes[m_idx] / sizeof(double));
                queue_injection(^{
                  \id("_"sv, name) =
                      std::span(reinterpret_cast<typename[: \(type):]*>(storage.data() + offset),
                                byte_sizes[m_idx] / sizeof(typename[: \(type):]));
                });
            }
            offset += byte_sizes[m_idx];

            // Fill storage vector
            size_t e_idx = 0;
            for (auto elem : data) {
                consteval {
                    // e.g, new (_x[e_idx]) double(elem.x);
                    queue_injection(^{
                      new (&\id("_"sv, name_of(e))[e_idx]) decltype(elem.\id(name_of(e)))(
                          elem.\id(name_of(e)));
                    });
                }
                e_idx++;
            }
        };
    }

    auto size() const -> std::size_t { return sizes[0]; }

    auto operator[](std::size_t m_idx) const -> aos_view {
        consteval {
            // gather references to sov elements
            std::meta::list_builder member_data_tokens{};
            for (auto member : nonstatic_data_members_of(^T)) {
                auto name = name_of(member);

                member_data_tokens += ^{
                  .\id(name) = \id("_"sv, name)[m_idx]
                };
            }

            // Injects: return aos_view(_x[m_idx], _y[m_idx], _z[m_idx], _value[m_idx]);
            queue_injection(^{
              return aos_view{\tokens(member_data_tokens)};
            });
        }
    }
};
}  // namespace mds

// dummy
struct data {
    double x, y, z, value;
};

int main() {
    data e1 = {0, 1, 2, 3};
    data e2 = {4, 5, 6, 7};
    data e3 = {8, 9, 10, 11};

    mds::vector<data, 64> maos = {e1, e2, e3};

    std::cout << "maos.size = " << maos.size() << "\n";
    for (size_t i = 0; i != maos.size(); ++i) {
        std::cout << "maos[" << i << "] = ({";

        consteval {
            for (auto member : nonstatic_data_members_of(^data)) {
                queue_injection(^{
                  std::cout << \(name_of(member)) << ": " << maos[i].\id(name_of(member)) << ", ";
                });
            }
        }

        std::cout << "})\n";
    }
    std::cout << "\n";

    for (size_t i = 0; i != maos.size(); ++i) {
        std::cout << "maos[" << i << "] = ({";

        consteval {
            for (auto member : nonstatic_data_members_of(^data)) {
                queue_injection(^{
                  std::cout << \(name_of(member)) << ": " << &maos[i].\id(name_of(member)) << ", ";
                });
            }
        }

        std::cout << "})\n";
    }

    return 0;
}
