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

#include <iostream>

#include <nagisa/platform.h>
#include <nagisa/debug.h>
#include <magic_enum.hpp>

namespace nagisa::debug {
    using namespace ir;
    NAGISA_API std::string to_text(const ir::Node &node) {
        struct DumpVisitor {
            std::string out;
            std::string dump_type(const Type &ty) {
                if (ty->isa<PrimitiveType>()) {
                    return std::string(magic_enum::enum_name(ty->cast<PrimitiveType>()->prim));
                }
                if (ty->isa<StructType>()) {
                    return ty->cast<StructType>()->name;
                }
                return "unknown";
            }
            void emit(int level, const std::string &s) { out.append(std::string(level, ' ')).append(s); }
            void emit(const std::string &s) { out.append(s); }
            void recurse(const Node &node, int level) {
                if (node->isa<Constant>()) {
                    auto val = node->cast<Constant>()->value();
                    std::visit(
                        [=](auto &&arg) {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> ||
                                          std::is_same_v<T, double>) {
                                emit(std::to_string(arg));
                            } else {
                                emit("#constant");
                            }
                        },
                        val);
                } else if (node->isa<Primitive>()) {
                    emit(std::string(magic_enum::enum_name(node->cast<Primitive>()->primitive())).data());
                } else if (node->isa<Var>()) {
                    emit(std::string("%") + node->cast<Var>()->name());
                } else if (node->isa<Function>()) {
                    emit("fn (");
                    auto func = node->cast<Function>();
                    for (auto &i : func->parameters()) {
                        recurse(i, level);
                        emit(" ");
                    }
                    emit("){\n");
                    recurse(func->body(), level + 1);
                    emit("\n");
                    emit(level, "}");
                } else if (node->isa<Call>()) {
                    auto cnode = node->cast<Call>();
                    recurse(cnode->op(), level);
                    emit("(");
                    for (size_t i = 0; i < cnode->args().size(); i++) {
                        recurse(cnode->args()[i], level);
                        if (i + 1 != cnode->args().size()) {
                            emit(", ");
                        }
                    }
                    emit(")");
                } else if (node->isa<Select>()) {
                    auto if_ = node->cast<Select>();
                    emit("select(");
                    recurse(if_->cond(), level);
                    emit(", ");
                    recurse(if_->then(), level++);
                    emit(", ");
                    recurse(if_->else_(), level++);
                    emit(")");
                } else if (node->isa<Let>()) {
                    auto let = node->cast<Let>();
                    emit(level, "let ");
                    recurse(let->var(), level);
                    emit(level, " = ");
                    recurse(let->value(), level);
                    emit(std::string(" : ").append(dump_type(let->var()->type)).append("\n"));
                    if (!let->body()->isa<Let>()) {
                        emit(level, "");
                    }
                    recurse(let->body(), level);
                } else if (node->isa<UndefStruct>()) {
                    emit(std::string("decl ").append(dump_type(node->type)));
                } else if (node->isa<LoadField>()) {
                    auto load = node->cast<LoadField>();
                    emit("load field ");
                    recurse(load->aggregate, level);
                    emit(std::string(" ").append(std::to_string(load->idx)));
                } else if (node->isa<StoreField>()) {
                    auto store = node->cast<StoreField>();
                    emit("store field ");
                    recurse(store->aggregate, level);
                    emit(std::string(" ").append(std::to_string(store->idx)));
                    recurse(store->val, level);
                } else {
                    if (!node) {
                        emit("NULL!");
                    } else {
                        std::cerr << "unknown node " << node->type_name() << std::endl;
                        NAGISA_ASSERT(false);
                    }
                }
            }
        };
        DumpVisitor vis;
        vis.recurse(node, 0);
        return vis.out;
    }
} // namespace nagisa::debug