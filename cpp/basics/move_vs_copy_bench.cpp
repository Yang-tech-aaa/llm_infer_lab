#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "mystring.h"   

using Clock = std::chrono::steady_clock;

static double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// 源数据：准备阶段【不计时】
static std::vector<MyString> make_srcs(int n) {
    std::vector<MyString> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) v.emplace_back("a reasonably long string to force heap allocation");
    return v;
}

int MyString::ctor = 0, MyString::copy_ctor = 0, MyString::move_ctor = 0;
int MyString::copy_assign = 0, MyString::move_assign = 0, MyString::dtor = 0;

int main() {
    const int N = 200000;                 
    std::vector<double> copy_ms, move_ms;

    for (int round = 0; round < 3; ++round) {
        // ---- 拷贝搬 ----
        auto srcs = make_srcs(N);          // 每轮重建（移动版会把源掏空）
        std::vector<MyString> dst;
        dst.reserve(N);                    // 排除扩容
        auto t0 = Clock::now();
        for (const auto& s : srcs) dst.push_back(s);          
        copy_ms.push_back(ms_since(t0));
        volatile size_t sink = 0; for (const auto& s : dst) sink += s.size();  //防优化删除

        // ---- 移动搬 ----
        srcs = make_srcs(N);               //重建
        std::vector<MyString> dst2;
        dst2.reserve(N);
        t0 = Clock::now();
        for (auto& s : srcs) dst2.push_back(std::move(s));    
        move_ms.push_back(ms_since(t0));
        sink = 0; for (const auto& s : dst2) sink += s.size();

        std::printf("round %d: copy=%.2f ms  move=%.2f ms  (sink=%zu)\n",
                    round, copy_ms.back(), move_ms.back(), (size_t)sink);
    }
}