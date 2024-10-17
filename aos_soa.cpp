#include <array>
#include <cstdlib>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <utility>

// Helper code

#define SOASTRUCT() template <template <typename> typename __SOA_T>
#define SOATYPE(SOA_TYPE) __SOA_T<SOA_TYPE>

// forward declarations
template <template <template <typename> typename> typename T> struct Type_Ref;
template <template <template <typename> typename> typename T> struct Type_ConstRef;
template <template <template <typename> typename> typename T> struct Type_Plain;
template <template <template <typename> typename> typename T, size_t N> struct Type_SOA;
template <template <template <typename> typename> typename T, size_t N> struct Type_AOS;
template <template <template <typename> typename> typename T> struct Type_SOAPtr;

namespace __Type_Helpers {

constexpr size_t SOA_ALIGN = 16;

template <typename T, typename S, typename R> static S SOA_convert(R& val)
{
    auto& [p1, p2, p3] = reinterpret_cast<T&>(val); // TODO: Need automatic counting, or define tuple with types, or reflection.
    return S(p1, p2, p3);
}

template <typename T, typename S, typename R> static S SOA_convert(R& val, size_t idx)
{
    auto& [p1, p2, p3] = reinterpret_cast<T&>(val); // TODO: Need automatic counting, or define tuple with types, or reflection.
    return S(p1[idx], p2[idx], p3[idx]);
}

template <typename T> requires std::is_standard_layout_v<T>
using plain_wrapper = T;

template <typename T>
using ref_wrapper = T&;

template <typename T>
using constref_wrapper = const T&;

template <size_t N> struct soa_array {
template <typename T>
using soa_wrapper = alignas(__Type_Helpers::SOA_ALIGN) T[N]; // TODO: Does this alignment work?
};

template <typename T>
using soaptr_wrapper = T*;// TODO: Not sure if we really need this, but could be helpful to pass around to functions.

template <template <template <typename> typename> typename T, typename S>
struct AOS_arrayview
{
    AOS_arrayview(T<__Type_Helpers::plain_wrapper>* s, S T<__Type_Helpers::plain_wrapper>::*p) : source(s), pointer(p) {}
    S& operator[](size_t idx) { return source[idx].*pointer; }
    const S& operator[](size_t idx) const { return source[idx].*pointer; }

private:
    T<__Type_Helpers::plain_wrapper>* source;
    S T<__Type_Helpers::plain_wrapper>::*pointer;
};

template <template <template <typename> typename> typename S>
struct array_helper {
    template <typename T>
    using array_type_wrapper = AOS_arrayview<S, T>;
};

} // namespace Type_Helpers

template <template <template <typename> typename> typename T>
struct Type_Ref : public T<__Type_Helpers::ref_wrapper>
{
    Type_Ref(const T<__Type_Helpers::ref_wrapper>& obj) : T<__Type_Helpers::ref_wrapper>(obj) {}
    Type_Ref(const Type_Ref<T>&) = default;
    Type_Ref() = default;

    auto get_ref(size_t = 0) { return *this; }
    auto get_ref(size_t = 0) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::ref_wrapper>, T<__Type_Helpers::constref_wrapper>>(*this); };
    auto get_copy(size_t = 0) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::ref_wrapper>, T<__Type_Helpers::plain_wrapper>>(*this); };
};

template <template <template <typename> typename> typename T>
struct Type_ConstRef : public T<__Type_Helpers::constref_wrapper>
{
    Type_ConstRef(const T<__Type_Helpers::ref_wrapper>& obj) : T<__Type_Helpers::constref_wrapper>(((const Type_Ref<T>)Type_Ref<T>(obj)).get_ref()) {}
    Type_ConstRef(const T<__Type_Helpers::constref_wrapper>& obj) : T<__Type_Helpers::constref_wrapper>(obj) {}
    Type_ConstRef(const Type_ConstRef<T>&) = default;
    Type_ConstRef() = default;

    auto get_ref(size_t = 0) const { return *this; }
    auto get_copy(size_t = 0) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::constref_wrapper>, T<__Type_Helpers::plain_wrapper>>(*this); };
};

template <template <template <typename> typename> typename T>
struct Type_SOAPtr : public T<__Type_Helpers::soaptr_wrapper>
{
    Type_SOAPtr(const T<__Type_Helpers::soaptr_wrapper>& obj) : T<__Type_Helpers::soaptr_wrapper>(obj) {}
    Type_SOAPtr(const Type_SOAPtr<T>&) = default;
    Type_SOAPtr() = default;
};

template <template <template <typename> typename> typename T>
struct Type_Plain : public T<__Type_Helpers::plain_wrapper>
{
    Type_Plain(const T<__Type_Helpers::plain_wrapper>& obj) : T<__Type_Helpers::plain_wrapper>(obj) {}
    Type_Plain(const Type_Plain<T>&) = default;
    Type_Plain() = default;

    auto get_ref(size_t = 0) { return __Type_Helpers::SOA_convert<T<__Type_Helpers::plain_wrapper>, T<__Type_Helpers::ref_wrapper>>(*this); };
    auto get_ref(size_t = 0) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::plain_wrapper>, T<__Type_Helpers::constref_wrapper>>(*this); };
    auto get_copy(size_t = 0) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::plain_wrapper>, T<__Type_Helpers::plain_wrapper>>(*this); };    
};

template <template <template <typename> typename> typename T, size_t N>
struct Type_SOA : public T<__Type_Helpers::soa_array<N>::template soa_wrapper>
{
    Type_Ref<T> operator[](size_t idx) { return get_ref(idx); }
    Type_ConstRef<T> operator[](size_t idx) const { return get_ref(idx); }

    auto get_ref(size_t idx) { return __Type_Helpers::SOA_convert<T<__Type_Helpers::soa_array<N>::template soa_wrapper>, T<__Type_Helpers::ref_wrapper>>(*this, idx); };
    auto get_ref(size_t idx) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::soa_array<N>::template soa_wrapper>, T<__Type_Helpers::constref_wrapper>>(*this, idx); };
    auto get_copy(size_t idx) const { return __Type_Helpers::SOA_convert<const T<__Type_Helpers::soa_array<N>::template soa_wrapper>, T<__Type_Helpers::plain_wrapper>>(*this, idx); };
    auto get_ptrs() { return __Type_Helpers::SOA_convert<T<__Type_Helpers::soa_array<N>::template soa_wrapper>, T<__Type_Helpers::soaptr_wrapper>>(*this); };
};

template <template <template <typename> typename> typename T, size_t N>
struct Type_AOS : public T<__Type_Helpers::array_helper<T>::template array_type_wrapper>
{
    Type_AOS() : T<__Type_Helpers::array_helper<T>::template array_type_wrapper>(get_arrays()) {

    }
    auto& operator[](size_t idx) { return values[idx]; }
    const auto& operator[](size_t idx) const { return values[idx]; }

    auto get_ref(size_t idx) { return __Type_Helpers::SOA_convert<decltype(values[idx]), T<__Type_Helpers::ref_wrapper>>(values[idx]); };
    auto get_ref(size_t idx) const { return __Type_Helpers::SOA_convert<decltype(values[idx]), T<__Type_Helpers::constref_wrapper>>(values[idx]); };
    auto get_copy(size_t idx) const { return __Type_Helpers::SOA_convert<decltype(values[idx]), T<__Type_Helpers::plain_wrapper>>(values[idx]); };

    template <typename S>
    auto get_array_ptr(S T<__Type_Helpers::plain_wrapper>::*p) {
        return __Type_Helpers::AOS_arrayview<T, S>(values, p);
    }

private:
    T<__Type_Helpers::plain_wrapper> values[N];

    T<__Type_Helpers::array_helper<T>::template array_type_wrapper> get_arrays() {
        // T<Plain_Helper::type_wrapper>::auto* [p1, p2, p3] = values[0]; // TODO: Structured bindings do not work with member variable pointers, or I am too stupid to find out how
        return {this->get_array_ptr(&Type_Plain<T>::x), this->get_array_ptr(&Type_Plain<T>::y), this->get_array_ptr(&Type_Plain<T>::z)};
    }

};

// Type definition

struct sub_point // TODO: what do we do for nested SoAoS?
{
    int u, v;
};

SOASTRUCT() struct point_d
{
    SOATYPE(int) x; // TODO: What can we do for default values / constuctor?
    SOATYPE(float) y;
    SOATYPE(sub_point) z;
};
typedef Type_Plain<point_d> point;

// Usage

int main(int, char**)
{
    // TODO: We need to overwrite the constructors, to either take a custom allocator function, or to create in place in existing memory like placement-new
    Type_SOA<point_d, 10> p_soa;
    Type_AOS<point_d, 10> p_aos;
    point p_plain;

    p_soa.x[1] = 10;
    p_aos.x[1] = 11;

    p_soa[2].x = 20;
    p_aos[2].x = 21;

    p_plain.x = 30;

    point tmp = p_aos[2];
    Type_Ref<point_d> tmp_ref = tmp.get_ref();
    const point tmp2 = tmp;
    [[maybe_unused]] Type_ConstRef<point_d> tmp_ref2 = tmp2.get_ref();
    [[maybe_unused]] Type_ConstRef<point_d> tmp_ref3 = tmp_ref;

    [[maybe_unused]] point x1 = p_plain.get_copy(), x2 = p_soa.get_copy(1), x3 = p_aos.get_copy(2), x4 = tmp_ref.get_copy();
    [[maybe_unused]] Type_SOAPtr<point_d> ptrs = p_soa.get_ptrs();

    auto test = p_aos.get_array_ptr(&point::z);
    test[3].u = 123;

    printf("FOO %3d %3d %3d - %3d %3d %3d - %3d\n", p_soa.x[1], p_aos[2].x, p_plain.x, p_aos.x[1], p_soa[2].x, tmp.x, p_aos[3].z.u);

    // Check references are working
    Type_Ref<point_d> r_plain = p_plain.get_ref();
    Type_Ref<point_d> r_soa = p_soa.get_ref(1);
    Type_Ref<point_d> r_aos = p_aos.get_ref(2);
    p_plain.x += 100;
    p_soa.x[1] += 100;
    p_aos[2].x += 100;
    printf("BAR %3d %3d %3d\n", r_soa.x, r_aos.x, r_plain.x);

    return 0;
}
