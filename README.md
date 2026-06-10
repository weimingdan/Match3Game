# Match3Game

一个基于 `Qt Quick + C++` 的三消 Demo，采用 `MSVC + QML` 工程结构，实现了一个以清除箱子为目标的 `9 列 x 8 排` 棋盘。

## 项目说明

项目目标是实现一个可演示、可复现、规则清晰的三消原型。当前版本已经具备完整的基础循环与主要道具系统：

- 普通元素匹配与连锁结算
- 箱子清除目标
- 火箭、炸弹、螺旋桨三类特殊元素
- 掉落、斜向补位、顶部补位
- 基于随机种子的稳定复现
- 顶部目标栏与胜利反馈

当前棋盘包含以下元素类型：

- 普通元素
- 箱子
- 火箭
- 炸弹
- 螺旋桨
- 空位

## 当前已实现规则

### 棋盘与目标

- 棋盘尺寸为 `9 列 x 8 排`
- 箱子不可交换、不可移动、不参与普通三消匹配
- 目标是清除棋盘中的全部箱子

### 初始化与随机性

- 初始盘面不预放特殊元素
- 初始盘面只生成普通元素和箱子
- 顶部补位只生成普通元素，不会随机补出特殊元素
- 同一 `seed` 下初始盘面与补位路径可复现

### 箱子布局

- 底部三排全部填满箱子
- 倒数第四排左侧 3 个、右侧 3 个放置箱子

### 普通匹配

- 支持横向、纵向 `3+` 匹配
- 普通三消命中格会对上下左右相邻箱子造成清除
- 同一个箱子即使被多个命中格同时波及，也只清除一次

### 特殊元素生成

- 横向 `4` 连或直线 `5` 连生成纵向火箭
- 纵向 `4` 连或直线 `5` 连生成横向火箭
- `T/L` 形五连生成炸弹
- `2x2` 方块匹配生成螺旋桨

### 特殊元素效果

- 火箭：清除整行或整列
- 火箭路径上的箱子会被直接清除
- 火箭不会再向路径外扩散出“相邻受击”
- 炸弹：清除以自身为中心的 `5x5` 范围
- 螺旋桨：交换触发后先清除自身中心和上下左右 1 格，再额外锁定当前最上层箱子中的一个并清除

### 特殊交换

- 火箭 + 螺旋桨交换时，先执行螺旋桨十字清除
- 若棋盘上存在异向火箭，则以该火箭位置触发双火箭十字消除
- 若不存在异向火箭，则退化为执行当前交换火箭的普通逻辑

### 连锁与胜利

- 一次有效交换会自动执行完整结算循环：

`交换 -> 匹配/触发 -> 清除 -> 掉落 -> 补位 -> 再检测`

- 棋盘未稳定前会锁定输入
- 所有箱子清除后进入 `Great` 胜利状态

## 实现方案

### 总体思路

项目采用“QML 展示层 + C++ 规则层”的结构：

- `QML` 负责界面表现、棋盘绘制、顶部信息和点击交互
- `C++` 负责棋盘数据、匹配规则、特殊元素生成、清除结算、掉落补位和状态流转

这次重构后，原来的大 `BoardModel` 被拆成了多个类的组合，职责更清晰。

### 重构后的核心类

#### `BoardModel`

文件：

- [Match3Game/Match3Game/boardmodel.h](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardmodel.h)
- [Match3Game/Match3Game/boardmodel.cpp](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardmodel.cpp)

职责：

- 继承 `QAbstractListModel`
- 向 QML 暴露格子数据和属性角色
- 管理选中状态、输入锁、胜利状态、状态文本
- 驱动一轮操作的时序：交换、回退、结算、掉落、连锁检测

可以把它理解为“QML 适配层 + 回合状态机”。

#### `BoardState`

文件：

- [Match3Game/Match3Game/boardstate.h](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardstate.h)
- [Match3Game/Match3Game/boardstate.cpp](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardstate.cpp)

职责：

- 存储棋盘格子数据
- 管理随机种子与随机数生成器
- 初始化棋盘与箱子布局
- 提供坐标换算、格子访问与基础类型判断
- 提供底层数据操作：清除箱子、清除可移动元素、掉落、补位

可以把它理解为“棋盘数据层”。

#### `BoardEngine`

文件：

- [Match3Game/Match3Game/boardengine.h](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardengine.h)
- [Match3Game/Match3Game/boardengine.cpp](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardengine.cpp)

职责：

- 分析普通匹配
- 计算特殊元素生成位置
- 处理普通匹配清除
- 处理火箭、炸弹、螺旋桨效果
- 处理火箭 + 螺旋桨特殊交换

可以把它理解为“纯玩法规则层”。

#### `boardtypes.h`

文件：

- [Match3Game/Match3Game/boardtypes.h](/D:/workspace/interview/Match3Game/Match3Game/Match3Game/boardtypes.h)

职责：

- 定义 `ItemType`
- 定义 `Cell`
- 定义交换、匹配、效果等中间结构体

可以把它理解为“共享领域类型定义”。

## 为什么这样拆分

原始 `BoardModel` 同时承担了：

- QML 模型
- 棋盘存储
- 匹配规则
- 特殊元素规则
- 交换状态机
- 掉落补位

这会带来几个问题：

- 类职责过多，阅读成本高
- 修改某条规则时容易影响 UI 层
- 规则逻辑不利于单独扩展
- 后续增加新道具或新关卡时维护压力更大

拆分后有几个直接收益：

- `BoardModel` 更薄，和 QML 的边界更清楚
- `BoardState` 可以专注于数据与基础操作
- `BoardEngine` 可以专注于“规则怎么结算”
- 后续扩展新规则时，基本只需要动 `BoardEngine`

## 技术栈

### 语言与框架

- `C++17`
- `Qt 6`
- `Qt Quick / QML`

## 目录结构

```text
Match3Game/
├─ README.md
├─ 开发计划书.md
└─ Match3Game/
   ├─ Match3Game.sln
   └─ Match3Game/
      ├─ boardtypes.h
      ├─ boardstate.h
      ├─ boardstate.cpp
      ├─ boardengine.h
      ├─ boardengine.cpp
      ├─ boardmodel.h
      ├─ boardmodel.cpp
      ├─ main.cpp
      ├─ main.qml
      ├─ qml.qrc
      └─ Match3Game.vcxproj
```

## 运行方式

### 在 Visual Studio 中运行

1. 打开 `Match3Game/Match3Game.sln`
2. 选择 `Release | x64`
3. 确认本机已安装对应 Qt6.7.2 和 `QtMsBuild`
4. 编译并运行
