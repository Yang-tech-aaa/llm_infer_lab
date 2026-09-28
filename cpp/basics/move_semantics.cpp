#include <cstring>
#include <iostream>
#include <utility>
#include <vector>
#include "mystring.h"
// 用宏而不是复制两个类：编译两次即可对比 noexcept 的影响
#ifdef WITHOUT_NOEXCEPT
#  define MY_NOEXCEPT
#else
#  define MY_NOEXCEPT noexcept
#endif


int MyString::ctor = 0, MyString::copy_ctor = 0, MyString::move_ctor = 0;
int MyString::copy_assign = 0, MyString::move_assign = 0, MyString::dtor = 0;

int main() {
    std::cout << "== A 基本：拷贝 vs 移动 ==\n";
    MyString a("hello");
    MyString b = a;                 // 期望 copy-ctor
    MyString c = std::move(a);      // 期望 move-ctor，且 a.data_ 变 null

    std::cout << "== B const 陷阱 ==\n";
    const MyString k("const-one");
    MyString d = std::move(k);      // 期望仍是 copy-ctor

    std::cout << "== C vector 扩容：noexcept 的作用 ==\n";
    MyString::reset();
    std::vector<MyString> v;
    v.reserve(1);
    for (int i = 0; i < 8; ++i) v.push_back(MyString("this string is long enough to matter"));
    MyString::report("C");
}
