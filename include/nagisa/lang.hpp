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
#include <variant>
#include <magic_enum.hpp>
#include <unordered_set>
#include <iostream>
#include <functional>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <nagisa/ir.hpp>
#include <nagisa/letlist.hpp>
#include <nagisa/irbuilder.h>
#include <nagisa/backend.h>

// Embedded DSL
namespace nagisa::lang {

    // class Thunk {
    // public:
    //     Thunk(const Thunk&)=delete;
    //     Thunk(Thunk&&)=delete;
    //     Thunk&operator=(const Thunk&)=delete;
    //     Thunk&operaotr=(Thunk&&)=delete;
    // };
    struct mark_as_parameter {};

    struct VarBase {
        VarBase(const ir::Var v) : var(v) {}
        VarBase() = default;
        void _set_var(const ir::Var &v) { var = v; }
        const ir::Var &_get_var() { return var; }

      protected:
        ir::Var var;
    };
    template <typename T> struct Var : VarBase {
        using element_t = T;
        using VarBase::VarBase;
        template <typename U> friend struct Var;
        template <typename U> Var(const Var<U> &rhs) { set(rhs); }
        template <typename U> void set(const Var<U> &rhs) {
            using From = U;
            using To = T;
            if constexpr (std::is_same_v<From, int32_t>) {
                if constexpr (std::is_same_v<To, float>) {
                    var = ir::IRBuilder::get()->create_i2sp(rhs.var);
                } else if constexpr (std::is_same_v<To, double>) {
                    var = ir::IRBuilder::get()->create_i2dp(rhs.var);
                } else {
                    static_assert(false);
                }
            } else if constexpr (std::is_same_v<From, float>) {
                if constexpr (std::is_same_v<To, int32_t>) {
                    var = ir::IRBuilder::get()->create_sp2i(rhs.var);
                } else {
                    static_assert(false);
                }
            } else if constexpr (std::is_same_v<From, double>) {
                if constexpr (std::is_same_v<To, int32_t>) {
                    var = ir::IRBuilder::get()->create_dp2i(rhs.var);
                } else {
                    static_assert(false);
                }
            }
            var->type = ir::get_type_from_native<T>();
        }
        void _set_var(const ir::Var &v) {
            var = v;
            var->type = get_type();
        }
        static ir::Type get_type() { return ir::get_type_from_native<T>(); }
        explicit Var(mark_as_parameter) { var = ir::IRBuilder::get()->make_parameter(ir::get_type_from_native<T>()); }
        Var() {}
        Var(const T &v) {
            var = ir::IRBuilder::get()->add_constant(std::make_shared<ir::ConstantNode>(v));
            var->type = ir::get_type_from_native<T>();
        }

        template <typename U> auto add(const Var<U> &rhs) const {
            using R = decltype(std::declval<T>() + std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            if constexpr (std::is_floating_point_v<R>) {
                res._set_var(ir::IRBuilder::get()->create_fadd(converted_lhs.var, converted_rhs.var));
            } else {
                res._set_var(ir::IRBuilder::get()->create_iadd(converted_lhs.var, converted_rhs.var));
            }
            return res;
        }
        template <typename U> auto sub(const Var<U> &rhs) const {
            using R = decltype(std::declval<T>() - std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            if constexpr (std::is_floating_point_v<R>) {
                res._set_var(ir::IRBuilder::get()->create_fsub(converted_lhs.var, converted_rhs.var));
            } else {
                res._set_var(ir::IRBuilder::get()->create_isub(converted_lhs.var, converted_rhs.var));
            }
            return res;
        }

        template <typename U> auto mul(const Var<U> &rhs) const {
            using R = decltype(std::declval<T>() * std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            if constexpr (std::is_floating_point_v<R>) {
                res._set_var(ir::IRBuilder::get()->create_fmul(converted_lhs.var, converted_rhs.var));
            } else {
                res._set_var(ir::IRBuilder::get()->create_imul(converted_lhs.var, converted_rhs.var));
            }
            return res;
        }

        template <typename U> auto div(const Var<U> &rhs) const {
            using R = decltype(std::declval<T>() / std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            if constexpr (std::is_floating_point_v<R>) {
                res._set_var(ir::IRBuilder::get()->create_fdiv(converted_lhs.var, converted_rhs.var));
            } else {
                res._set_var(ir::IRBuilder::get()->create_idiv(converted_lhs.var, converted_rhs.var));
            }
            return res;
        }
        template <typename U> auto mod(const Var<U> &rhs) const {
            static_assert(std::is_integral_v<T> && std::is_integral_v<U>);
            using R = decltype(std::declval<T>() % std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            res._set_var(ir::IRBuilder::get()->create_imod(converted_lhs.var, converted_rhs.var));
            return res;
        }
        template <typename U> auto shl(const Var<U> &rhs) const {
            static_assert(std::is_integral_v<T> && std::is_integral_v<U>);
            using R = decltype(std::declval<T>() << std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            res._set_var(ir::IRBuilder::get()->create_shl(converted_lhs.var, converted_rhs.var));
            return res;
        }
        template <typename U> auto shr(const Var<U> &rhs) const {
            static_assert(std::is_integral_v<T> && std::is_integral_v<U>);
            using R = decltype(std::declval<T>() >> std::declval<U>());
            Var<R> converted_lhs = *this, converted_rhs = rhs;
            Var<R> res;
            res._set_var(ir::IRBuilder::get()->create_shr(converted_lhs.var, converted_rhs.var));
            return res;
        }
#define NGS_DEF_CMP_OPERATOR(op, name)                                                                                 \
    template <typename U> Var<bool> name(const Var<U> &rhs) const {                                                    \
        using R = decltype(std::declval<T>() + std::declval<U>());                                                     \
        Var<R> converted_lhs = *this, converted_rhs = rhs;                                                             \
        Var<bool> res;                                                                                                 \
        if constexpr (std::is_floating_point_v<R>) {                                                                   \
            res._set_var(ir::IRBuilder::get()->create_f##name(converted_lhs.var, converted_rhs.var));                  \
        } else {                                                                                                       \
            res._set_var(ir::IRBuilder::get()->create_i##name(converted_lhs.var, converted_rhs.var));                  \
        }                                                                                                              \
        return res;                                                                                                    \
    }                                                                                                                  \
    template <typename U> auto operator op(const Var<U> &rhs) const { return name(rhs); }                              \
    auto operator op(const Var &rhs) const { return name(rhs); }
        NGS_DEF_CMP_OPERATOR(<, cmplt)
        NGS_DEF_CMP_OPERATOR(<=, cmple)
        NGS_DEF_CMP_OPERATOR(>, cmpgt)
        NGS_DEF_CMP_OPERATOR(>=, cmpge)
        NGS_DEF_CMP_OPERATOR(==, cmpeq)
        NGS_DEF_CMP_OPERATOR(!=, cmpne)

#define NGS_COMPUTE_DEF_OPERATOR(op, assign_op, name)                                                                  \
    template <typename U> auto operator op(const Var<U> &rhs) const { return name(rhs); }                              \
    auto operator op(const Var &rhs) const { return name(rhs); }                                                       \
    template <typename U> Var &operator assign_op(const Var<U> &rhs) {                                                 \
        *this = *this op rhs;                                                                                          \
        return *this;                                                                                                  \
    }                                                                                                                  \
    Var &operator assign_op(const Var &rhs) {                                                                          \
        *this = *this op rhs;                                                                                          \
        return *this;                                                                                                  \
    }                                                                                                                  \
    friend auto operator op(const T &v, const Var &rhs) { return Var(v) + rhs; }
        NGS_COMPUTE_DEF_OPERATOR(+, +=, add)
        NGS_COMPUTE_DEF_OPERATOR(-, -=, sub)
        NGS_COMPUTE_DEF_OPERATOR(*, *=, mul)
        NGS_COMPUTE_DEF_OPERATOR(/, /=, div)
    };

    template <typename F, typename T, typename... Ts> void apply_w_index(F &&f, size_t idx, T &first, Ts &&... args) {
        f(first, idx);
        if constexpr (sizeof...(args) > 0)
            apply_w_index(f, idx + 1, args...);
    }
    /*

    Function<float32(float32)>f = [](float32)->float32{...};
    float (*fp)(float) = f->compile();


    */
    template <class Ret, class... Args> struct Function {};
    template <class Ret, class... Args> struct Function<Var<Ret>(Var<Args>...)> {
        using func_t = Var<Ret> (*)(Var<Args>...);
        using native_func_t = Ret (*)(Args...);
        Function(func_t func) {
            auto fb = std::make_shared<ir::FunctionBlock>();
            auto args = std::make_tuple(Var<Args>(mark_as_parameter{})...);
            fb->parameters.resize(sizeof...(Args));
            std::apply(
                [&](auto &&... args) {
                    apply_w_index([&](auto &&arg, size_t idx) { fb->parameters[idx] = arg._get_var(); }, 0u, args...);
                },
                args);

            ir::IRBuilder::get()->set_func_block(fb);
            std::apply(
                [&](auto &&... args) {
                    apply_w_index(
                        [&](auto &&arg, size_t idx) {
                            if constexpr (std::is_aggregate_v<typename std::decay_t<decltype(arg)>::element_t>) {
                                arg.unpack();
                            }
                        },
                        0u, args...);
                },
                args);
            auto body = std::apply(func, args);
            if constexpr (std::is_aggregate_v<decltype(body)::element_t>) {
                body.pack();
            }
            ir::IRBuilder::get()->create_ret(body._get_var());
            function = fb->get_func_node();
        }
        native_func_t compile(const std::shared_ptr<Backend> &backend) {
            return (native_func_t)backend->compile(function);
        }
        const ir::Function &__get_func_node() const { return function; }

      private:
        ir::Function function;
    };

    template <typename T, typename U = typename T::element_t>
    Var<U> select(const Var<bool> &cond, const T &a, const T &b) {
        return Var<U>(ir::IRBuilder::get()->create_select(cond._get_var(), Var<U>(a)._get_var(), Var<U>(b)._get_var()));
    }
    template <class T, class M> M get_member_type(M T::*);

#define NGS_STRUCT_FIELD(r, data, Field)      Var<decltype(get_member_type(&CStruct::Field))> Field;
#define NGS_STRUCT_FIELD_REG_TYPE(r, data, f) st->append(#f, decltype(f)::get_type());
#define NGS_STRUCT_FIELD_UNPACK(r, data, f)                                                                            \
    f._set_var(ir::IRBuilder::get()->create_get_element(var, idx));                                                    \
    idx++;
#define NGS_STRUCT_FIELD_PACK(r, data, f)                                                                              \
    var = ir::IRBuilder::get()->create_store_element(var, idx, f._get_var());                                          \
    idx++;

#define NGS_TSTRUCT(Name, ...) NGS_STRUCT(Name, __VA_ARGS__)

#define NGS_STRUCT(Name, ...)                                                                                          \
    template<> struct Var<Name> : VarBase {                                                                                       \
        using element_t = Name;                                                                                        \
        using CStruct = Name;                                                                                          \
        using VarBase::VarBase;                                                                                        \
        BOOST_PP_SEQ_FOR_EACH(NGS_STRUCT_FIELD, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                              \
        NGS_STRUCT_COMMON(Name, __VA_ARGS__)                                                                           \
    };
#define NGS_STRUCT_COMMON(Name, ...)                                                                                   \
    static ir::StructType get_type() {                                                                                 \
        auto name = typeid(Name).name();                                                                               \
        if (auto ty = ir::get_struct_type(name)) {                                                                     \
            return ty;                                                                                                 \
        }                                                                                                              \
        auto st = std::make_shared<ir::StructTypeNode>();                                                              \
        st->name = #Name;                                                                                              \
        BOOST_PP_SEQ_FOR_EACH(NGS_STRUCT_FIELD_REG_TYPE, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__));                    \
        ir::register_struct_type(name, st);                                                                            \
        return st;                                                                                                     \
    }                                                                                                                  \
    void unpack() {                                                                                                    \
        int idx = 0;                                                                                                   \
        BOOST_PP_SEQ_FOR_EACH(NGS_STRUCT_FIELD_UNPACK, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__));                      \
    }                                                                                                                  \
    void pack() {                                                                                                      \
        int idx = 0;                                                                                                   \
        if (!var) {                                                                                                    \
            var = ir::IRBuilder::get()->add_undef_struct(get_type());                                                  \
            var->type = get_type();                                                                                    \
        }                                                                                                              \
        BOOST_PP_SEQ_FOR_EACH(NGS_STRUCT_FIELD_PACK, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__));                        \
    }                                                                                                                  \
    void _set_var(const ir::Var &v) {                                                                                  \
        var = v;                                                                                                       \
        var->type = get_type();                                                                                        \
    }                                                                                                                  \
    explicit Var(mark_as_parameter) { var = ir::IRBuilder::get()->make_parameter(get_type()); }

    using boolean = Var<bool>;
    using int32 = Var<int32_t>;
    using float32 = Var<float>;
    using float64 = Var<double>;
} // namespace nagisa::lang