#include <iostream>
#include <cstring>
class MyString{
public:
    static int ctor, copy_ctor, move_ctor, copy_assign, move_assign, dtor;
    static void reset();
    static void report(const char* tag);

    char*  data_;
    size_t len_;

    MyString(const char* s = "");

    //拷贝构造 
    MyString(const MyString& o);
    //移动构造
    MyString(MyString&& o);

    //拷贝赋值 + 移动赋值
    MyString& operator=(const MyString& o);
    MyString& operator=(MyString&& o);

    ~MyString();

    void log(const char* what) const;
    size_t size() const;
    
    friend void swap(MyString&a,MyString&b)noexcept;
};