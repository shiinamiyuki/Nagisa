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
#include <algorithm>
#include <nagisa/nagisa.hpp>
#include <cl/cl.hpp>
#include <list>
#include <sstream>
#include <iostream>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>
namespace nagisa {
    template <size_t DEFAULT_BLOCK_SIZE = 262144ull>
    class MemoryArena {
        static constexpr size_t align16(size_t x) { return (x + 15ULL) & (~15ULL); }

        struct Block {
            size_t size;
            uint8_t *data;

            Block(uint8_t *data, size_t size) : size(size), data(data) {
                //                log::log("???\n");
            }

            ~Block() = default;
        };

        std::list<Block> availableBlocks, usedBlocks;

        size_t currentBlockPos = 0;
        Block currentBlock;

      public:
        MemoryArena() : currentBlock(new uint8_t[DEFAULT_BLOCK_SIZE], DEFAULT_BLOCK_SIZE) {}

        uint8_t *alloc(size_t size) {
            typename std::list<Block>::iterator iter;
            auto allocSize = size;

            uint8_t *p = nullptr;
            if (currentBlockPos + allocSize > currentBlock.size) {
                usedBlocks.emplace_front(currentBlock);
                currentBlockPos = 0;
                for (iter = availableBlocks.begin(); iter != availableBlocks.end(); iter++) {
                    if (iter->size >= allocSize) {
                        currentBlockPos = allocSize;
                        currentBlock = *iter;
                        availableBlocks.erase(iter);
                        break;
                    }
                }
                if (iter == availableBlocks.end()) {
                    auto sz = std::max<size_t>(allocSize, DEFAULT_BLOCK_SIZE);
                    currentBlock = Block(new uint8_t[sz], sz);
                }
            }
            p = currentBlock.data + currentBlockPos;
            currentBlockPos += allocSize;
            return p;
        }

        void reset() {
            currentBlockPos = 0;
            availableBlocks.splice(availableBlocks.begin(), usedBlocks);
        }

        ~MemoryArena() {
            delete[] currentBlock.data;
            for (auto i : availableBlocks) {
                delete[] i.data;
            }
            for (auto i : usedBlocks) {
                delete[] i.data;
            }
        }
    };

    struct OCLContext {
        cl::Platform platform;
        cl::Device device;
        cl::Context context;
        cl::CommandQueue queue;
        OCLContext() {
            std::vector<cl::Platform> all_platforms;
            cl::Platform::get(&all_platforms);

            if (all_platforms.size() == 0) {
                std::cout << " No platforms found. Check OpenCL installation!\n";
                exit(1);
            }
            cl::Platform default_platform = all_platforms[0];
            std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";
            platform = default_platform;
            std::vector<cl::Device> all_devices;
            default_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
            if (all_devices.size() == 0) {
                std::cout << " No devices found. Check OpenCL installation!\n";
                exit(1);
            }

            cl::Device default_device = all_devices[0];
            std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";
            device = default_device;
            context = cl::Context({device});
            queue = cl::CommandQueue(context, device);
        }
    };
    static std::unique_ptr<OCLContext> ocl_ctx = nullptr;
    struct Value {
        Instruction inst;
        Type type;
        int idx = 0;
        int buf_idx = -1;
        size_t size = 1;
        int _ref_int = 0;
        int _ref_ext = 0;
        int _last_sync_time = -1;
    };
    class Context {
      public:
        int _time = 0;
        size_t cur_var = -1;
        size_t cur_size = 1;
        std::unordered_set<int> live;
        std::unordered_map<size_t, Value> vars;
        std::map<int, std::unique_ptr<DeviceBuffer>> buffers;
        std::unordered_map<std::string, cl::Program> kernel_cache;
        MemoryArena<> arena;
    };
    static std::unique_ptr<Context> ctx = nullptr;
    void nagisa_add_predefined();
    void nagisa_init() {
        ctx = std::make_unique<Context>();
        ocl_ctx = std::make_unique<OCLContext>();
    }
    void nagisa_destroy() {
        ocl_ctx = nullptr;
        ctx = nullptr;
    }
    void nagisa_set_var_size(int idx, size_t s) {
        NGS_ASSERT(ctx->vars.at(idx).buf_idx == -1);
        ctx->vars.at(idx).size = s;
    }
    int nagisa_buffer_id(int idx) {
        // NGS_ASSERT(ctx->vars.at(idx).buf_idx != -1);
        return ctx->vars.at(idx).buf_idx;
    }
    void nagisa_inc_int(int idx) {
        if (idx < (int)Predefined::Total)
            return;
        ctx->vars.at(idx)._ref_int++;
    }
    void nagisa_dec_int(int idx) {
        if (idx < (int)Predefined::Total)
            return;
        ctx->vars.at(idx)._ref_int--;
    }
    void nagisa_inc_ext(int idx) {
        if (idx < (int)Predefined::Total)
            return;
        ctx->vars.at(idx)._ref_ext++;
    }
    void nagisa_dec_ext(int idx) {
        if (idx < (int)Predefined::Total)
            return;
        NGS_ASSERT(ctx->vars.at(idx)._ref_ext > 0);
        ctx->vars.at(idx)._ref_ext--;
        if (ctx->vars.at(idx)._ref_ext == 0) {
            ctx->live.erase(idx);
        }
    }
    int nagisa_ref_ext(int idx) { return ctx->vars.at(idx)._ref_ext; }
    void nagisa_add_predefined() {
        auto &vars = ctx->vars;
        NGS_ASSERT(vars.empty());
        Value vs[(int)Predefined::Total];
        vs[(int)Predefined::ThreadIdx].idx = (int)Predefined::ThreadIdx;
        vs[(int)Predefined::ThreadIdx].type = Type::i32;
        for (int i = 0; i < (int)Predefined::Total; i++) {
            vars[i] = vs[i];
        }
        ctx->cur_var = (int)Predefined::Total - 1;
    }
    class OCLBuffer : public DeviceBuffer {
        cl::Buffer buffer;

      public:
        size_t _size;
        OCLBuffer(Type type, size_t s) : DeviceBuffer(type), _size(s), buffer(ocl_ctx->context, CL_MEM_READ_WRITE, s) {}
        size_t size() { return _size; }
        void write(const uint8_t *p, size_t bytes, size_t offset) {
            ocl_ctx->queue.enqueueWriteBuffer(buffer, CL_TRUE, offset, bytes, p);
        }
        void read(uint8_t *p, size_t bytes, size_t offset) {
            ocl_ctx->queue.enqueueReadBuffer(buffer, CL_TRUE, offset, bytes, p);
        }
        void *get() override { return buffer(); }
    };
    std::pair<DeviceBuffer *, int32_t> nagisa_alloc(size_t s, Type type) {
        auto buffer = std::make_unique<OCLBuffer>(type, s);
        auto p = buffer.get();
        ctx->buffers.emplace(ctx->buffers.size(), std::move(buffer));
        return {p, (int)ctx->buffers.size() - 1};
    }
    void nagisa_free(DeviceBuffer *) {}
    int nagisa_trace_append(const Instruction &i, Type type) {
        if (ctx->vars.empty()) {
            nagisa_add_predefined();
        }
        ++ctx->cur_var;
        Value v;
        v.inst = i;
        v.idx = (int)ctx->cur_var;
        v.type = type;
        ctx->vars.emplace(ctx->cur_var, v);
        ctx->live.insert(v.idx);
        return v.idx;
    }
    std::string type_to_str(Type type) {
        if (type == Type::f32) {
            return "float";
        }
        if (type == Type::i32) {
            return "int";
        }
        if (type == Type::boolean) {
            return "bool";
        }
        std::cerr << "unknown type" << std::endl;
        std::abort();
    }
    void scan_traces(std::unordered_set<int32_t> &visited, std::vector<int> &trace, int idx) {
        if (visited.find(idx) != visited.end()) {
            return;
        }
        auto &v = ctx->vars.at(idx);
        if (v.idx >= (int)Predefined::Total && v._last_sync_time != -1) {
            // v is from last kernel launch
            return;
        }
        visited.insert(idx);
        std::array<int, 3> deps = v.inst.deps;
        for (auto k : deps) {
            if (k >= 0) {
                scan_traces(visited, trace, k);
            }
        }
        trace.push_back(idx);
    }
    std::string nagisa_generate_kernel_trace(std::unordered_map<int, std::string> &to_var,
                                             const std::vector<int> &trace) {
        std::ostringstream out, kernel;
        int _var_cnt = 0;
        ctx->cur_size = 1;
        for (auto idx : trace) {
            auto &v = ctx->vars.at(idx);
            // if (v.inst.op == Store) {
            //     auto st = v.inst.store_inst;
            //     // clang-format off
            //     out << "if(" << to_var.at(st.mask) << "){" \
            //         << "buffer" << st.buffer_id \
            //         << "[" << to_var.at(st.idx) << "] = " << to_var.at(st.value) << ";}\n";
            //     // clang-format on
            //     continue;
            // }
            for (auto dep : v.inst.deps) {
                if (dep >= (int)Predefined::Total) {
                    auto &u = ctx->vars.at(dep);
                    if (u._last_sync_time >= 0 && u._last_sync_time < ctx->_time && to_var.find(dep) == to_var.end()) {
                        std::string var = std::string("v").append(std::to_string(_var_cnt++));
                        to_var[dep] = var;
                        out << type_to_str(u.type) << " " << var << " = buffer" << u.buf_idx << "[get_global_id(0)];\n";
                    }
                }
            }
            ctx->cur_size = std::max(ctx->cur_size, v.size);
            std::string var = std::string("v").append(std::to_string(_var_cnt++));
            out << type_to_str(v.type) << " " << var << " = ";
            to_var[v.idx] = var;
            if (v.idx < (int)Predefined::Total) {
                if (v.idx == 0) {
                    out << "get_global_id(0)";
                }
            } else {
                auto op = v.inst.op;
                if (op == ConstantInt) {
                    out << v.inst.ival;
                } else if (op == ConstantFloat) {
                    out << v.inst.fval;
                } else if (op == FAdd) {
                    out << to_var.at(v.inst.operand[0]) << " + " << to_var.at(v.inst.operand[1]);
                } else if (op == FSub) {
                    out << to_var.at(v.inst.operand[0]) << " - " << to_var.at(v.inst.operand[1]);
                } else if (op == FMul) {
                    out << to_var.at(v.inst.operand[0]) << " * " << to_var.at(v.inst.operand[1]);
                } else if (op == FDiv) {
                    out << to_var.at(v.inst.operand[0]) << " / " << to_var.at(v.inst.operand[1]);
                } else if (op == Mod) {
                    out << to_var.at(v.inst.operand[0]) << " % " << to_var.at(v.inst.operand[1]);
                } else if (op == CmpLt) {
                    out << to_var.at(v.inst.operand[0]) << " < " << to_var.at(v.inst.operand[1]);
                } else if (op == CmpLe) {
                    out << to_var.at(v.inst.operand[0]) << " <= " << to_var.at(v.inst.operand[1]);
                } else if (op == CmpGt) {
                    out << to_var.at(v.inst.operand[0]) << " > " << to_var.at(v.inst.operand[1]);
                } else if (op == CmpGe) {
                    out << to_var.at(v.inst.operand[0]) << " >= " << to_var.at(v.inst.operand[1]);
                } else if (op == CmpEq) {
                    out << to_var.at(v.inst.operand[0]) << " == " << to_var.at(v.inst.operand[1]);
                } else if (op == CmpNe) {
                    out << to_var.at(v.inst.operand[0]) << " != " << to_var.at(v.inst.operand[1]);
                } else if (op == Select) {
                    out << to_var.at(v.inst.operand[0]) << " ? " << to_var.at(v.inst.operand[1]) << " :"
                        << to_var.at(v.inst.operand[2]);
                } else if (op == Load) {
                    out << to_var.at(v.inst.operand[1]) << " ? " << to_var.at(v.inst.operand[0]) << "["
                        << to_var.at(v.inst.operand[2]) << "] : 0";
                } else if (op == Sin) {
                    out << "sin(" << to_var.at(v.inst.operand[0]) << ")";
                } else if (op == Cos) {
                    out << "cos(" << to_var.at(v.inst.operand[0]) << ")";
                } else if (op == Sqrt) {
                    out << "sqrt(" << to_var.at(v.inst.operand[0]) << ")";
                } else {
                    NGS_ASSERT(false);
                }
            }
            out << ";\n";
            if (v.size != 1 && v._ref_ext > 0) {
                if (v.buf_idx == -1) {
                    auto [_, buf_id] = nagisa_alloc(v.size * get_typesize(v.type), v.type);
                    v.buf_idx = buf_id;
                }
                out << "buffer" << v.buf_idx << "["
                    << "get_global_id(0)"
                    << "] = " << to_var.at(v.idx) << ";\n";
                v._last_sync_time = ctx->_time;
            }
        }
        kernel << "__kernel void main(";
        {
            for (auto &p : ctx->buffers) {
                auto i = p.first;
                auto &buf = p.second;
                kernel << "__global " << type_to_str(buf->type) << " * buffer" << i;
                if (i != ctx->buffers.size() - 1) {
                    kernel << ", ";
                }
            }
            kernel << "){\n";
        }
        kernel << out.str();
        kernel << "}";
        return kernel.str();
    }
    void nagisa_run_kernel(size_t size, const std::string &kernel_src) {
        std::cout << "kernel:\n" << kernel_src << std::endl;
        auto it = ctx->kernel_cache.find(kernel_src);
        cl::Program *program = nullptr;
        if (it != ctx->kernel_cache.end()) {
            std::cout << "hit!" << std::endl;
            program = &it->second;
        } else {
            cl::Program::Sources sources;
            sources.push_back({kernel_src.c_str(), kernel_src.length()});
            cl::Program p(ocl_ctx->context, sources);
            if (p.build({ocl_ctx->device}) != CL_SUCCESS) {
                std::cerr << "Error building: " << p.getBuildInfo<CL_PROGRAM_BUILD_LOG>(ocl_ctx->device) << std::endl;
                exit(1);
            }
            ctx->kernel_cache.emplace(kernel_src, p);
            program = &ctx->kernel_cache[kernel_src];
        }

        cl::Kernel kernel(*program, "main");
        for (auto &p : ctx->buffers) {
            kernel.setArg((cl_uint)p.first, p.second->get());
        }
        std::cout << "kernel launch with size: " << size << std::endl;
        ocl_ctx->queue.enqueueNDRangeKernel(kernel, cl::NDRange(0), cl::NDRange(size));
        ocl_ctx->queue.finish();
    }
    void nagisa_free_var(int i);
    void nagisa_eval() {
        if (ctx->live.empty())
            return;
        std::map<size_t, std::pair<std::unordered_set<int>, std::vector<int>>> traces;
        for (auto idx : ctx->live) {
            auto &rec = traces[ctx->vars.at(idx).size];
            scan_traces(rec.first, rec.second, idx);
        }
        ctx->live.clear();
        std::unordered_map<int, std::string> to_var;
        for (auto it = traces.begin(); it != traces.end(); it++) {
            auto &trace = it->second.second;
            auto knl = nagisa_generate_kernel_trace(to_var, trace);
            nagisa_run_kernel(ctx->cur_size, knl);
        }
        for (auto it = ctx->vars.begin(); it != ctx->vars.end();) {
            if (it->first < (int)Predefined::Total) {
                it++;
                continue;
            }

            bool need_remove = false;
            // if (it->second._last_sync_time == -1) {
            //     // not synced to global memory
            //     // is lost permanently after kernel launch
            //     need_remove = true;
            // }
            if (it->second._ref_ext == 0) {
                // no need keep the variable
                need_remove = true;
            }
            if (need_remove) {
                nagisa_free_var(it->first);
                it = ctx->vars.erase(it);
            } else {
                it++;
            }
        }
        decltype(ctx->buffers) new_buffers;
        std::unordered_map<int, int> buffer_rename;
        for (auto &p : ctx->buffers) {
            buffer_rename[p.first] = (int)new_buffers.size();
            new_buffers.emplace((int)new_buffers.size(), std::move(p.second));
        }
        for (auto &v : ctx->vars) {
            if (v.second.buf_idx != -1) {
                v.second.buf_idx = buffer_rename[v.second.buf_idx];
            }
        }
        std::swap(ctx->buffers, new_buffers);
        ctx->_time++;
    }
    void nagisa_free_var(int i) {
        // std::cout << "free " << i << std::endl;
        auto v = ctx->vars.at(i);
        if (v.buf_idx != -1) {
            ctx->buffers.erase(v.buf_idx);
        }
    }
    void nagisa_copy_to_host(int idx, void *p) {
        auto &v = ctx->vars.at(idx);
        NGS_ASSERT(v.size != 1);
        nagisa_eval();

        auto buf_id = v.buf_idx;
        std::cout << "reading buffer" << buf_id << std::endl;
        ctx->buffers[buf_id]->read((uint8_t *)p, get_typesize(v.type) * v.size, 0);
    }
} // namespace nagisa