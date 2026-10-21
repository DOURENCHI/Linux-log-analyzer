# 代码笔记

关键函数与实现细节。全部在 `src/main.cpp`（237 行）。

## 1. 数据结构

### `LogStats`

```cpp
struct LogStats {
    int total_lines = 0;      // 通过筛选的行数
    int info_count = 0;
    int warning_count = 0;
    int error_count = 0;
    int unknown_count = 0;
    int empty_count = 0;
};
```

C++11 起的**类内成员初始化**，所以 `LogStats stats;` 一声明就全为 0，
不需要额外写构造函数或 `memset`。

**不变式**：`total_lines == info + warning + error + unknown + empty`
（因为是先 `total_lines++` 再归到某一类）。

## 2. 校验类函数

### `isValidTime(const std::string& time)`

两段式校验：

| 段 | 检查 |
|---|---|
| 格式 | `size() == 8`、`[2] == ':'`、`[5] == ':'`、其余位是 `'0'..'9'` |
| 范围 | 时 0–23、分 0–59、秒 0–59 |

**为什么先查字符再 `stoi`**：`std::stoi` 遇到非数字会**抛 `std::invalid_argument`**，
未捕获就 terminate。先把字符检查完，`stoi` 就只会遇到数字，安全。

`"99:99:99"` 也能被 `stoi` 正常解析（得到 99），所以范围检查不能省。

## 3. 筛选类函数

### `isBlankLine(const std::string& line)`

```cpp
return line.find_first_not_of(" \t\r\n") == std::string::npos;
```

`find_first_not_of` 返回第一个"不是这些字符"的位置。返回 `npos` 说明整行只由空白构成。

比 `line.empty()` 更严谨：`"   "` 不算空行、但 `isBlankLine` 会认它。

### `matchesKeyword(line, keyword)`

```cpp
if (keyword.empty() || keyword == "-") return true;   // 不做过滤
return line.find(keyword) != std::string::npos;
```

`keyword == "-"` 是一种**占位符约定**：命令行里第 3 个参数必须存在才能传第 4、5 个，
所以需要一个"我要跳过关键词过滤"的写法。

代价：如果日志里真的有 `-` 想搜索，这条路就断了。

### `matchesTimeRange(line, start, end)`

```cpp
if (start.empty() || end.empty()) return true;        // 时间过滤关闭

if (line.size() < 19 || line[10] != ' ' ||
    line[13] != ':' || line[16] != ':') {
    return false;                                     // 格式不符，直接跳过
}

std::string logTime = line.substr(11, 8);
return logTime >= start && logTime <= end;            // 字符串比较
```

三个要点：

| 点 | 说明 |
|---|---|
| 长度与分隔符预检 | 防止 `substr` 越界，也排除格式不符的行 |
| `substr(11, 8)` | 从第 11 位取 8 个字符，正好是 `HH:MM:SS` |
| **字符串比较** | 定长补零格式下，字典序 == 时间序，省掉解析成整数的开销 |

`>=` / `<=` 都是闭区间，起止时刻本身算在范围内。

## 4. 统计类函数

### `parseLine(line, stats)`

```cpp
if (isBlankLine(line))                                stats.empty_count++;
else if (line.find(" INFO ")    != npos)              stats.info_count++;
else if (line.find(" WARNING ") != npos)              stats.warning_count++;
else if (line.find(" ERROR ")   != npos)              stats.error_count++;
else                                                   stats.unknown_count++;
```

**关键词两侧带空格**（`" INFO "` 而不是 `"INFO"`），是为了避免误匹配：
日志内容里若出现 `INFO` 作为某个单词的一部分（如 `PREFINFO`），不带空格就会算错。

**`else if` 链的顺序**决定了优先级：一行里同时出现 `INFO` 和 `ERROR` 时，只算 INFO。
这在正常日志里不会发生，但换数据源时要注意。

### `analyzeLogFile(filePath, stats, keyword, start, end)`

```cpp
std::ifstream file(filePath);
if (!file.is_open()) { ... return false; }

while (std::getline(file, line)) {
    if (!matchesKeyword(line, keyword))    continue;
    if (!matchesTimeRange(line, start, end)) continue;
    stats.total_lines++;
    parseLine(line, stats);
}
if (stats.total_lines == 0) { ... }   // 提示日志为空/全被过滤
return true;
```

`std::getline` 会**自动去掉换行符**，所以后面 `isBlankLine` 里保留 `\r` 的处理
是为了兼容 Windows 换行（`\r\n` 经 getline 后可能残留 `\r`）。

## 5. 输出类函数

### `writeReport(std::ostream& out, const LogStats& stats)`

接收 `std::ostream&` 而不是文件名，因此**终端和文件共用同一份格式代码**：

```cpp
writeReport(std::cout, stats);        // printReport 内部
writeReport(reportFile, stats);       // saveReport 内部
```

这是本项目里最好的一处设计——避免了"屏幕上看到的格式"和"文件里的格式"不一致。

### `saveReport(reportPath, stats)`

两级检查：

| 检查点 | 失败含义 |
|---|---|
| `!reportFile.is_open()` | 路径不可写、目录不存在 |
| `!reportFile`（写完后再查） | 磁盘满、流状态异常 |

**写完还要再查一次流状态**，因为 `ofstream` 的写入是带缓冲的，
打开成功不代表写入一定成功。

## 6. `main()` 的参数解析

```cpp
if (argc != 2 && argc != 3 && argc != 5) { /* 打印 Usage，return 1 */ }

std::string filePath = argv[1];
if (argc == 3) { keyword = argv[2]; }
if (argc == 5) { keyword = argv[2]; startTime = argv[3]; endTime = argv[4]; }
```

用 `argc` 判断"调用者想要哪种模式"，简单直接。

`argv[0]` 被用在 Usage 提示里，所以打印出来的是**调用时用的名字**
（`./log_analyzer` 还是别的路径），对用户更友好。

## 7. 参数速查

| 项 | 值 |
|---|---|
| 标准 | C++17 |
| 编译 | `g++ -std=c++17 -Wall -Wextra -g src/main.cpp -o log_analyzer` |
| 环境 | Ubuntu 22.04 (WSL2) + G++ 11.4.0 |
| 时间格式 | `HH:MM:SS`（定长、补零） |
| 日志时间位置 | 行首 `YYYY-MM-DD HH:MM:SS`，`substr(11, 8)` |
| 级别关键词 | `" INFO "` / `" WARNING "` / `" ERROR "`（两侧带空格） |
| 报告路径 | `reports/analysis_report.txt`（硬编码） |

## 8. 原理补充

### 为什么用 `std::getline` 而不是 `file >> line`

`>>` 以空白为分隔，会把一行拆成多个"词"，日志行里的空格就全丢了。
`getline` 按行读，保留行内所有空格。

### 为什么字符串比较时间能成立

前提有三个：**定长**、**补零**、**高位在前**。

```
"09:05:03" vs "14:23:05"
  字典序：'0' < '1'  → 09 在前 ✓  与时间序一致
```

如果写成 `9:5:3`，`'9' > '1'`，字典序会把 9 点排到 14 点后面，就错了。
所以这个技巧**依赖格式约束**，不是通用做法。
