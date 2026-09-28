# C++ 内存模型实验笔记

## 实验 1：拷贝 vs 移动（value_semantics.cpp）
- 移动构造必须写 `data(std::move(other.data))`；只写 `data(other.data)` 会退化成**深拷贝**，
  证据：错误版本 c.buf 是新地址、a.size() 仍为 3；正确版本 c.buf == 原 a.buf 且 a 变成 size=0/ buf=nullptr。
- 谁清空了源？是 std::vector 的移动构造自己。**手写类不会自动清空**。

## 实验 2：const 陷阱
- `const Tracker k; Tracker d = std::move(k);` → 走 copy-ctor。
- 原因：std::move(k) 的类型是 `const Tracker&&`，绑不上 `Tracker&&`，重载决议退回 `const Tracker&`。

## 实验 3：消除与优化无关
- prvalue 返回（`return Tracker{4,5,6};`）：两份输出完全一致 → **C++17 强制消除，-fno-elide-constructors 也关不掉**。
- 具名返回 NRVO：vs2 多出 move-ctor，且 id=7 的 buf 与 id=8 相同 → 关掉 NRVO 后发生的是**移动**（局部对象在 return 时被视为右值）。

## 附：内存分配开销
- 堆缓冲区地址每次 +0x20（32B），3 个 int 只占 12B → 多出的是 malloc 簿记 + 对齐开销。

### 1. `std::move()` 的签名
```cpp
template<class T>
constexpr std::remove_reference_t<T>&& std::move(T&& t) { return static_cast<T&&>(t); }
```

### 2. `std::move` 只是编译期的静态类型转换
- 它本身**并不做资源的移动工作**，只是把对象转换为一个**将亡值（xvalue）**；真正实现资源移动的是对象的**移动构造 / 移动赋值**函数。
- 注意 **`const T&&` 不会触发移动构造，会退化成拷贝**。
- 对内置类型使用 `std::move` 并不会有性能提升，只是语义上的一个体现。
- 函数 `return` 局部对象时：
  - **未命名（prvalue）**：C++17 强制触发 RVO（无法关掉），自动解析为右值，无需手动添加 `std::move`。
  - **已命名局部对象**：触发 NRVO，结果与 RVO 一致，但这是编译器层面的优化（可通过 `g++ -fno-elide-constructors` 关闭该优化），同样无需手动添加 `std::move`。
- `throw` 和 `return` 一样，会把局部变量当作「隐式可移动实体」，优先调用移动构造函数；**手动调用 `std::move` 反而会禁用复制消除**。

### 3. `vector` 的 `push_back` / `emplace_back` 与 `noexcept`
- `vector` 容器的 `push_back` 有两种重载，分别对应**拷贝**和**移动**；`emplace_back` 则是能直接在容器内构造对象。
- `vector` 容器在扩容时**检查对象的移动构造是否有 `noexcept` 标签**：有就会使用移动构造，否则使用拷贝构造。
- 原因是扩容时有 `is_nothrow_move_constructible` 的判断，以保证扩容时的**强异常安全**。
- 总结：`vector` 扩容要在"搬完之前不出异常"的前提下才能保证强异常安全，所以它用 move_if_noexcept——移动构造标了 noexcept 才敢搬，否则宁可拷贝

> 计数口径：`move_ctor` 是全局计数，含"临时对象搬进容器"与"扩容搬移"两部分。
> 带 noexcept：ctor=8, move_ctor=15(8+7), copy_ctor=0；不带 noexcept：ctor=8, move_ctor=8, copy_ctor=7。
> 结论：noexcept 只影响**扩容搬移**的选择（7 次移动 vs 7 次拷贝），不影响显式右值实参的移动。

### More Advanced
- T x=T(); 这行代码只产生一个对象（不是被优化了，而是自c++17后临时对象根本不存在）。
- return std::move(t) 破坏 NRVO 前提（操作数不是名字）→ 至少多一次移动；且-fno-elide-constructors 是证明不了它更慢的；
- 标量类型上 push_back(const T&) 与 push_back(T&&) 汇编等价——std::move 只改重载决议，不改工作量。

## D4 · RAII 进阶：循环引用、deleter 代价、拷 vs 移微基准

### 实验 1：`shared_ptr` 循环引用 → 泄漏（valgrind 实测）

两个 Node 用 `shared_ptr` 互指（`a->peer=b; b->peer=a`），出作用域后**一条 `dtor` 都没有**。

```text
definitely lost: 64 bytes in 1 blocks     ← Node A 整块（对象 + 控制块）
indirectly lost: 64 bytes in 1 blocks     ← Node B 整块（只被 A 内部的指针指着）
possibly lost: 0 / still reachable: 0
```

**为什么是 2 块 × 64B**：`make_shared` 把**对象和控制块放在一次分配**里。
- `64 = Node 48 + 控制块 16`
- `Node 48 = std::string 32 + shared_ptr 16`
- 控制块 16 = 强计数 8 + 弱计数 8

**为什么一个 `definitely` 一个 `indirectly`**：valgrind 从「根」（栈/寄存器/全局）做**可达性分析**。局部 `a`、`b` 销毁后强计数各从 2 降到 1（互相持有）→ 没有任何根能到达 → **不可达却又永不释放**；先被判 lost 的那块里的指针把另一块「带」成间接丢失。谁是什么取决于遍历顺序，不用纠结。
> **读泄漏只看总数：`definitely + indirectly + possibly`。**

**修法**：至少一条边改 `weak_ptr`（本次改 `b->peer`）；用时必须 `lock()` 提升并判空。
valgrind 应输出 `All heap blocks were freed`

**定位方法**：`valgrind --leak-check=full --show-leak-kinds=all --num-callers=20`，看 `definitely lost` 块下面的 **`alloc'd at`** 调用栈 → `make_shared` ← `make_cycle` ← `main`。

### 实验 2：deleter 与 `sizeof`（g++ 14.2 / x86-64 实测）

| 类型 | `sizeof` | 原因 |
|---|---|---|
| `unique_ptr<int>` | 8 | `default_delete` 是空类 |
| `unique_ptr<int, 空类deleter>` | 8 | 空基类优化 / `[[no_unique_address]]` 吃掉 0 字节 |
| `unique_ptr<int, 无捕获 lambda>` | 8 | 同上 |
| `unique_ptr<int, void(*)(int*)>` | **16** | 函数指针要存 → +8 |
| `unique_ptr<int, 带 4 个 int 的 deleter>` | 24 | 8 + 16 |
| `shared_ptr<int>` / `weak_ptr<int>` | 16 / 16 | 对象指针 + 控制块指针 |

**结论**：deleter 写成**无状态函数对象**就是零开销；写成**裸函数指针**每个指针白多 8 字节（100 万个 = 8MB 纯浪费）→ 生产代码用函数对象或无捕获 lambda。

### 实验 3：拷 vs 移微基准（`vector<MyString>`，N = 200000，`-O2`）

**口径**：`steady_clock` 计时；`reserve(N)` 排除扩容；`sink=9800000`（= 2e5 × 49）证明循环没被优化掉；每轮重建源数据（移动会掏空源）；3 轮、**丢弃第 1 轮**。

```text
round 0: copy=13.00 ms  move=4.10 ms   ← 含首次缺页/分配器扩张，作预热丢弃
round 1: copy= 7.95 ms  move=4.31 ms
round 2: copy= 6.97 ms  move=4.30 ms
```

**结果**：copy ≈ 7.46ms（7.95 / 6.97），move ≈ 4.31ms → **≈ 1.73×**；抖动主要来自 copy 轮（分配器抖动）。

**为什么只有 1.73×、不是 10×**：
- 每元素成本：copy ≈ 37ns（`malloc` ~25ns + `memcpy` 49B）；move ≈ 21ns（写 16B + 源置空 + `push_back` 分支）；
- 21ns 对「几次指针赋值」太贵 → 说明这段区间里**固定开销主导**（循环、`push_back`、内存写入、虚拟机开销），真实差异被淹没；
- 规律：**差异 ∝ 被搬运的载荷大小**。49B 太小，载荷越大倍数越高。



### 总结

1. **环**：`shared_ptr` 互指 → 强计数永不归零 → 既不可达又不释放；valgrind 报「1 块 `definitely` + 1 块 `indirectly`」；至少一条边改 `weak_ptr`。
2. **deleter**：`unique_ptr` 零开销的前提是 deleter **无状态**；`unique_ptr<T, void(*)(T*)>` 多占一个指针，因为函数指针本身有状态。
3. **移动**：收益 = 省下的那次**分配 + 深拷贝**；小对象测不出、大载荷才显形 → **报数字必须带载荷和口径**。