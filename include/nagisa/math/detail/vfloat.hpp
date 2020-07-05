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

#include "functions.hpp"

namespace nagisa::math {
#define GEN_VEC_FLOAT_CMP(name, op)                                                                                    \
    template <size_t N> struct name<float, N, 0> {                                                                     \
        using V = Vectorf<N>;                                                                                          \
        using M = Maskf<N>;                                                                                            \
        static auto compute(const V &a, const V &b) {                                                                  \
            M r;                                                                                                       \
            for (int i = 0; i < V::padded_size; i++) {                                                                 \
                r[i] = a[i] op b[i];                                                                                   \
            }                                                                                                          \
            return r;                                                                                                  \
        }                                                                                                              \
    };
    GEN_VEC_FLOAT_CMP(VectorCmpLt, <)
    GEN_VEC_FLOAT_CMP(VectorCmpLe, <=)
    GEN_VEC_FLOAT_CMP(VectorCmpGt, >)
    GEN_VEC_FLOAT_CMP(VectorCmpGt, >=)
    GEN_VEC_FLOAT_CMP(VectorCmpEq, ==)
    GEN_VEC_FLOAT_CMP(VectorCmpNe, !=)
#undef GEN_VEC_FLOAT_CMP

    template <size_t N> struct VectorSelect<float, N, 0> {
        using V = Vectorf<N>;
        using M = Maskf<N>;

        static auto compute(const M &cond, const V &a, const V &b) {
            V r;
            for (int i = 0; i < V::padded_size; i++) {
                r[i] = cond[i] ? a[i] : b[i];
            }
            return r;
        }
    };
} // namespace nagisa::math