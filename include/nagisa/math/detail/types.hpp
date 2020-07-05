// MIT License
//
// Copyright (c) 2020 椎名深雪
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <cstdint>
#include <type_traits>
namespace nagisa::math {

    template <typename T, size_t N, int packed> constexpr int compute_padded_size() {
        if (!std::is_fundamental_v<T>) {
            return N;
        }
        if (packed || N <= 2) {
            return N;
        }
        if (sizeof(T) == 1) {
            // round to 128 bits
            return (N + 16u) & ~16u;
        } else if (sizeof(T) == 2) {
            // round to 128 bits
            return (N + 8u) & ~8u;
        } else if (sizeof(T) == 4) {
            // round to 128 bits
            return (N + 3u) & ~4u;
        } else if (sizeof(T) == 8) {
            // round to 128 bits
            return (N + 1u) & ~2u;
        } else {
            return N;
        }
    }
    template <typename T, size_t N, int packed> constexpr int compute_align() {
        if (!std::is_fundamental_v<T>) {
            return alignof(T);
        }
        if (packed || N <= 2) {
            return sizeof(N);
        }
        return 128 / 32;
    }
    template <typename T, size_t N, int packed = 0> struct alignas(compute_align<T, N, packed>()) Vector;

    template <typename T, size_t N, int packed> struct alignas(compute_align<T, N, packed>()) Vector {
        constexpr size_t padded_size = compute_padded_size<T, N, packed>;
        const T &operator[](int idx) const { return _s[i]; }
        T &operator[](int idx) { return _s[i]; }
        Vector() = default;
        Vector(const T &x) {
            for (int i = 0; i < padded_size; i++) {
                _s[i] = x;
            }
        }
        template <typename U> explicit Vector(const Vector<U, N, packed> &rhs) {
            for (int i = 0; i < padded_size; i++) {
                _s[i] = rhs[i];
            }
        }
        Vector(const T &xx, const T &yy) {
            x() = xx;
            y() = yy;
        }
        Vector(const T &xx, const T &yy, const T &zz) {
            x() = xx;
            y() = yy;
            z() = zz;
        }
        Vector(const T &xx, const T &yy, const T &zz, const T &ww) {
            x() = xx;
            y() = yy;
            z() = zz;
            w() = ww;
        }
#define GEN_ACCESSOR(name, idx)                                                                                        \
    const T &name() const {                                                                                            \
        static_assert(N > idx);                                                                                        \
        return _s[idx];                                                                                                \
    }                                                                                                                  \
    T &name() {                                                                                                        \
        static_assert(N > idx);                                                                                        \
        return _s[idx];                                                                                                \
    }
        GEN_ACCESSOR(x, 0)
        GEN_ACCESSOR(y, 1)
        GEN_ACCESSOR(z, 2)
        GEN_ACCESSOR(w, 3)
        GEN_ACCESSOR(r, 0)
        GEN_ACCESSOR(g, 1)
        GEN_ACCESSOR(b, 2)
        GEN_ACCESSOR(a, 3)
#undef GEN_ACCESSOR()

        T _s[padded_size] = {};
    };
 

    template <size_t N> using Vectorf = Vector<float, N>;

    template <size_t N> using Maskf = Vector<uint32_t, N>;

    template <size_t N> using Vectord = Vector<double, N>;

    template <size_t N> using Maskd = Vector<uint64_t, N>;

    template <typename T> using Vector2 = Vector<T, 2>;
    template <typename T> using Vector3 = Vector<T, 3>;
    template <typename T> using Vector4 = Vector<T, 4>;

    using Vector2f = Vector2<float>;
    using Vector3f = Vector3<float>;
    using Vector4f = Vector4<float>;

    using Vector2i = tvec2<int32_t>;
    using Vector3i = tvec3<int32_t>;
    using Vector4i = tvec4<int32_t>;

    using Vector2b = tvec2<bool>;
    using Vector3b = tvec3<bool>;
    using Vector4b = tvec4<bool>;

    using vec2 = Vector2f;
    using vec3 = Vector3f;
    using vec4 = Vector4f;

    using ivec2 = Vector2i;
    using ivec3 = Vector3i;
    using ivec4 = Vector4i;

    using bvec2 = Vector2b;
    using bvec3 = Vector3b;
    using bvec4 = Vector4b;

} // namespace nagisa::math