#include <cstdlib>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <utility>

// Helper code

#define SOASTRUCT() template <template <typename, size_t> typename __SOA_T, size_t __SOA_N>
#define SOATYPE(__SOA_TYPE) typename __SOA_T<__SOA_TYPE, __SOA_N>::type

constexpr size_t SOA_ALIGN = 16;

// forward declarations
template <template <template <typename, size_t> typename, size_t> typename T> struct Type_Ref;
template <template <template <typename, size_t> typename, size_t> typename T> struct Type_ConstRef;
template <template <template <typename, size_t> typename, size_t> typename T> struct Type_Plain;
template <template <template <typename, size_t> typename, size_t> typename T, size_t N> struct Type_SOA;
template <template <template <typename, size_t> typename, size_t> typename T, size_t N> struct Type_AOS;
template <template <template <typename, size_t> typename, size_t> typename T> struct Type_SOAPtr;

template <typename T, typename S, typename R> static S __SOA_convert(R& val)
{
    auto& [p1, p2, p3] = reinterpret_cast<T&>(val); // TODO: Need automatic counting, or define tuple with types, or reflection.
    return S(p1, p2, p3);
}

template <typename T, typename S, typename R> static S __SOA_convert(R& val, size_t idx)
{
    auto& [p1, p2, p3] = reinterpret_cast<T&>(val); // TODO: Need automatic counting, or define tuple with types, or reflection.
    return S(p1[idx], p2[idx], p3[idx]);
}

struct Plain_Helper
{
    template <typename T, size_t> struct type_wrapper
    {
        static_assert(std::is_standard_layout_v<T> == true, "Invalid type for SOATypes"); // TODO: can we do the asserts better, do we need them?
        typedef T type;
    };
};

struct Ref_Helper
{
    template <typename T, size_t> struct type_wrapper
    {
        static_assert(std::is_standard_layout_v<T> == true, "Invalid type for SOATypes");
        typedef T& type;
    };
};

struct ConstRef_Helper
{
    template <typename T, size_t> struct type_wrapper
    {
        static_assert(std::is_standard_layout_v<T> == true, "Invalid type for SOATypes");
        typedef const T& type;
    };
};

struct SOA_Helper
{
    template <typename T, size_t N> struct type_wrapper
    {
        static_assert(std::is_standard_layout_v<T> == true, "Invalid type for SOATypes");
        typedef T type[N] __attribute__ ((aligned(SOA_ALIGN))); // TODO: Does this alignment work?
    };
};

struct SOAPtr_Helper // TODO: Not sure if we really need this, but could be helpful to pass around to functions.
{
    template <typename T, size_t N> struct type_wrapper
    {
        static_assert(std::is_standard_layout_v<T> == true, "Invalid type for SOATypes");
        typedef T* type;
    };
};

template <template <template <typename, size_t> typename, size_t> typename T, size_t N, typename S> struct __AOS_arrayview
{
    __AOS_arrayview(T<Plain_Helper::type_wrapper, 1>* s, S T<Plain_Helper::type_wrapper, 1>::*p) : source(s), pointer(p) {}
    S& operator[](size_t idx) { return source[idx].*pointer; }
    const S& operator[](size_t idx) const { return source[idx].*pointer; }

private:
    T<Plain_Helper::type_wrapper, 1>* source;
    S T<Plain_Helper::type_wrapper, 1>::*pointer;
};

struct AOS_Helper
{
    template <template <template <typename, size_t> typename, size_t> typename S> struct array_helper {
        template <typename T, size_t N> struct array_type_wrapper
        {
            typedef __AOS_arrayview<S, N, T> type;
        };
    };
};

template <template <template <typename, size_t> typename, size_t> typename T> struct Type_Ref : public T<Ref_Helper::type_wrapper, 1>
{
    Type_Ref(const T<Ref_Helper::type_wrapper, 1>& obj) : T<Ref_Helper::type_wrapper, 1>(obj) {}
    Type_Ref(const Type_Ref<T>&) = default;
    Type_Ref() = default;

    auto get_ref(size_t = 0) { return *this; }
    auto get_ref(size_t = 0) const { return __SOA_convert<const T<Ref_Helper::type_wrapper, 1>, T<ConstRef_Helper::type_wrapper, 1>>(*this); };
    auto get_copy(size_t = 0) const { return __SOA_convert<const T<Ref_Helper::type_wrapper, 1>, T<Plain_Helper::type_wrapper, 1>>(*this); };
};

template <template <template <typename, size_t> typename, size_t> typename T> struct Type_ConstRef : public T<ConstRef_Helper::type_wrapper, 1>
{
    Type_ConstRef(const T<Ref_Helper::type_wrapper, 1>& obj) : T<ConstRef_Helper::type_wrapper, 1>(((const Type_Ref<T>)Type_Ref<T>(obj)).get_ref()) {}
    Type_ConstRef(const T<ConstRef_Helper::type_wrapper, 1>& obj) : T<ConstRef_Helper::type_wrapper, 1>(obj) {}
    Type_ConstRef(const Type_ConstRef<T>&) = default;
    Type_ConstRef() = default;

    auto get_ref(size_t = 0) const { return *this; }
    auto get_copy(size_t = 0) const { return __SOA_convert<const T<ConstRef_Helper::type_wrapper, 1>, T<Plain_Helper::type_wrapper, 1>>(*this); };
};

template <template <template <typename, size_t> typename, size_t> typename T> struct Type_SOAPtr : public T<SOAPtr_Helper::type_wrapper, 1>
{
    Type_SOAPtr(const T<SOAPtr_Helper::type_wrapper, 1>& obj) : T<SOAPtr_Helper::type_wrapper, 1>(obj) {}
    Type_SOAPtr(const Type_SOAPtr<T>&) = default;
    Type_SOAPtr() = default;
};

template <template <template <typename, size_t> typename, size_t> typename T> struct Type_Plain : public T<Plain_Helper::type_wrapper, 1>
{
    Type_Plain(const T<Plain_Helper::type_wrapper, 1>& obj) : T<Plain_Helper::type_wrapper, 1>(obj) {}
    Type_Plain(const Type_Plain<T>&) = default;
    Type_Plain() = default;

    auto get_ref(size_t = 0) { return __SOA_convert<T<Plain_Helper::type_wrapper, 1>, T<Ref_Helper::type_wrapper, 1>>(*this); };
    auto get_ref(size_t = 0) const { return __SOA_convert<const T<Plain_Helper::type_wrapper, 1>, T<ConstRef_Helper::type_wrapper, 1>>(*this); };
    auto get_copy(size_t = 0) const { return __SOA_convert<const T<Plain_Helper::type_wrapper, 1>, T<Plain_Helper::type_wrapper, 1>>(*this); };    
};

template <template <template <typename, size_t> typename, size_t> typename T, size_t N> struct Type_SOA : public T<SOA_Helper::type_wrapper, N>
{
    Type_Ref<T> operator[](size_t idx) { return get_ref(idx); }
    Type_ConstRef<T> operator[](size_t idx) const { return get_ref(idx); }

    auto get_ref(size_t idx) { return __SOA_convert<T<SOA_Helper::type_wrapper, 1>, T<Ref_Helper::type_wrapper, 1>>(*this, idx); };
    auto get_ref(size_t idx) const { return __SOA_convert<const T<SOA_Helper::type_wrapper, 1>, T<ConstRef_Helper::type_wrapper, 1>>(*this, idx); };
    auto get_copy(size_t idx) const { return __SOA_convert<const T<SOA_Helper::type_wrapper, 1>, T<Plain_Helper::type_wrapper, 1>>(*this, idx); };
    auto get_ptrs() { return __SOA_convert<T<SOA_Helper::type_wrapper, 1>, T<SOAPtr_Helper::type_wrapper, 1>>(*this); };
};

// Type definition

struct sub_point
{
    int u, v;
};

SOASTRUCT() struct point_def
{
    SOATYPE(int) x; // TODO: What can we do for default values / constuctor?
    SOATYPE(float) y;
    SOATYPE(sub_point) z;
};
typedef Type_Plain<point_def> point;

// TODO: In this class, T should be point_def, but it some places I had to write point_def explicitly or I get a compile error, could not yet figure out why. This should be fixed. Then, we can also move this class body above the point_def definition.
template <template <template <typename, size_t> typename, size_t> typename T, size_t N> struct Type_AOS : public T<AOS_Helper::array_helper<point_def>::array_type_wrapper, N>
{
    Type_AOS() : T<AOS_Helper::array_helper<point_def>::array_type_wrapper, N>(get_arrays()) {

    }
    auto& operator[](size_t idx) { return values[idx]; }
    const auto& operator[](size_t idx) const { return values[idx]; }

    auto get_ref(size_t idx) { return __SOA_convert<decltype(values[idx]), T<Ref_Helper::type_wrapper, 1>>(values[idx]); };
    auto get_ref(size_t idx) const { return __SOA_convert<decltype(values[idx]), T<ConstRef_Helper::type_wrapper, 1>>(values[idx]); };
    auto get_copy(size_t idx) const { return __SOA_convert<decltype(values[idx]), T<Plain_Helper::type_wrapper, 1>>(values[idx]); };

    template <typename S>
    auto get_array_ptr(S T<Plain_Helper::type_wrapper, 1>::*p) {
        return __AOS_arrayview<T, N, S>(values, p);
    }

private:
    T<Plain_Helper::type_wrapper, 1> values[N];

    T<AOS_Helper::array_helper<point_def>::array_type_wrapper, N> get_arrays() {
        // T<Plain_Helper::type_wrapper, 1>::auto* [p1, p2, p3] = values[0]; // TODO: Structured bindings do not work with member variable pointers, or I am too stupid to find out how
        return {this->get_array_ptr(&Type_Plain<T>::x), this->get_array_ptr(&Type_Plain<T>::y), this->get_array_ptr(&Type_Plain<T>::z)};
    }

};

// Usage

int main(int, char**)
{
    // TODO: We need to overwrite the constructors, to either take a custom allocator function, or to create in place in existing memory like placement-new
    Type_SOA<point_def, 10> p_soa;
    Type_AOS<point_def, 10> p_aos;
    point p_plain;

    p_soa.x[1] = 10;
    p_aos.x[1] = 11;

    p_soa[2].x = 20;
    p_aos[2].x = 21;

    p_plain.x = 30;

    point tmp = p_aos[2];
    Type_Ref<point_def> tmp_ref = tmp.get_ref();
    const point tmp2 = tmp;
    [[maybe_unused]] Type_ConstRef<point_def> tmp_ref2 = tmp2.get_ref();
    [[maybe_unused]] Type_ConstRef<point_def> tmp_ref3 = tmp_ref;

    [[maybe_unused]] point x1 = p_plain.get_copy(), x2 = p_soa.get_copy(1), x3 = p_aos.get_copy(2), x4 = tmp_ref.get_copy();
    [[maybe_unused]] Type_SOAPtr<point_def> ptrs = p_soa.get_ptrs();

    auto test = p_aos.get_array_ptr(&point::z);
    test[3].u = 123;

    printf("FOO %3d %3d %3d - %3d %3d %3d - %3d\n", p_soa.x[1], p_aos[2].x, p_plain.x, p_aos.x[1], p_soa[2].x, tmp.x, p_aos[3].z.u);

    // Check references are working
    Type_Ref<point_def> r_plain = p_plain.get_ref();
    Type_Ref<point_def> r_soa = p_soa.get_ref(1);
    Type_Ref<point_def> r_aos = p_aos.get_ref(2);
    p_plain.x += 100;
    p_soa.x[1] += 100;
    p_aos[2].x += 100;
    printf("BAR %3d %3d %3d\n", r_soa.x, r_aos.x, r_plain.x);

    return 0;
}