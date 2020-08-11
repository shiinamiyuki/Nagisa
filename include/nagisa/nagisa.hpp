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
#include <algorithm>
#include <optional>
#include <array>
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
    void nagisa_inc_int(int idx);
    void nagisa_dec_int(int idx);
    void nagisa_inc_ext(int idx);
    void nagisa_dec_ext(int idx);
    int nagisa_buffer_id(int idx);
    void nagisa_copy_to_host(int idx, void *);
    int nagisa_ref_ext(int idx);

    template <typename Value>
    constexpr Type get_type() {
        if constexpr (std::is_same_v<Value, bool>) {
            return Type::boolean;
        } else if constexpr (std::is_same_v<Value, int32_t>) {
            return Type::i32;
        } else if constexpr (std::is_same_v<Value, float>) {
            return Type::f32;
        } else if constexpr (std::is_same_v<Value, double>) {
            return Type::f32;
        } else {
            return Type::none;
        }
    }

    inline size_t get_typesize(Type type) {
        switch (type) {
        case Type::i32:
        case Type::f32:
            return 4;
        case Type::boolean:
            return 1;
        }
        NGS_ASSERT(false);
    }
    enum Opcode {
        Select,
        FAdd,
        FSub,
        FMul,
        FDiv,
        Mod,
        CmpLt,
        CmpLe,
        CmpGe,
        CmpGt,
        CmpEq,
        CmpNe,
        ConstantInt,
        ConstantFloat,
        Store,
        Load,
        Sin,
        Cos,
        Sqrt
    };
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
        std::array<int, 3> deps = {-1};
        static Instruction ternary(Opcode op, int a, int b, int c) {
            Instruction i;
            i.op = op;
            i.operand[0] = a;
            i.operand[1] = b;
            i.operand[2] = c;
            i.deps[0] = a;
            i.deps[1] = b;
            i.deps[2] = c;
            return i;
        }
        static Instruction binary(Opcode op, int a, int b) {
            Instruction i;
            i.op = op;
            i.operand[0] = a;
            i.operand[1] = b;
            i.deps[0] = a;
            i.deps[1] = b;
            return i;
        }
        static Instruction unary(Opcode op, int a) {
            Instruction i;
            i.op = op;
            i.operand[0] = a;
            i.deps[0] = a;
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
            i.deps[0] = idx;
            i.deps[1] = value;
            i.deps[2] = mask;
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

    struct Index {
        int32_t i = -1;
        Index(int32_t i = -1) : i(i) {
            if (i >= 0) {
                nagisa_inc_ext(i);
            }
        }
        Index(const Index &rhs) : i(rhs.i) {
            if (i >= 0) {
                nagisa_inc_ext(i);
            }
        }
        Index(Index &&rhs) : i(rhs.i) { rhs.i = -1; }
        Index &operator=(Index &&rhs) {
            if (i >= 0) {
                nagisa_dec_ext(i);
            }

            i = rhs.i;
            rhs.i = -1;
            return *this;
        }
        Index &operator=(const Index &rhs) {
            if (&rhs == this)
                return *this;
            if (i >= 0) {
                nagisa_dec_ext(i);
            }
            i = rhs.i;
            if (i >= 0) {
                nagisa_inc_ext(i);
            }
            return *this;
        }
        operator int() const { return i; }
        ~Index() {
            if (i >= 0) {
                nagisa_dec_ext(i);
            }
        }
    };
    /*
    A GPUArray holds an index to the SSA value used in backend
    */
    template <typename Value>
    class GPUArray {
      public:
        template <typename _>
        friend class GPUArray;
        Index _index;
        enum from_index_tag {};
        GPUArray(const Index &i, size_t sz, from_index_tag) : _index(i), _size(sz) {}

        size_t _size = 1;
        // std::optional<Buffer<Value>> _buffer;
        mutable std::vector<Value> _buffer;
        mutable bool need_sync = false;

      public:
        static const Type type = get_type<Value>();
        using Mask = GPUArray<bool>;
        const Index &index() const { return _index; }
        size_t size() const { return _size; }
        GPUArray(const Value v = Value()) : GPUArray(v, 1) {}
        GPUArray(const Value v, size_t s) : _size(s) {
            if constexpr (std::is_integral_v<Value>) {
                _index = nagisa_trace_append(Instruction::const_int(v), type);
            } else {
                _index = nagisa_trace_append(Instruction::const_float(v), type);
            }
        }
        template <typename U>
        GPUArray(const GPUArray<U> &rhs) {
            _size = rhs._size;
            _index = rhs._index;
        }
        GPUArray(const GPUArray &other) : _index(other._index), _size(other._size) {}
        GPUArray &operator=(const GPUArray &other) {
            _index = (other._index);
            _size = (other._size);
            return *this;
        }
        static GPUArray from_index(const Index &i, size_t sz) { return GPUArray(i, sz, from_index_tag{}); }
        template <typename U>
        size_t check_size(const GPUArray<U> &rhs) const {
            NGS_ASSERT((_size == 1 || rhs.size() == 1) || (_size == rhs.size()));
            return std::max(_size, rhs._size);
        }
        GPUArray add_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a = from_index(nagisa_trace_append(Instruction::binary(FAdd, index(), rhs.index()), type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray sub_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a = from_index(nagisa_trace_append(Instruction::binary(FSub, index(), rhs.index()), type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray mul_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a = from_index(nagisa_trace_append(Instruction::binary(FMul, index(), rhs.index()), type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray div_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a = from_index(nagisa_trace_append(Instruction::binary(FDiv, index(), rhs.index()), type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray mod_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a = from_index(nagisa_trace_append(Instruction::binary(Mod, index(), rhs.index()), type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        Mask lt_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a =
                Mask::from_index(nagisa_trace_append(Instruction::binary(CmpLt, index(), rhs.index()), Mask::type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        Mask le_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a =
                Mask::from_index(nagisa_trace_append(Instruction::binary(CmpLe, index(), rhs.index()), Mask::type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray<bool> gt_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a =
                Mask::from_index(nagisa_trace_append(Instruction::binary(CmpGt, index(), rhs.index()), Mask::type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray<bool> ge_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a =
                Mask::from_index(nagisa_trace_append(Instruction::binary(CmpGe, index(), rhs.index()), Mask::type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray<bool> eq_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a =
                Mask::from_index(nagisa_trace_append(Instruction::binary(CmpEq, index(), rhs.index()), Mask::type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray<bool> ne_(const GPUArray &rhs) const {
            auto sz = check_size(rhs);
            auto a =
                Mask::from_index(nagisa_trace_append(Instruction::binary(CmpNe, index(), rhs.index()), Mask::type), sz);
            nagisa_set_var_size(a.index(), sz);
            return a;
        }
        GPUArray operator-() const { return GPUArray(Value(-1)) * (*this); }
        friend GPUArray sin(const GPUArray &v) {
            auto a = GPUArray::from_index(nagisa_trace_append(Instruction::unary(Sin, v.index()), GPUArray::type), sz);
            nagisa_set_var_size(a.index(), v.size());
            return a;
        }
        friend GPUArray cos(const GPUArray &v) {
            auto a = GPUArray::from_index(nagisa_trace_append(Instruction::unary(Cos, v.index()), GPUArray::type), v.size());
            nagisa_set_var_size(a.index(), v.size());
            return a;
        }
        friend GPUArray sqrt(const GPUArray &v) {
            auto a = GPUArray::from_index(nagisa_trace_append(Instruction::unary(Sqrt, v.index()), GPUArray::type), v.size());
            nagisa_set_var_size(a.index(), v.size());
            return a;
        }
        static GPUArray select_(const Mask &cond, const GPUArray &a, const GPUArray &b) {
            auto sz = a.check_size(cond);
            NGS_ASSERT(sz == b.check_size(cond));
            auto o = from_index(
                nagisa_trace_append(Instruction::ternary(Select, cond.index(), a.index(), b.index()), a.type), sz);
            nagisa_set_var_size(o.index(), sz);
            return o;
        }
        static GPUArray range_(size_t count) {
            GPUArray a;
            a._size = count;
            a._index = (int)Predefined::ThreadIdx;
            nagisa_set_var_size(a._index, count);
            return a;
        }
        // template <size_t Stride>
        // template <typename I, typename M>
        // static void store(const GPUArray &buffer, const I &idx, const M &mask, const GPUArray &value) {
        //     (void)nagisa_trace_append(
        //         Instruction::store(nagisa_buffer_id(buffer.index()), idx.index(), value.index(), mask.index()),
        //         Type::none);
        // }

        template <typename I>
        GPUArray load(const Mask &mask, const GPUArray<I> &index) {
            auto a = from_index(
                nagisa_trace_append(Instruction::ternary(Load, value.index(), mask.index(), index.index()), type));
            a._size = index.size();
            nagisa_set_var_size(a.index(), index.size());
            return a;
        }
        void sync() const {
            _buffer.resize(_size);
            nagisa_copy_to_host(index(), _buffer.data());
        }
        const std::vector<Value> &data() const {
            sync();
            return _buffer;
        }
    };
    template <typename Value, typename Index, typename Mask>
    void store(const GPUArray<Value> &buffer, const Index &idx, const Mask &m, const GPUArray<Value> &v) {
        GPUArray<Value>::store(buffer, idx, m, v);
    }

    template <class Array>
    Array range(size_t n) {
        return Array::range_(n);
    }

    using Mask = GPUArray<bool>;
    template <class Array>
    Array select(const Mask &cond, const Array &a, const Array &b) {
        return Array::select_(cond, a, b);
    }
    template <typename T>
    struct is_array : std::false_type {};
    template <typename T>
    struct is_array<GPUArray<T>> : std::true_type {};
#define NGS_OP(op, func)                                                                                               \
    template <typename T1, typename T2>                                                                                \
    auto operator op(const GPUArray<T1> &a, const GPUArray<T2> &b) {                                                   \
        using R = decltype(std::declval<T1>() op std::declval<T2>());                                                  \
        GPUArray<R> r = a.func(b);                                                                                     \
        return r;                                                                                                      \
    }                                                                                                                  \
    template <typename T1, typename T2, typename = std::enable_if_t<!is_array<T2>::value>>                             \
    auto operator op(const GPUArray<T1> &a, const T2 &b) {                                                             \
        using R = decltype(std::declval<T1>() op std::declval<T2>());                                                  \
        GPUArray<R> r = a.func(GPUArray<T1>(b));                                                                       \
        return r;                                                                                                      \
    }                                                                                                                  \
    template <typename T1, typename T2, typename = std::enable_if_t<!is_array<T1>::value>>                             \
    auto operator op(const T1 &a, const GPUArray<T2> &b) {                                                             \
        using R = decltype(std::declval<T1>() op std::declval<T2>());                                                  \
        GPUArray<R> r = GPUArray<T1>(a).func(GPUArray<T2>(b));                                                         \
        return r;                                                                                                      \
    }
    NGS_OP(+, add_)
    NGS_OP(-, sub_)
    NGS_OP(*, mul_)
    NGS_OP(/, div_)
    NGS_OP(<, lt_)
    NGS_OP(<=, le_)
    NGS_OP(>, gt_)
    NGS_OP(>=, ge_)
    NGS_OP(==, eq_)
    NGS_OP(!=, ne_)
    NGS_OP(%, mod_)
} // namespace nagisa