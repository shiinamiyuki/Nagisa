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
#include <memory>
#include <type_traits>
namespace nagisa {
    template <typename T> struct is_shared_ptr : std::false_type {};
    template <typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
    class Base : public std::enable_shared_from_this<Base> {
      public:
        template <typename T> bool isa() const {
            static_assert(is_shared_ptr<T>::value);
            using U = typename T::element_type;

            return dynamic_cast<const U *>(this) != nullptr;
        }
        template <typename T> T cast() const {
            static_assert(is_shared_ptr<T>::value);
            using U = typename T::element_type;
            return std::dynamic_pointer_cast<const U>(shared_from_this());
        }
        template <typename T> T cast() {
            static_assert(is_shared_ptr<T>::value);
            using U = typename T::element_type;
            return std::dynamic_pointer_cast<U>(shared_from_this());
        }
        virtual std::string type_name() const = 0;
        virtual ~Base() = default;
    };
} // namespace nagisa