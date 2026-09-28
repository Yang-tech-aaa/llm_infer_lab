#include <iostream>
#include <memory>

void my_del(int *p){
    std::cout<<"successfully delete in my_del\n";
    delete p;
}
int main(){
    std::unique_ptr<int> a=std::make_unique<int>(8);
    std::unique_ptr<int,void(*)(int*)> b(new int(8),my_del);
    std::shared_ptr<int> c= std::make_shared<int>(8);

    std::cout<<"sizeof(unique_ptr):"<<sizeof(a)<<"bytes\n";
    std::cout<<"sizeof(unique_ptr(deleter)):"<<sizeof(b)<<"bytes\n";
    std::cout<<"sizeof(shared_ptr):"<<sizeof(c)<<"bytes\n";
    
}