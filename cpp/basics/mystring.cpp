#include "mystring.h"

void MyString::reset() { ctor = copy_ctor = move_ctor = copy_assign = move_assign = dtor = 0; }
void MyString::report(const char* tag) {
    std::cout << tag << ": ctor=" << ctor << " copy_ctor=" << copy_ctor
            << " move_ctor=" << move_ctor << " copy_assign=" << copy_assign
            << " move_assign=" << move_assign << " dtor=" << dtor << "\n";
}

MyString::MyString(const char* s){
    len_ = std::strlen(s);
    data_ = new char[len_ + 1];
    std::memcpy(data_, s, len_ + 1);
    ++ctor; 
#ifdef DEBUG
    log("ctor       ");
#endif
}

    //拷贝构造 
MyString::MyString(const MyString& o){
    len_ = o.len_;
    data_ = new char[len_+1];
    std::memcpy(data_,o.data_,len_+1);
    ++copy_ctor; 
#ifdef DEBUG
    log("copy-ctor  ");
#endif
}

    //移动构造
MyString::MyString(MyString&& o){
    len_ = o.len_;
    data_ = std::move(o.data_);
    o.data_ = nullptr;
    o.len_ = 0;
    ++move_ctor;
#ifdef DEBUG 
    log("move-ctor  ");
#endif
}

    //拷贝赋值 + 移动赋值
MyString& MyString::operator=(const MyString& o){
    MyString temp(o);
    swap(*this,temp);
    ++copy_assign;
#ifdef DEBUG
    log("copy_assign    ");
#endif
    return *this;

}
MyString& MyString::operator=(MyString&& o){
    swap(*this,o);
    ++move_assign;
#ifdef DEBUG
    log("move_assign    ");
#endif
    return *this;
}

MyString::~MyString() { 
    ++dtor; 
#ifdef DEBUG
    log("dtor       "); 
#endif
    delete[] data_; 
}

void MyString::log(const char* what) const {
    std::cout << what << " this=" << this
              << " data_=" << static_cast<const void*>(data_)
              << " len_=" << len_ << "\n";
}

size_t MyString::size() const { return len_; }
    
void swap(MyString&a,MyString&b)noexcept{
    using std::swap;
    swap(a.data_,b.data_);
    swap(a.len_,b.len_);
}
