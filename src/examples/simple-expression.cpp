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

#include <nagisa/nagisa.h>
#include <nagisa/debug.h>
using namespace nagisa;
using namespace lang;

struct vec2 {
    float x, y;
};

NGS_STRUCT(vec2, x, y)

int main() {
    

    // Function<float32(float32)> f = [](float32 x) -> float32 {
    //     auto sqr = [](float32 x) -> float32 { return select<float32>(x < 0, 0.0f, x * x); };
    //     return sqr(x) + 2.0f;
    // };
    Function<float32(Var<vec2>)> f = [](Var<vec2> p)->float32{
        return p.x + p.y;
    };
    std::cout << debug::to_text(f.__get_func_node()) << std::endl;
    auto be = create_llvm_backend();
    auto fp = f.compile(be);
    vec2 p{1, 2};
    std::cout << fp(p) << std::endl;
}
