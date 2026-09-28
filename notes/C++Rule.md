# C++ Rule of Three / Rule of Five / Rule of Zero（三/五/零法则）
 
>这是C++针对资源管理类的编码指导原则，用于规范5个特殊成员函数：析构函数、拷贝构造、拷贝赋值、移动构造、移动赋值。
 
## 1.Rule of Three（三法则，C++98）
```text
If a class defines any one of a user-provided destructor, copy constructor, or copy assignment operator, it almost certainly needs to define all three.
```
若类用户自定义了`析构函数`、`拷贝构造函数`、`拷贝赋值运算符`三者中的任意一个，则几乎必须显式定义全部三者.

- 适用场景：类持有裸指针、文件句柄等外部资源，编译器隐式生成的拷贝操作是浅拷贝，会造成资源共享、double free、野指针等未定义行为。
- 三个成员：
1. 析构函数  ~T() 
2. 拷贝构造函数  T(const T&) 
3. 拷贝赋值运算符  T& operator=(const T&) 
## 2. Rule of Five（五法则，C++11，引入移动语义后扩展）
```text
A class that requires user-defined copy/destructor logic should also consider defining move constructor and move assignment operator; declaring any of the three Rule-of-Three members suppresses compiler-generated move special members.
```
- 若用户声明了三法则中任意成员，编译器不会隐式生成移动构造与移动赋值。需要资源转移语义的类，应显式定义全部五个特殊成员函数。
 
- 在三法则基础上增加两个移动相关成员：
4. 移动构造函数  T(T&&) noexcept 
5. 移动赋值运算符  T& operator=(T&&) noexcept 
 
>补充：若不需要拷贝能力，可使用  = delete  删除拷贝构造与拷贝赋值，只保留析构+移动两个函数。
 
## 3. Rule of Zero（零法则，现代C++推荐，C++ Core Guidelines C.20）

```text
Classes that do not directly manage ownership of resources should avoid custom destructors, copy/move constructors and copy/move assignment operators. Prefer RAII resource holders such as
std::string , std::vector , std::unique_ptr
```
- 不直接管理资源所有权的类，不要自定义析构、拷贝/移动构造、拷贝/移动赋值；将资源交给标准库RAII类型托管，编译器自动生成的特殊成员函数就是安全正确的版本。
 
- 核心思想：遵循单一职责，资源所有权交给RAII容器/智能指针，外层类完全不处理资源释放与拷贝逻辑。
 
## 总结优先级（官方推荐顺序）
 
- 1. 优先 Rule of Zero：现代C++首选，避免手动内存管理；
​
- 2. 必须手动管理裸资源（裸指针、fd）→ 使用 Rule of Five；
​
- 3. 兼容老旧C++98代码，无移动语义 → 使用 Rule of Three。
 
### 关键底层规则
 
- 一旦用户声明（包含 =default  /  =delete ）析构、拷贝构造、拷贝赋值三者之一，编译器将不再隐式生成移动构造、移动赋值；C++17起，隐式拷贝构造也被弃用。
 