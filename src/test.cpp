#include <gperftools/profiler.h>
#include "param.hpp"
#include "tlwe.cpp"
#include "trlwe.cpp"
#include "trgsw.cpp"
#include "bootstrapping.cpp"
#include "gate.cpp"
#include <iostream>
#include <cassert>

template <typename Parameter>
void test_tlwe(unsigned int seed, const SecretKey<Parameter>& s) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        bool m = dist(rng);
        TLWElvl0<Parameter> tlwe = TLWElvl0<Parameter>::encrypt(s, m, rng);
        bool m_ = tlwe.decrypt_bool(s);
        std::cerr << m << " " << m_ << std::endl;
        assert(m == m_);
    }
}
template <typename Parameter>
void test_trlwe(unsigned int seed, const SecretKey<Parameter>& s) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        Poly<bool, Parameter::N> m;
        for (std::size_t i = 0; i < Parameter::N; ++i) m[i] = dist(rng);
        TRLWE<Parameter> trlwe = TRLWE<Parameter>::encrypt(s, m, rng);
        Poly<bool, Parameter::N> m_ = trlwe.decrypt_poly_bool(s);
        for (std::size_t i = 0; i < Parameter::N; ++i) {
            std::cerr << m[i] << " " << m_[i] << std::endl;
            assert(m[i] == m_[i]);
        }
        std::cerr << std::endl;
    }
}
template <typename Parameter>
void test_external_product(unsigned int seed, const SecretKey<Parameter>& s) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        Poly<bool, Parameter::N> m;
        for (int i = 0; i < Parameter::N; ++i) m[i] = dist(rng);
        int c = dist(rng) ? 1 : -1;
        auto trlwe = TRLWE<Parameter>::encrypt(s, m, rng);
        auto trgsw = TRGSW<Parameter>::encrypt(s, c, rng);
        TRLWE<Parameter> res;
        external_product(res, trgsw, trlwe);
        auto m_ = res.decrypt_poly_bool(s);
        std::cerr << c << std::endl;
        if (c == 1) {
            for (std::size_t i = 0; i < Parameter::N; ++i) {
                assert(m[i] == m_[i]);
            }
        } else {
            for (std::size_t i = 0; i < Parameter::N; ++i) {
                assert(m[i] != m_[i]);
            }
        }
    }
}

template <typename Parameter>
void test_cmux(unsigned int seed, const SecretKey<Parameter>& s) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        Poly<bool, Parameter::N> t = {};
        for (std::size_t i = 0; i < Parameter::N; ++i) t[i] = dist(rng);
        Poly<bool, Parameter::N> f = {};
        for (std::size_t i = 0; i < Parameter::N; ++i) f[i] = dist(rng);
        bool c = dist(rng);
        auto trlwe_t = TRLWE<Parameter>::encrypt(s, t, rng);
        auto trlwe_f = TRLWE<Parameter>::encrypt(s, f, rng);
        auto trgsw_c = TRGSW<Parameter>::encrypt(s, c, rng);
        TRLWE<Parameter> trlwe;
        cmux(trlwe, trgsw_c, trlwe_t, trlwe_f);
        Poly<bool, Parameter::N> res = trlwe.decrypt_poly_bool(s);
        for (std::size_t i = 0; i < Parameter::N; ++i) {
            std::cerr << res[i] << " " << (c ? t[i] : f[i]) << std::endl;
            assert(res[i] == (c ? t[i] : f[i]));
        }
    }
}

template <class Parameter>
void test_blind_rotate(unsigned int seed, const SecretKey<Parameter>& s, const BootstrappingKey<Parameter>& bk) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        bool m = dist(rng);
        auto tlwe = TLWElvl0<Parameter>::encrypt(s, m, rng);
        TLWElvl1<Parameter> res_tlwe;
        gate_bootstrapping_tlwe_to_tlwe(res_tlwe, tlwe, bk);
        bool m_ = res_tlwe.decrypt_bool(s);
        std::cerr << m << " " << m_ << std::endl;
        assert(m == m_);
    }
}

template <class Parameter>
void test_identity_key_switch(unsigned int seed, const SecretKey<Parameter>& s) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        bool m = dist(rng);
        auto ks = KeySwitchKey<Parameter>::make_ptr(s, rng);
        TLWElvl1<Parameter> tlwe1 = TLWElvl1<Parameter>::encrypt(s, m, rng);
        TLWElvl0<Parameter> tlwe0 = identity_key_switch(tlwe1, *ks);
        bool m_ = tlwe0.decrypt_bool(s);
        std::cerr << m << " " << m_ << std::endl;
        assert(m == m_);
    }
}

template <class Parameter>
void test_hom_nand(unsigned int seed, const SecretKey<Parameter>& s, const BootstrappingKey<Parameter>& bk) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        bool x = dist(rng), y = dist(rng);
        auto ks = KeySwitchKey<Parameter>::make_ptr(s, rng);
        TLWElvl0<Parameter> tlwex = TLWElvl0<Parameter>::encrypt(s, x, rng);
        TLWElvl0<Parameter> tlwey = TLWElvl0<Parameter>::encrypt(s, y, rng);
        TLWElvl0<Parameter> tlwexy;
        hom_xor(tlwexy, tlwex, tlwey, bk, *ks);
        bool xy = tlwexy.decrypt_bool(s);
        std::cerr << x << " " << y << " " << xy << std::endl;
        assert((x ^ y) == xy);
    }
}

template <class Parameter>
void test_hom_mux(unsigned int seed, const SecretKey<Parameter>& s, const BootstrappingKey<Parameter>& bk) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        bool x = dist(rng), y = dist(rng), z = dist(rng);
        auto ks = KeySwitchKey<Parameter>::make_ptr(s, rng);
        TLWElvl0<Parameter> tlwex = TLWElvl0<Parameter>::encrypt(s, x, rng);
        TLWElvl0<Parameter> tlwey = TLWElvl0<Parameter>::encrypt(s, y, rng);
        TLWElvl0<Parameter> tlwes = TLWElvl0<Parameter>::encrypt(s, z, rng);
        TLWElvl0<Parameter> tlwemux;
        hom_mux(tlwemux, tlwex, tlwey, tlwes, bk, *ks);
        bool mux = tlwemux.decrypt_bool(s);
        // std::cerr << x << " " << y << " " << z << " " << mux << std::endl;
        assert((z ? x : y) == mux);
    }
}
double sum = 0;
template <class Parameter>
void test_gates(unsigned int seed, const SecretKey<Parameter>& s, const BootstrappingKey<Parameter>& bk) {
    std::default_random_engine rng{seed};
    {
        std::binomial_distribution<> dist;
        bool x = dist(rng), y = dist(rng);
        auto ks = KeySwitchKey<Parameter>::make_ptr(s, rng);
        TLWElvl0<Parameter> tlwex = TLWElvl0<Parameter>::encrypt(s, x, rng);
        TLWElvl0<Parameter> tlwey = TLWElvl0<Parameter>::encrypt(s, y, rng);
        {
            clock_t start = clock();
            TLWElvl0<Parameter> tlwenand;
            hom_nand(tlwenand, tlwex, tlwey, bk, *ks);
            bool xynand = tlwenand.decrypt_bool(s);
            assert((!(x && y)) == xynand);
            clock_t end = clock();
            double t = static_cast<double>(end - start) / CLOCKS_PER_SEC * 1000.0;
            sum += t;
        }/*
        {
            TLWElvl0<Parameter> tlweor;
            hom_or(tlweor, tlwex, tlwey, bk, *ks);
            bool xyor = tlweor.decrypt_bool(s);
            assert((x | y) == xyor);
        }
        {
            TLWElvl0<Parameter> tlweand;
            hom_and(tlweand, tlwex, tlwey, bk, *ks);
            bool xyand = tlweand.decrypt_bool(s);
            assert((x & y) == xyand);
        }
        {
            TLWElvl0<Parameter> tlwenor;
            hom_nor(tlwenor, tlwex, tlwey, bk, *ks);
            bool xynor = tlwenor.decrypt_bool(s);
            assert((!(x | y)) == xynor);
        }
        {
            TLWElvl0<Parameter> tlwexor;
            hom_xor(tlwexor, tlwex, tlwey, bk, *ks);
            bool xyxor = tlwexor.decrypt_bool(s);
            assert((x ^ y) == xyxor);
        }
        {
            TLWElvl0<Parameter> tlwexnor;
            hom_xnor(tlwexnor, tlwex, tlwey, bk, *ks);
            bool xyxnor = tlwexnor.decrypt_bool(s);
            assert((!(x ^ y)) == xyxnor);
        }*/
    }
}

int main() {
    using P = param::Security128bit;
    static constexpr int n = 10, m = 20;
    std::cerr << n << " " << m << std::endl;
    //ProfilerStart("test.prof");
    for (int i = 0; i < n; ++i) {
        std::cerr << i << std::endl;
        SecretKey<P> key;
        unsigned seed = std::random_device{}();
        std::default_random_engine rng{seed};
        key = SecretKey<P>{rng};
        auto bk = BootstrappingKey<P>::make_ptr(key, rng);
        std::cerr << "finished Bootstarpping" << std::endl;

        for (int j = 0; j < m; ++j) {
            //clock_t start = clock();
            unsigned int seed = std::random_device{}();
            test_gates<P>(seed, key, *bk);
            //test_hom_mux<P>(seed, key, *bk);
            //clock_t end = clock();
            //double t = static_cast<double>(end - start) / CLOCKS_PER_SEC * 1000.0;
            //std::cerr << t << std::endl;
        }
    }
    //ProfilerStop();
    std::cerr << sum / (n * m) << std::endl;
    std::cout << "PASS" << std::endl;
}
