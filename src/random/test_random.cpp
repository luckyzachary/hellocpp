#include <boost/thread/shared_mutex.hpp>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

size_t rand_rand() {
    return rand();
}

size_t rand_rand_r() {
    static unsigned int seed = time(nullptr);
    return rand_r(&seed);
}

size_t rand_std_random_device() {
    std::random_device rd;
    return rd();
}

size_t rand_std_random_device_static() {
    static std::random_device rd;
    return rd();
}

size_t rand_std_mt19937_64() {
    std::random_device rd;
    std::mt19937_64    mt(rd());
    return mt();
}

size_t rand_std_mt19937_64_static() {
    static std::random_device rd;
    static std::mt19937_64    mt(rd());
    return mt();
}

size_t rand_std_uniform_int_distribution() {
    std::random_device                      rd;
    std::mt19937_64                         mt(rd());
    std::uniform_int_distribution<unsigned> dis(0, std::numeric_limits<int>::max());
    return dis(mt);
}

size_t rand_std_uniform_int_distribution_static() {
    static std::random_device                      rd;
    static std::mt19937_64                         mt(rd());
    static std::uniform_int_distribution<unsigned> dis(0, std::numeric_limits<int>::max());
    return dis(mt);
}

std::tuple<std::unordered_map<size_t, size_t>, size_t, size_t> test_rand(const std::string& name, size_t (*randfunc)()) {
    auto                      threadNum = 16;
    std::vector<std::thread*> vt;
    std::vector<size_t>       counter(threadNum);
    auto                      randMax = 16;

    boost::shared_mutex mutex;
    mutex.lock();
    for (int i = 0; i < threadNum; i++) {
        vt.emplace_back(new std::thread(
            [&](int idx) {
                mutex.lock_shared();
                counter[idx] = randfunc() % randMax;
                mutex.unlock_shared();
            },
            i));
    }
    mutex.unlock();
    for (auto& t : vt) {
        if (t->joinable()) {
            t->join();
        }
    }

    std::unordered_map<size_t, size_t> times;
    for (int i = 0; i < randMax; i++) {
        if (times.count(i) <= 0) {
            times[i] = 0;
        }
    }
    for (auto i : counter) {
        times[i]++;
    }
    return std::make_tuple(times, threadNum, randMax);
}

double entropy(const std::unordered_map<size_t, size_t>& times, size_t threadNum, size_t randMax) {
    double e = 0.0;
    for (const auto& kv : times) {
        if (kv.second == 0) {
            continue;
        }
        auto p = double(kv.second) / double(threadNum);
        e -= p * log(p);
    }
    return e;
}

double variance(const std::unordered_map<size_t, size_t>& times, size_t threadNum, size_t randMax) {
    double v   = 0.0;
    double avg = double(threadNum) / double(randMax);
    for (const auto& kv : times) {
        v += (double(kv.second) - avg) * (double(kv.second) - avg);
    }
    return v;
}

int main(int argc, const char* argv[]) {
    srand(time(nullptr));

    auto v = {
        std::make_tuple("rand", rand_rand),
        std::make_tuple("rand_r", rand_rand_r),
        std::make_tuple("std::random_device", rand_std_random_device),
        std::make_tuple("static std::random_device", rand_std_random_device_static),
        std::make_tuple("std::mt19937_64", rand_std_mt19937_64),
        std::make_tuple("static std::mt19937_64", rand_std_mt19937_64_static),
        std::make_tuple("std::uniform_int_distribution_static", rand_std_uniform_int_distribution),
        std::make_tuple("static std::uniform_int_distribution_static", rand_std_uniform_int_distribution_static),
    };
    for (const auto& nf : v) {
        auto name = std::get<0>(nf);
        auto func = std::get<1>(nf);

        int                                threadNum = 0;
        int                                randMax   = 0;
        std::unordered_map<size_t, size_t> times;
        std::tie(times, threadNum, randMax) = test_rand(name, func);

        auto e = entropy(times, threadNum, randMax);
        auto v = variance(times, threadNum, randMax);

        std::cout << std::setw(50) << std::setiosflags(std::ios::left) << name << " => " << e << ", " << v << std::endl;
        // for (auto [key, val] : times) {
        //     std::cout << key << " => " << val << std::endl;
        // }
        // std::cout << std::endl;
    }
}