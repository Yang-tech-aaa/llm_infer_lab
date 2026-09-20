#include <iostream>
#include <vector>
#include <utility>

class Tracker{
public:
    std::vector<int> data;
    int id;
    static int next_id;
    
    //1.普通构造
    Tracker(std::initializer_list<int> init):data(init),id(next_id++){
        log("ctor    ");
    }

    //2.拷贝构造
    Tracker(const Tracker& other):data(other.data),id(next_id++){
        log("copy-ctor");
    }

    //3.移动构造
    Tracker(Tracker&& other) noexcept:data(std::move(other.data)),id(next_id++){
        log("move-ctor");
    }
    
    //4.析构
    ~Tracker(){log("dtor    ");}

    void log(const char* what)const{
        std::cout<<what
                 <<" id="  <<id
                 <<" this="<<this
                 <<" size="<<data.size()
                 <<" buf=" <<data.data()<<"\n";
    }
};

int Tracker::next_id = 1;

Tracker make_prvalue(){return Tracker{4,5,6};}            //c++17强制消除；
Tracker make_named(){Tracker t{7,8,9}; return t;}         //NRVO:可被关掉
                                                          //
int main(){
    std::cout<<" === A 左值初始化：期望 copy-ctor ===\n";
    Tracker a{1,2,3};
    Tracker b = a;

    std::cout<<" === B std::move: 期望 move-ctor 且该行过后a.buf变为nullptr ===\n";
    Tracker c = std::move(a);  //这行过后a不该再被使用

    std::cout<<" === C const陷阱：期望 copy-ctor ===\n";
    const Tracker k{10,11,12};
    Tracker d = std::move(k);

    std::cout<<" === D prvalue返回：期望 看不到ctor调用 ===\n";
    Tracker e = make_prvalue();

    std::cout<<" === E 具名返回:NRVO,可关闭 ===\n";
    Tracker f = make_named();

    std::cout<<" === 离开作用域：析构顺序应为 f e d k c b a ===\n";
}
