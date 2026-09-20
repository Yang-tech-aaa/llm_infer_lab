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
