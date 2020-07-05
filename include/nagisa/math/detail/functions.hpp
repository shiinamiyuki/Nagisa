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
#include "types.hpp"
namespace nagisa::math {
    template<typename T>
    inline float select(bool c, const T & a, const T & b){
        return c ? a : b;
    }
    
#define GEN_VEC_OP(name, op)                                                                                           \
    template <typename T, size_t N, int packed> struct name {                                                          \
        using V = Vector<T, N, packed>;                                                                                \
        using R = decltype(std::declval<T>() op std::declval<T>());                                                    \
        static auto compute(const V &a, const V &b) {                                                                  \
            Vector<R, N, packed> r;                                                                                    \
            for (int i = 0; i < V::padded_size; i++) {                                                                 \
                r[i] = a[i] op b[i];                                                                                   \
            }                                                                                                          \
            return r;                                                                                                  \
        }                                                                                                              \
    };                                                                                                                 \
    template <typename T, size_t N, int packed, typename V = Vector<T, N, packed>>                                     \
    auto operator op(const V &a, const V &b) {                                                                         \
        return name<T, N, packed>::compute(a, b);                                                                      \
    }

    GEN_VEC_OP(VectorAdd, +)
    GEN_VEC_OP(VectorSub, -)
    GEN_VEC_OP(VectorMul, *)
    GEN_VEC_OP(VectorDiv, /)
    GEN_VEC_OP(VectorDiv, %)
    GEN_VEC_OP(VectorCmpLt, <)
    GEN_VEC_OP(VectorCmpLe, <=)
    GEN_VEC_OP(VectorCmpGt, >)
    GEN_VEC_OP(VectorCmpGt, >=)
    GEN_VEC_OP(VectorCmpEq, ==)
    GEN_VEC_OP(VectorCmpNe, !=)
    GEN_VEC_OP(VectorAnd, &&)
    GEN_VEC_OP(VectorBitAnd, &)
    GEN_VEC_OP(VectorOr, ||)
    GEN_VEC_OP(VectorBitOr, |)
    GEN_VEC_OP(VectorBitXor, ^)
    GEN_VEC_OP(VectorShl, <<)
    GEN_VEC_OP(VectorShr, >>)
#undef GEN_VEC_OP
#define GEN_VEC_UNARY_OP(name, op)                                                                                     \
    template <typename T, size_t N, int packed> struct name {                                                          \
        using V = Vector<T, N, packed>;                                                                                \
        using R = decltype(op std::declval<T>());                                                                      \
        static auto compute(const V &a) {                                                                              \
            Vector<R, N, packed> r;                                                                                    \
            for (int i = 0; i < V::padded_size; i++) {                                                                 \
                r[i] = op a[i];                                                                                        \
            }                                                                                                          \
            return r;                                                                                                  \
        }                                                                                                              \
    };                                                                                                                 \
    template <typename T, size_t N, int packed, typename V = Vector<T, N, packed>>                                     \
    auto operator op(const V &a, const V &b) {                                                                         \
        return name<T, N, packed>::compute(a, b);                                                                      \
    }
    GEN_VEC_UNARY_OP(VectorNeg, -)
    GEN_VEC_UNARY_OP(VectorBitNot, ~)
    GEN_VEC_UNARY_OP(VectorNot, !)
#undef GEN_VEC_UNARY_OP
    template <typename T, size_t N, int packed> struct VectorSelect {
        using V = Vector<T, N, packed>;

        static auto compute(const Vector<bool, N, packed> &cond, const V &a, const V &b) {
            V r;
            for (int i = 0; i < V::padded_size; i++) {
                r[i] = select(cond[i], a[i], b[i]);
            }
            return r;
        }
    };

    template <typename T, size_t N, int packed, typename C, typename V = Vector<T, N, packed>>
    auto select(const C &cond, const V &a, const V &b) {
        return VectorSelect<T, N, packed>::compute(cond, a, b);
    }

    template <int I0, int I1, typename T, int N, int packed>
    Vector<T, N, packed> permute(const Vector<T, N, packed> &v) {
        static_assert(N >= 2);
        Vector r;
        r[0] = v[I0];
        r[1] = v[I1];
        return r;
    }
    template <int I0, int I1, int I2, typename T, int N, int packed>
    Vector<T, N, packed> Vector permute(const Vector<T, N, packed> &v) {
        static_assert(N >= 3);
        Vector r;
        r[0] = v[I0];
        r[1] = v[I1];
        r[2] = v[I2];
        return r;
    }
    template <int I0, int I1, int I2, int I3, typename T, int N, int packed>
    Vector<T, N, packed> Vector permute(const Vector<T, N, packed> &v) {
        static_assert(N >= 4);
        Vector r;
        r[0] = v[I0];
        r[1] = v[I1];
        r[2] = v[I2];
        r[3] = v[I3];
        return r;
    }

#define VEC_DEF_FUNC1(trait, name)                                                                                     \
    template <typename T, int N, int packed, V = Vector<T, N, packed>> auto name(const V &a) {                         \
        return trait<T, N, packed>::compute(a);                                                                        \
    }
#define VEC_DEF_FUNC2(trait, name)                                                                                     \
    template <typename T, int N, int packed, V = Vector<T, N, packed>> auto name(const V &a, const V &b) {             \
        return trait<T, N, packed>::compute(a, b);                                                                     \
    }
    template <typename T, int N, int packed> struct VectorDot {
        using V = Vector<T, N, packed>;
        static auto compute(const V &a, const V &b) {
            auto s = a[0] * b[0];
            for (int i = 1; i < N; i++) {
                s += a[i] * b[i];
            }
            return s;
        }
    };
    VEC_DEF_FUNC2(VectorDot, dot)
    template <typename T, int N, int packed> struct VectorLength {
        using V = Vector<T, N, packed>;
        static auto compute(const V &a) { return sqrt(dot(a, a)); }
    };
    VEC_DEF_FUNC2(VectorLength, length)
    template <typename T, int N, int packed> struct VectorNormalize {
        using V = Vector<T, N, packed>;
        static auto compute(const V &a) { return a / sqrt(dot(a, a)); }
    };
    VEC_DEF_FUNC2(VectorNormalize, normalize)
    template <typename T, int N, int packed> struct VectorDistance {
        using V = Vector<T, N, packed>;
        static auto compute(const V &a, const V &b) { return length(a - b); }
    };
    VEC_DEF_FUNC2(VectorDistance, distance)

    template <typename T, int N> Vector<T, N, 0> unpack(const Vector<T, N, 1> &u) const {
        Vector<T, N, 0> u;
        for (int i = 0; i < N; i++) {
            u[i] = _s[i];
        }
        return u;
    }

    template <typename T, int N> Vector<T, N, 1> pack(const Vector<T, N, 0> &u) const {
        Vector<T, N, 0> u;
        for (int i = 0; i < N; i++) {
            u[i] = _s[i];
        }
        return u;
    }
} // namespace nagisa::math