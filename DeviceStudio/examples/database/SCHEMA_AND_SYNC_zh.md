# 上位机历史库的数据模型与同步算法

## 数据模型从查询与不变量推导

`devices` 保存设备身份和显示名；`telemetry_samples` 保存不可变采样；`outbox_events` 只存在于本地 SQLite，用于跨库同步。模式没有把 JSON/CBOR 全塞进一个万能表，因为温度、转速、振动和负载需要类型检查、索引与分析查询。CBOR 只作为 outbox 传输快照。

核心不变量：

- event ID 是 16 字节 UUID，SQLite 用 BLOB、MySQL 用 `BINARY(16)`；不用 36 字符文本浪费索引空间。
- UTC epoch millisecond 用有符号 64 位整数，避免 Qt 5/MySQL 时区隐式转换；显示层才转本地时区。
- device ID 先进入父表，采样外键指向设备。
- quality 限制为 0..255；浮点值在进入数据库前拒绝 NaN/Inf。
- `(device_id, captured_at_ms DESC, event_id DESC)` 同时服务设备时间范围、稳定排序和 keyset 游标。

## 为什么不是通用 ORM

此项目故意用小型 repository：SQL 与业务用例相邻，方言差异显式，事务边界由调用者读得见。大型 ORM 能减少机械映射，但不能替你决定批事务、幂等键、索引、锁顺序和故障恢复。对 CAD/工业上位机，数据路径通常少而性能/时序要求明确，显式 repository 更便于计划审查和故障注入。

## 本地写算法

```text
enqueue(record):
    validate(record)
    if memory_queue is full: apply declared overload policy
    push record
    if queue_size >= batch_limit: flush()

flush():
    take at most batch_limit records
    BEGIN
    for record in batch:
        upsert device
        insert telemetry idempotently
        insert outbox idempotently (only when MySQL enabled)
    COMMIT
    on failure: prepend the complete batch in original order
```

复杂度：批次数据库语句数为 O(n)，队列提取为 O(n)。吞吐关键不是 Big-O，而是一次事务摊薄固定提交成本。失败时整个批次回队，依赖 event ID 幂等；不能只回队“看起来失败”的最后一行，因为事务已经整体回滚。

## 同步算法

```text
SQLite worker:
    if no batch in flight:
        select pending where next_attempt <= now, ordered, limited
        mark one batch in memory as in-flight
        emit values to MySQL worker

MySQL worker:
    decode and validate each immutable payload
    dead-letter corrupt/unknown events
    connect/migrate if necessary
    persist valid records in one InnoDB transaction
    on success: ack event IDs
    on retryable failure: return event snapshots and diagnostic

SQLite worker:
    success -> state=synced
    retryable failure -> attempt++, next_attempt=now+backoff
    bad payload -> state=dead
    release in-flight slot only after terminal result
```

延期采用上限 60 秒的指数退避：`min(60000, 250 × 2^min(attempt,8))`。生产系统应加入随机 jitter，避免大量边缘站点在中心库恢复瞬间形成同步风暴。`attempt_count`、最老 pending 年龄和 dead 数量都应成为告警指标。

## 一致性语义

本地 SQLite 是采集系统的事实来源，中心 MySQL 是最终一致副本。系统保证：

- 本地事务成功后，采样与待同步意图同时存在；
- 远端可能收到同一事件多次；
- 唯一 event ID 使重复提交结果相同；
- 远端成功、本地确认前崩溃不会丢数据，只会重放；
- 没有声称跨 SQLite/MySQL 原子提交或全局精确一次。

如果业务命令涉及机器动作，不能直接套用遥测最终一致模型。命令需要授权、状态机、超时、去重、审计和设备侧安全联锁；数据库确认不等同于物理动作完成。

## 分页算法

第一页按 `(captured_at_ms,event_id)` 降序取 N 行。若正好 N 行，用最后一行组成下一页游标。后续条件为：

```sql
captured_at_ms < :time
OR (captured_at_ms = :time AND event_id < :event_id)
```

它保持 O(page size) 附近的索引扫描，不像大 offset 必须跳过不断增长的前缀。游标必须包含排序中的所有非唯一字段及最终唯一 tie-breaker。

## 错误分类而非字符串猜测

`DbError` 保存操作名、Qt driver text、数据库 text、native code、类别与 retryable。策略只依赖稳定类别/native code：SQLite 5/6 是 busy/locked；MySQL 1205 是锁等待、1213 是死锁，2002/2003/2006/2013 属于连接族。展示层仍保留原始诊断，便于现场定位。

分类不是永恒映射。部署新的 client library/server 后应通过契约测试校验 native code；业务层不得把所有 SQL 错误无限重试，因为约束、坏数据和迁移错误通常需要人工处理。

## SQLite 与 MySQL 的事务差异

SQLite WAL 仍是单写者模型；`BEGIN IMMEDIATE` 可提前取得写锁，busy timeout 提供有界等待。MySQL InnoDB 有行/索引范围锁与死锁检测；即使单行写也可能死锁，应用必须准备重试完整事务。两者共同原则是事务要短，不在持锁期间等待网络或用户。

MySQL DDL 存在隐式提交语义。因此迁移表的事务封装不能让任意 DDL 自动变成可回滚原子操作。生产变更应：

1. 用部署级互斥保证只有一个迁移者；
2. 设计可重复或可检测的步骤；
3. 采用 expand/contract 兼容多个应用版本；
4. 在等量数据副本上测量锁与耗时；
5. 备份并实际演练恢复。

## 面向 CAD 上位机的扩展

下一步可在保持边界的前提下增加：

- `alarm_events`：不可变告警发生/确认记录，独立于高频趋势采样；
- `machine_sessions`：加工任务、程序版本、工件/刀具批次与开始结束时间；
- `channel_samples`：对动态测点采用窄表或分区策略，但避免无约束 EAV；
- 分层保留：秒级原始数据短期、分钟聚合长期，聚合记录算法版本；
- 数据质量位：断线、传感器故障、插值、时钟异常，不用空值含糊表达；
- 设备时钟校准：同时保留 captured/received，监控漂移，禁止无依据改写原始时间；
- MySQL 分区/归档：先用查询和容量数据证明需要，再选择分区键，不能把分区当自动性能按钮。

每次扩展先写查询清单、保留策略、不变量和故障语义，再设计表与类。数据库 schema 是跨版本协议，修改成本通常高于普通私有 C++ 类。
