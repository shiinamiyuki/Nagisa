#include <iostream>
#include <fstream>
#include <nagisa/nagisa.hpp>
using namespace nagisa;
template <typename T>
struct Vec3 {  // Usage: time ./smallpt 5000 && xv image.ppm
    T x, y, z; // position, also color (r,g,b)
    Vec3(T x_ = 0, T y_ = 0, T z_ = 0) {
        x = x_;
        y = y_;
        z = z_;
    }

    Vec3 operator+(const Vec3 &b) const { return Vec3(x + b.x, y + b.y, z + b.z); }

    Vec3 operator-(const Vec3 &b) const { return Vec3(x - b.x, y - b.y, z - b.z); }

    Vec3 operator*(T b) const { return Vec3(x * b, y * b, z * b); }

    Vec3 operator*(const Vec3 &b) const { return Vec3(x * b.x, y * b.y, z * b.z); }

    Vec3 normalized() const { return *this * (1.0f / sqrt(x * x + y * y + z * z)); }

    T dot(const Vec3 &b) const { return x * b.x + y * b.y + z * b.z; } // cross:
    Vec3 cross(Vec3 &b) const { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
};

template <typename T>
struct Ray3 {
    Vec3<T> o, d;

    Ray3(Vec3<T> o_, Vec3<T> d_) : o(o_), d(d_) {}
};
template <typename Float>
struct TSphere {
    Float rad;     // radius
    Vec3<Float> p; // position, emission, color
    using Vec = Vec3<Float>;
    using Ray = Ray3<Float>;
    TSphere(Float rad_, Vec p_) : rad(rad_), p(p_) {}

    Float intersect(const Ray &r) const { // returns distance, 0 if nohit
        Vec op = p - r.o;                 // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
        Float t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        // if (det < 0.0)
        //     return 0;
        // else
        //     det = sqrt(det);
        // return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
        auto m = det >= 0.0;
        det = select<Float>(m, sqrt(det), 0.0f);
        Float t1 = b - det, t2 = b + det;
        return select<Float>(m, select<Float>(t1 >= 0.0, t1, select<Float>(t2 >= 0.0, t2, -1.0f)), -1.0f);
    }
};

int main() {

    nagisa_init();
    {
        using Float = GPUArray<float>;
        using Int = GPUArray<int>;
        using Sphere = TSphere<Float>;
        using Vec = Vec3<Float>;
        using Ray = Ray3<Float>;
        auto w = 1024, h = 1024;
        Vec image(0.0f);
        {
            Int idx = range<Int>(w * h);
            Int x = idx % w, y = idx / w;
            Sphere sphere(1.0f, Vec(0.0f, 0.0f, -10.0f));
            Float rx = Float(x) / w, ry = Float(y) / h;
            ry = 1.0f - ry;
            rx = rx * 2.0f - 1.0f;
            ry = ry * 2.0f - 1.0f;
            Vec o(0, 0, 0);
            Vec d = Vec(rx, ry, -1.0f).normalized();

            auto t = sphere.intersect(Ray(o, d));
            image = Vec(select<Float>(t >= 0.0, 1.0f, 0.0f)) * 255.0f;
        }
        auto data = image.x.data();
        std::ofstream out("out.ppm");
        out << "P2\n" << w << " " << h << "\n255\n";
        for (auto i : data) {
            out << i << " ";
        }
        out << "\n";
    }
    // nagisa_destroy();
}