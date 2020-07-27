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
#include <string_view>
#include <vector>
#define NGS_ASSERT(expr)                                                                                               \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, #expr);                                                  \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)
namespace nagisa {
    class Node;
    enum class Type { none, boolean, f32, i32 };
    void nagisa_init();
    void nagisa_destroy();
    void nagisa_eval();
    class DeviceBuffer;
    std::pair<DeviceBuffer *, int32_t> nagisa_alloc(size_t, Type);
    void nagisa_free(DeviceBuffer *);
    void nagisa_set_var_size(int idx, size_t);

    template <typename Value>
    constexpr Type get_type() {
        if constexpr (std::is_same_v<Value, int32_t> || std::is_same_v<Value, bool>) {
            return Type::i32;
        } else if constexpr (std::is_same_v<Value, float>) {
            return Type::f32;
        } else {
            static_assert(false);
        }
    }

    enum Opcode { FAdd, FSub, FMul, FDiv, ConstantInt, ConstantFloat, Store };
    struct Instruction {
        Opcode op;
        union {
            int ival;
            double fval;
            int operand[3] = {-1};
            struct {
                int buffer_id;
                int idx;
                int value;
                int mask;
            } store_inst;
        };
        static Instruction binary(Opcode op, int a, int b) {
            Instruction i;
            i.op = op;
            i.operand[0] = a;
            i.operand[1] = b;
            return i;
        }
        static Instruction const_int(int x) {
            Instruction i;
            i.op = ConstantInt;
            i.ival = x;
            return i;
        }
        static Instruction const_float(double x) {
            Instruction i;
            i.op = ConstantFloat;
            i.fval = x;
            return i;
        }
        static Instruction store(int buffer, int idx, int value, int mask) {
            Instruction i;
            i.op = Store;
            i.store_inst.buffer_id = buffer;
            i.store_inst.idx = idx;
            i.store_inst.value = value;
            i.store_inst.mask = mask;
            return i;
        }
    };

    int nagisa_trace_append(const Instruction &i, Type type);
    enum class Predefined { ThreadIdx = 0, Total };
    class DeviceBuffer {
      public:
        const Type type;
        DeviceBuffer(Type type) : type(type) {}
        virtual ~DeviceBuffer() = default;
        virtual size_t size() = 0;
        virtual void write(const uint8_t *p, size_t bytes, size_t offset) = 0;
        virtual void read(uint8_t *p, size_t bytes, size_t offset) = 0;
        virtual void *get() = 0;
    };

    template <typename Value>
    class Buffer {
        DeviceBuffer *impl;
        std::vector<Value> _data;
        using Index = int;
        Index _index;
        Buffer(const Buffer &) = delete;

      public:
        const Type type = get_type<Value>();
        explicit Buffer(size_t count) : _data(count) {
            auto p = nagisa_alloc(sizeof(Value) * count, type);
            impl = p.first;
            _index = p.second;
        }
        template <typename Iter>
        Buffer(const Iter &begin, const Iter &end) : _data(begin, end) {}
        Index index() const { return _index; }
        void copy_to_device() { impl->write(_data.data(), _data.size() * sizeof(Value), 0); }
        const std::vector<Value> &copy_to_host() {
            impl->read((uint8_t *)_data.data(), _data.size() * sizeof(Value), 0);
            return _data;
        }
        const std::vector<Value> &data() const { return _data; }
    };

    template <typename Value>
    class GPUArray {
        using Index = int32_t;
        Index _index;
        enum from_index_tag {};
        GPUArray(const Index &i, from_index_tag) : _index(i) {}
        Type type = get_type<Value>();
        size_t _size = 1;
        // std::optional<Buffer<Value>> _buffer;

      public:
        using Mask = GPUArray<bool>;
        Index index() const { return _index; }
        size_t size() const { return _size; }
        GPUArray(const Value v = Value()) {
            if constexpr (std::is_integral_v<Value>) {
                _index = nagisa_trace_append(Instruction::const_int(v), type);
            } else {
                _index = nagisa_trace_append(Instruction::const_float(v), type);
            }
        }
        static GPUArray from_index(const Index &i) { return GPUArray(i, from_index_tag{}); }
        GPUArray add_(const GPUArray &rhs) const {
            return from_index(nagisa_trace_append(Instruction::binary(FAdd, index(), rhs.index()), type));
        }
        static GPUArray range_(size_t count) {
            GPUArray a;
            a._size = count;
            a._index = (int)Predefined::ThreadIdx;
            nagisa_set_var_size(a._index, count);
            return a;
        }
        // template <size_t Stride>
        template <typename I, typename M>
        static void store(const GPUArray &buffer, const I &idx, const M &mask, const GPUArray &value) {
            (void)nagisa_trace_append(
                Instruction::store(buffer._buffer.value().index(), idx.index(), value.index(), mask.index()),
                Type::none);
        }
    };
    template <typename Value, typename Index, typename Mask>
    void store(const Buffer<Value> &buffer, const Index &idx, const Mask &m, const GPUArray<Value> &v) {
        GPUArray<Value>::store(buffer, idx, m, v);
    }

    template <typename Value>
    GPUArray<Value> range(size_t n) {
        return GPUArray<Value>::range_(n);
    }

    using Mask = GPUArray<bool>;

} // namespace nagisa