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