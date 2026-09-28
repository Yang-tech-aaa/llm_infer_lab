#include <iostream>
#include <memory>
#include <string>

struct Node {
    std::string name;
    //std::shared_ptr<Node> peer;   // shared 互指
    std::weak_ptr<Node> peer;  // 换成 weak
    explicit Node(std::string n) : name(std::move(n)) { std::cout << "ctor " << name << "\n"; }
    ~Node() { std::cout << "dtor " << name << "\n"; }
};

void make_cycle() {
    auto a = std::make_shared<Node>("A");
    auto b = std::make_shared<Node>("B");
    a->peer = b;
    b->peer = a;                  //  环形成
    std::cout << "a.use_count=" << a.use_count() << " b.use_count=" << b.use_count() << "\n";
}
int main() { make_cycle(); std::cout << "--- 作用域已结束 ---\n"; }