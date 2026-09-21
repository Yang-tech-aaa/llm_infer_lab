#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

// 用宏而不是复制两个类：编译两次即可对比 noexcept 的影响
#ifdef WITHOUT_NOEXCEPT
#  define MY_NOEXCEPT
#else
#  define MY_NOEXCEPT noexcept
#endif

class MyString {
public:
    static int ctor, copy_ctor, move_ctor, copy_assign, move_assign, dtor;
    static void reset() { ctor = copy_ctor = move_ctor = copy_assign = move_assign = dtor = 0; }
    static void report(const char* tag) {
        std::cout << tag << ": ctor=" << ctor << " copy_ctor=" << copy_ctor
                  << " move_ctor=" << move_ctor << " copy_assign=" << copy_assign
                  << " move_assign=" << move_assign << " dtor=" << dtor << "\n";
    }

    char*  data_;
    size_t len_;

    explicit MyString(const char* s = "") : data_(nullptr), len_(0) {
        len_ = std::strlen(s);
        data_ = new char[len_ + 1];
        std::memcpy(data_, s, len_ + 1);
        ++ctor; log("ctor       ");
    }

    //拷贝构造 
    MyString(const MyString& o){
        len_ = o.len_;
        data_ = new char[len_+1];
        std::memcpy(data_,o.data_,len_+1);
        ++copy_ctor; log("copy-ctor  ");
    }

    //移动构造
    MyString(MyString&& o)MY_NOEXCEPT{
        len_ = o.len_;
        data_ = std::move(o.data_);
        o.data_ = nullptr;
        o.len_ = 0;
        ++move_ctor; log("move-ctor  ");
    }

    //拷贝赋值 + 移动赋值
    MyString& operator=(const MyString& o){
        MyString temp(o);
        swap(*this,temp);
        ++copy_assign;
        log("copy_assign    ");
        return *this;
    }
    MyString& operator=(MyString&& o) MY_NOEXCEPT{
        swap(*this,o);
        ++move_assign;
        log("move_assign    ");
        return *this;
    }

    ~MyString() { ++dtor; log("dtor       "); delete[] data_; }

    void log(const char* what) const {
        std::cout << what << " this=" << this
                  << " data_=" << static_cast<const void*>(data_)
                  << " len_=" << len_ << "\n";
    }
    size_t size() const { return len_; }
    
    friend void swap(MyString&a,MyString&b)noexcept{
        using std::swap;
        swap(a.data_,b.data_);
        swap(a.len_,b.len_);
    }
};
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
