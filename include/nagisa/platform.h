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
#ifdef _MSC_VER
#pragma warning(push, 4)
#pragma warning(disable : 4100)
#pragma warning(disable : 4244)
#pragma warning(disable : 4146)
#pragma warning(disable : 4305)
#pragma warning(disable : 4244)

#else
#pragma GCC diagnostic error "-Wall"
#pragma clang diagnostic error "-Wall"
#pragma GCC diagnostic ignored "-Wc++11-compat"
#pragma clang diagnostic ignored "-Wc++11-compat"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wattributes"
#pragma clang diagnostic ignored "-Wattributes"
#endif

#ifdef _MSC_VER
    #define NAGISA_API __declspec(dllexport)
    #pragma warning(disable : 4275)
    #pragma warning(disable : 4267)
    #pragma warning(disable : 4251) // 'field' : class 'A' needs to have dll-interface to be used by clients of class 'B'
    #pragma warning(disable : 4800) // 'type' : forcing value to bool 'true' or 'false' (performance warning)
    #pragma warning(disable : 4996) // Secure SCL warnings
    #pragma warning(disable : 5030)
#else
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef __GNUC__
          #define NAGISA_API __attribute__ ((dllexport))
          
        #else
          #define NAGISA_API __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else

        #define NAGISA_API __attribute__ ((visibility ("default")))

    #endif
#endif

#ifdef __GNUC__
#if __GNUC__ >= 8

#include <filesystem>
namespace nagisa {
    namespace fs = std::filesystem;
}
#else
#include <experimental/filesystem>
namespace nagisa {
    namespace fs = std::experimental::filesystem;
}
#endif
#else
#include <filesystem>
namespace nagisa {
    namespace fs = std::filesystem;
}

#endif


namespace nagisa {
     [[noreturn]] inline void panic(const char *file, int line, const char *msg) {
        fprintf(stderr, "PANIC at %s:%d: %s\n", file, line, msg);
        abort();
    }
#define NAGISA_PANIC(msg) panic(__FILE__, __LINE__, msg)
#define NAGISA_CHECK(expr)                                                                                              \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            fprintf(stderr, #expr " not satisfied at %s:%d\n", __FILE__, __LINE__);                                    \
        }                                                                                                              \
    } while (0)
#define NAGISA_ASSERT(expr)                                                                                             \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            NAGISA_PANIC(#expr " not satisfied");                                                                       \
        }                                                                                                              \
    } while (0)

}