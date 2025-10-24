# MyLogisim – 电路逻辑仿真器（内部开发版）

> 一个类 Logisim 的图形化数字电路设计工具，基于 wxWidgets 实现。  
> 当前阶段：完成主框架 + 工具箱拖拽 + 画布绘制，持续迭代中。

---

## 1. 如何快速跑起来

| 平台           | 命令                              |
| -------------- | --------------------------------- |
| Windows (MSVC) | 打开 `MyLogisim.sln` → F5         |
| Linux / macOS  | 见 [docs/BUILD.md](docs/BUILD.md) |

> 首次运行前，请把 `resources/canvas_elements.json` 与 `resources/tools.json` 拷贝到可执行文件同级目录。

---

## 2. 仓库目录速览

MyLogisim
├── src
│   ├── gui
│   │   ├── MainFrame.*
│   │   ├── MainMenuBar.*
│   │   ├── CanvasPanel.*
│   │   ├── ToolboxPanel.*
│   │   └── PropertyPanel.*
│   ├── model
│   │   ├── CanvasElement.*
│   │   ├── CanvasModel.*
│   │   └── CanvasDropTarget.*
│   ├── utils
│   │   ├── ToolboxModel.*
│   │   └── my_log.h
│   └── cMain.cpp
├── resources
│   ├── canvas_elements.json
│   └── tools.json
├── docs
│   ├── BUILD.md
│   └── API.md
├── CMakeLists.txt
├── README.md
└── .gitignore



---

## 3. 核心类 / 工具一句话职责

| 类                 | 责任                                                         |
| ------------------ | ------------------------------------------------------------ |
| `MainFrame`        | 拼装界面（AUIManager）、转发菜单事件、协调各面板。           |
| `MainMenuBar`      | 仅负责“显示菜单 + 绑定事件”，所有动作委托 `MainFrame` 的 `DoXXX()` 接口。 |
| `CanvasPanel`      | 1) 双缓冲绘制网格 + 元件；2) 接受拖放并 `AddElement()`。     |
| `ToolboxPanel`     | 读取 `tools.json` 生成树；发起 `wxDropSource` 拖拽，数据为 JSON `{action:"add", type:"AND Gate"}`。 |
| `CanvasDropTarget` | 解析拖拽 JSON → 在 `g_elements` 中找模板 → 克隆并 `AddElement()`。 |
| `CanvasElement`    | 持有图形列表（`variant<Line, PolyShape, Circle, Text>`）+ 输入/输出引脚；提供 `Draw(wxDC)`。 |
| `CanvasModel`      | 全局 `LoadCanvasElements(json)` → 填充 `g_elements`，供拖拽时查找。 |
| `PropertyPanel`    | 暂时只显示“当前选中的元件名称”，后续扩展属性表。             |
| `my_log.h`         | 提供 `MyLog(fmt, ...)`：仅 **Debug 版** 输出到 Visual Studio 调试窗口，**Release 版零开销**。 |

---

## 4. 调试打印怎么用（my_log.h）

```cpp
#include "utils/my_log.h"

MyLog("[ToolboxPanel]  rebuild: main=%zu, categories=%zu\n", mains.size(), cats.size());
```



在 VS 下运行调试版本，可在 **输出窗口** 看到：

```
[ToolboxPanel]  rebuild: main=7, categories=5
```

> 无需格式化缓冲区，支持 `printf` 风格；Linux / macOS 下可扩展宏切换至 `fprintf(stderr, ...)`。

------

## 5. 拖拽-显示数据流（调试必看）

1. 启动时 `cMain` → `LoadCanvasElements("resources/canvas_elements.json")` 填充 `g_elements`。
2. 用户从 `ToolboxPanel` 拖动 → 生成 JSON → `CanvasDropTarget` 接收。
3. `CanvasDropTarget` 按 `type` 在 `g_elements` 找模板 → 克隆 → `CanvasPanel::AddElement()`。
4. `CanvasPanel` 刷新 → `OnPaint` 遍历 `m_elements` → `elem.Draw(dc)`。

> 若画布无显示，**优先检查日志行**
> `Loaded X elements from canvas_elements.json` 与
> `Canvas: added <AND Gate>, total=1`。

------

## 6. 约定 & 风格

- 文件名与类名一致，`*Panel` 继承 `wxPanel`，`*Model` 为纯函数或静态仓库。
- 事件转发：菜单 → `MainMenuBar` → `MainFrame::DoXXX()` → 业务。
- JSON 放在 `resources/`，程序内用相对路径读取；**不要硬编码绝对路径**。
- 新增元件三步：
  ① 在 `canvas_elements.json` 写图形 →
  ② 在 `tools.json` 加条目 →
  ③ 重新运行即可，**零代码**。
- 调试打印统一用 `MyLog(...)`，禁止手写 `printf` / `std::cout`，方便统一关闭。

------

## 7. 下一步迭代（Backlog）

- [ ] 选中控件、框选、删除、撤销/重做
- [ ] 连线与网表生成
- [ ] 仿真引擎（事件驱动 + 信号传播）
- [ ] 保存/打开 `.circ` 工程格式
- [ ] 国际化（wxLocale）

详细任务见 [GitHub Projects](https://github.com/你的组织/MyLogisim/projects) 或内部 Issue 面板。

------

## 8. 遇到问题？

1. 先翻日志 `drag.log`（行缓冲，实时刷新）。
2. 看 [docs/FAQ.md](https://www.kimi.com/chat/docs/FAQ.md) 是否已收录。
3. 在 Issue 贴日志 + 复现步骤，@ 当前模块 Owner。

------

**Happy Hacking!**
如需提交 PR，请遵循 [docs/CONTRIBUTING.md](https://www.kimi.com/chat/docs/CONTRIBUTING.md)。