List of points that are currently unclear to me, how to realize best:

# Interface
From my test, I like the idea that both the SOA and the AOS data type expose both interfaces. That is fully flexible. If nothing speaks against it, I'd do it like this.

# Constructors
I would consider the following cases:
- Just a normal constructor.
- The possibility to provide an allocator, and be compatible with c++ memory resources
- A possibility to use preallocated memory. For this, we should also provide statig getters, that tell us how a large an soa or aos with n entries of struct X would be, including potential alignment.

# Default values
- It would be good if we could define default values for the constructor in C++11 style (https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L205).
- However, I don't really see how this can work in the SOA case. But ideally, I would define them in the naive way in the skelleton-struct definition, and then they should be automatically applied.

# Create uninitialized
- It might make sense to foresee a way to create an soa/aos without initializing the data, if the user knows he will anyway write to all of them. Then we can skip the initialialization explicitly.

# A general problem is how to do the conversion, which I currently attempted with structured binding, but for this we need the count.
- See https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L31
- One can count with some template magic in c++17, like in https://gist.github.com/utilForever/1a058050b8af3ef46b58bcfa01d5375d, with c++20 the counting can be improved: https://stackoverflow.com/questions/4024632/how-to-get-the-number-of-elements-in-a-struct. And obviously with reflection, this can be solved. But meanwhile we should find the smoothest temporary solution.

# Compile time
- If we use a lot of template code, we must make sure that the compile time and memory consumption does not grow out of control, and that we get reasonable error message in case of incorrect usage by the user.
- With the above examples, I had one case where a syntax error lead to ~5 seconds of compilation, until the compiler told me that no template candidate is valid. I think we should have some test cases, also failing cases, and regularly check the compile time.

# `span` types
- I think it will be really important to have `span` types, which point to a subregion of an aos / soa meta type.
- I added some tentative interface here: https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L154
- We should have constructors so we can easily create a span from any aos / soa type selecting a range, and obviously it needs to work with const / non-const / etc. objects, automatically obtaining the corresponding span.

# Which metatypes do we want to provide in the end. In my example I added a couple of types for special cases.
- https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L22: `span` types like `Type_AOS_s` will probably be needed in 2 flavors, referencing to const objects and non-const objects?
- https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L23: In order to support data from external arrays, a pure pointer type without size might be helpful, since perhaps we do not know the size of the AOS/SOD in every case: https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L23

# Alignment
- In cases where we create the memory layout for our soa/aos metaobject ourselves, we should have the possibility to define some alignment constraints.
- See https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L52
- E.g., if we have multiple large arrays, we could make sure that they start at a CPU cache line, or that they match the largest vector load the GPU can do.

# Alignment 2
- If we get memory for types, or references to one of our types from outside, we must provide an optimized version in case the user guarantues a certain alignment, but we must also provide a fallback solution if no alignment can be guaranteed.

# Template solution for creating spans with arbitrary number of members in the struct:
- In my playground https://github.com/davidrohr/soa_aos_tests/blob/f12c3f63535e653d500fb1ea46527ee7d22f7d9a/aos_soa.cpp#L190, I can currently create a span of an AOS for a fixed number of members.
- I did not manage to apply the above hack for structured binding with counting to this case. Is there a template metaprogramming solution, or does it require reflection?

# Member functions:
- I thought a bit about member functions. Ideally, I would of course simply define then in the skelleton struct.
- But the question is actually, what happens if I call the member function on an soa/aos? Would it apply the function on all elements of the array? Perhaps this is not really needed.
- I think what is more relevant is to have member functions for the plain struct and the struct of references. Actually for these two, just defining the member functions insite the skelleton class in my above example would work. But it would simply not even compile for the soa / aos classes.
- One workaround would be to have the skelleton class only for the members, and then derrive a class that adds the functions, e.g.
```
SOASTRUCT() struct point_d
{
    SOATYPE(float) x, y, z;
};
template <class T>
struct point_functions : T
{
  void foo () { x = 0; }
};
typedef point_functions<Type_Plain<point_d>> point;
typedef point_functions<Type_Ref<point_d>> point_ref;
```
- Something like this might work, but I think it is not yet ideal.

# Passing references:
I am wondering how to pass references to a function in a generic way. E.g.
```
void foo(point& p) {
...
}
Type_SOA<point_d, 10> p_soa;
Type_AOS<point_d, 10> p_aos;
foo(p_soa[5]);
foo(p_aos[5]);
```
In my current implementation, `p_soa[5]` and `p_aos[5]` have different types, even though both types offer the exact same interface. Now, the options I see are:
- The function must be templated, which would be pretty much OK for inline function, but a nightmare for multiple compilation units as you'd need to instantiate the templates explicitly.
- One could use onle the reference type, and always pass the `.get_ref()`, which will always be the same type. However, in case of the `p_aos`, `get_ref()` creates a struct of references, instead of passing a single references, so this would introduce significant overhead, which we should avoid.
- We could have a second meta-reference type, with a type flag, that would then either store a reference to the plain type, or a reference to the refence type. But in that case, every access would need to check the type flag, adding plenty of `if` statements.
- Is there any better solution?

# Nested structures: Support for AoSoA and SoAoS
- I think we should support nested structures. SoAoS in case some parameters are always used together, e.g. x and y coordinates. And AoSoA to store things in columnar way for vector access, but keep some cache locality.
- I suppose AoSoA should be straigt forward to do, with an array of SoA and some magic for the access operators. We should make sure that `for ( auto& : ...)` loops work efficiently.
- SoAoS is more complicated: Do we want to define the struct substructure when defining the variable, or could it be part of the skelleton class definition? Perhaps it is easiest to start with the latter as done here https://github.com/davidrohr/soa_aos_tests/blob/352405f771e7100c3a0409777d636a2ce3179cc8/aos_soa.cpp#L207, which works transparently.
