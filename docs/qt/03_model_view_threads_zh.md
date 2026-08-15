# Model/View 与 UI 更新边界

## 为什么使用 Model/View

`QTableWidget` 把显示项存入控件，对大数据或共享数据集不友好。`TelemetryTableModel` 拥有领域样本，`QTableView` 负责选择、滚动、表头和 delegate，数据与呈现职责分离。

## 模型协议

修改底层容器之前必须先通知行变化：

```cpp
beginInsertRows({}, 0, 0);
samples_.prepend(sample);
endInsertRows();
```

View 可能保存 persistent index 和选择状态。绕过 begin/end 通知直接修改 vector，会造成难以稳定复现的 UI 状态损坏。

批量插入时应一次声明连续范围，避免逐行触发布局与重绘。只有结构完全重建且无法表达增量变化时才使用 `beginResetModel`。

## 有界实时数据

实时模型只保留 500 行，SQLite 保存长期历史。这样把操作显示需求与留存需求分开，避免窗口运行一周后无限占用内存。

## 线程边界

模型和控件始终留在 GUI 线程。worker 通过队列连接传递值类型副本，绝不直接调用模型方法。对高频数据应聚合后批量投递，避免事件队列本身成为无界缓冲区。

## 实验

- 增加 `QSortFilterProxyModel`，按温度阈值过滤。
- 编写 delegate 为高振动值着色，颜色不得写入领域对象。
- 使用 `QSignalSpy` 验证插入/删除行信号。
- 以 1 kHz 模拟数据运行 10 分钟，记录 GUI 线程延迟、事件积压和内存。
- 将逐样本通知改为每 50 ms 批量更新，对比 CPU 与交互延迟。

验收：模型行数始终受限，排序过滤后索引正确，worker 线程中没有任何 QWidget/QAbstractItemModel 调用。
