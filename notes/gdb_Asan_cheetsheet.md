# 调试工具速查：gdb / ASan / valgrind

> 三个工具分工不同，别混用（ASan 与 valgrind 不能同时开）：
> **gdb 看"崩在哪、变量是什么"｜ASan 看"哪个内存错误、谁先释放谁分配"｜valgrind 看"泄漏分类与可达性"**

## 一、gdb（交互式调试器）

### 编译前提（缺一不可）
```bash
g++ -std=c++17 -O0 -g -Wall -Wextra prog.cpp -o prog
```
- `-g`：生成调试信息（没有它只有地址、没有行号）
- `-O0`：不做优化（否则变量被优化掉、行号乱跳）

### 两种用法
```bash
# ① 交互式（适合边看边试）
gdb -q ./prog
(gdb) run

# ② 批处理（适合把输出存进笔记 / 复现）
gdb -batch -ex run -ex "bt" -ex "frame 9" -ex "p this->data_" ./prog
```

### 核心命令
| 命令 | 作用 |
|---|---|
| `run` (`r`) | 启动程序 |
| `bt` | 打印调用栈（backtrace），崩溃后第一件事 |
| `frame N` / `f N` | 切到第 N 帧（`up` / `down` 也可） |
| `info locals` | 当前帧的局部变量 |
| `info args` | 当前帧的函数参数（如 libc 收到的地址） |
| `p 表达式` | 打印值；**打指针加 `(void*)`**：`p (void*)p->data_` |
| `list` (`l`) | 显示当前帧对应的源码 |
| `quit` (`q`) | 退出 |

### 读栈三原则
1. **从下往上读**：`#0` 是"停在哪"，最底下才是"从哪来"（`main`）。
2. **找第一个属于自己代码的帧**：崩在 `libc` 的 `free`/`abort` 不代表 bug 在 libc。
3. **看地址并把它们对齐**：把"要释放的指针"（`this->data_`）、"libc 收到的地址"（`info args` → `mem=`）、"对象里的成员"（`a.data_`）三者对比。


## 二、ASan（AddressSanitizer，编译期插桩）

### 编译与运行
```bash
g++ -std=c++17 -O0 -g -fsanitize=address prog.cpp -o prog_asan && ./prog_asan
# 常用组合（更全面，稍慢）
g++ -std=c++17 -O1 -g -fsanitize=address,undefined prog.cpp -o prog_san && ./prog_san
```

### 输出怎么读
```text
ERROR: AddressSanitizer: double-free on 0x...        ← 错误类型 + 地址
freed by thread T0 here:                             ← ★ 第一次释放的栈（谁先 free 的）
    #0 operator delete[] ... #3 main ...:19
previously allocated by thread T0 here:              ← ★ 当初分配的栈（谁 new 的）
    #0 operator new[] ... #2 main ...:15
SUMMARY: AddressSanitizer: double-free ...          ← 一行摘要
```
**相较于gdb**：gdb 体现"崩在哪"，ASan 表明"**谁先释放、谁分配的**"。

### 常见错误名对照
| ASan 报的名字 | 含义 |
|---|---|
| `double-free` | 同一块释放两次 |
| `use-after-free` | 释放后还在用（悬垂指针） |
| `heap-buffer-overflow` | 堆数组越界读写 |
| `stack-buffer-overflow` | 栈数组越界 |
| `LeakSanitizer: detected memory leaks` | 退出时仍有不可达内存 |

### 代价
约 2 倍减速（可直接日常开着跑测试）；**valgrind 慢 20–50 倍**，所以小用例用 valgrind、日常用 ASan。

## 三、valgrind（泄漏与可达性分析）

```bash
g++ -std=c++17 -O0 -g prog.cpp -o prog && \
valgrind --leak-check=full --show-leak-kinds=all --num-callers=20 ./prog 2>&1 | tail -30
```
- 四个类别：`definitely lost` / `indirectly lost` / `possibly lost` / `still reachable`
- **读泄漏只看总数**：`definitely + indirectly + possibly`（`still reachable` 多半是 libc，正常）
- 定位靠每块下面的 **`alloc'd at`** 调用栈 → 直接映回源码行

## 四、选择

| 目的 | 工具 |
|---|---|
| 定位崩溃在哪一行、看变量的值 | **gdb**（`-g -O0`） |
| 判断是 double-free / 越界 / use-after-free，并看两处栈 | **ASan** |
| 判泄漏种类、查环形引用的可达性 | **valgrind** |
| 找性能热点 | `valgrind --tool=callgrind`（配 `callgrind_annotate`） |