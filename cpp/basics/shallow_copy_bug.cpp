#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
/*class Buffer {
public:
    char*  data_;
    size_t len_;
    explicit Buffer(size_t n) : data_(new char[n]), len_(n) { std::memset(data_, 0, n); }
    ~Buffer() { delete[] data_; }        // 只写了析构 —— Three 的第一条，另外两条没写
    // 拷贝构造 / 拷贝赋值：编译器自动生成 = 浅拷贝
};*/

//零法则优化
class Buffer {
    std::vector<char> buf_;
public:
    explicit Buffer(size_t n) : buf_(n, 0) {}
    // 什么都不用写：拷贝/移动/析构全由 vector 管好，且都正确
};

int main() {
    Buffer a(16);
    Buffer b = a;                        // 浅拷贝：b.data_ == a.data_
    //std::printf("a.data_=%p  b.data_=%p\n", (void*)a.data_, (void*)b.data_);  
    return 0;
}   // 析构顺序：b 先 delete[] → a 再 delete[] → double free