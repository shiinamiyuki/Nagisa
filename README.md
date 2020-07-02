# Nagisa: C++ Expression JIT and EDSL Framework

## Example

```c++
#include <nagisa/nagisa.h>
using nagisa::lang;
float32 exp(float32 x){
    float32 p = 1.0f;
    float32 sum = 0.0f;
    float32 fact = 1.0f;
    for(int i = 0; i < 5; i++){
        sum += p / fact;
        p *= x;
        fact *= i;
    }
    return sum;
}



using vec3 = vec<float, 3>;
struct Ray {    
    vec3 o, d;
};
NGS_STRUCT(Ray, o, d);
NGS_VECTORIZE_STRUCT(TRay, Ray); // template<size_t Lane> struct TRay{...};

Var<vec3> at(Var<const Ray&> ray, float32 dist){
    return ray.o + ray.d * dist;
}

int main(){
	float(*real_exp)(float) = Function(exp).compile();
    using vec16 = vec<float, 16>;
    vec16 (*vec_exp)(vec16) = Function(exp).vectorize(16).compile();
}

```

