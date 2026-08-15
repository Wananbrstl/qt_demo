# 手写布局、停靠窗口与 MDI

## 学习目标

- 不使用绝对坐标构建可伸缩控件层次。
- 有意识地选择 stretch、size policy、margin 和 splitter。
- 使用 `QMdiArea` 管理唯一与非唯一文档。
- 持久化并安全恢复 `QMainWindow` 工作区。

## 主窗口外壳

`MainWindow` 以 `QMdiArea` 为 central widget；设备浏览器和属性检查器位于左右 `QDockWidget`，诊断窗口位于底部。这复用 `QMainWindow` 自带的拖动、浮动和状态持久化机制，避免自研一套窗口管理系统。

## 文档身份

`DocumentManager` 分配稳定键：

```text
monitor:<device-id>      每设备唯一
protocol:<device-id>     每设备唯一
history:<device-id>:N    可多开
```

再次打开唯一文档时只激活已有 `QMdiSubWindow`。历史查询允许多开，便于操作员对比不同时间范围。身份策略不应依赖窗口标题，因为标题会翻译和变化。

## 布局原则

- 页边距集中在 `UiMetrics`，不重复散布魔数。
- 实时摘要列通过 grid stretch 分配空间，不固定宽度。
- 曲线和表格之间用 `QSplitter`，允许用户调整。
- 表格列宽交给 `QHeaderView` 策略。
- 只有控件收缩后无法操作时才给最小尺寸提示。
- 页面必须适应字体、翻译和 DPI。

## 工作区持久化

`saveGeometry` 保存顶层窗口；`saveState(version)` 保存 dock 与 toolbar。显式版本让未来布局可以拒绝不兼容状态，“Reset Dock Layout” 是损坏或跑到屏幕外时的恢复通道。

恢复时应先验证几何是否与当前屏幕集合相交。显示器数量、缩放和主屏变化都可能让过去合法的矩形失效。

## 实验

1. 增加全局唯一的 Alarm 文档。
2. 独立持久化 MDI 模式与 dock 状态。
3. 增加前后文档快捷键。
4. 在 125%、150%、200% 缩放和中英文文本下测试布局。
5. 保存双屏布局后改为单屏，验证恢复路径。

验收：不使用 `setGeometry()` 修补布局，窗口在最小支持尺寸和高 DPI 下仍可操作。
