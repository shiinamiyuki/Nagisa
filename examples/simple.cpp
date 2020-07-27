#include <iostream>
#include <nagisa/nagisa.hpp>

int main() {
    using namespace nagisa;
    nagisa_init();
    GPUArray<float> a(2.0);
    GPUArray<float> idx = range<float>(128);
    a = a.add_(idx);
    Buffer<float> out(128);
    store(out, idx, Mask(true), a);
    nagisa_eval();
    out.copy_to_host();
    for (int i = 0; i < 128; i++) {
        std::cout << out.data()[i] << " ";
    }
    std::cout << std::endl;
    nagisa_destroy();
}