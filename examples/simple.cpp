#include <iostream>
#include <nagisa/nagisa.hpp>

int main() {
    using namespace nagisa;
    nagisa_init();
    GPUArray<float> a(2.0);
    GPUArray<float> idx = range<float>(128);
    for (int i = 0; i < 1; i++) {
        a = a.add_(idx);
    }
    // Buffer<float> out(128);
    // store(out, idx, Mask(true), a);
    std::vector<float> out = a.data();
    // nagisa_eval();
    // out.copy_to_host();
    for (int i = 0; i < 128; i++) {
        std::cout << out[i] << " ";
    }
    std::cout << std::endl;
    nagisa_destroy();
}