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
#include "vec.hpp"

namespace nagisa::math {
    template<typename T, int N>
    struct tmat{
        tvec<T, N> col[N];
    };

    template <typename T> using tmat2 = tmat<T, 2>;
    template <typename T> using tmat3 = tmat<T, 3>;
    template <typename T> using tmat4 = tmat<T, 4>;

    using mat2 = tmat2<float>;
    using mat3 = tmat3<float>;
    using mat4 = tmat4<float>;

    using imat2 = tmat2<int32_t>;
    using imat3 = tmat3<int32_t>;
    using imat4 = tmat4<int32_t>;

    using bmat2 = tmat2<bool>;
    using bmat3 = tmat3<bool>;
    using bmat4 = tmat4<bool>;
}