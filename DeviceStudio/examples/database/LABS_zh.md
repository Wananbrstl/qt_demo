# SQLite 与 MySQL 渐进实验

每个实验都包含“观察—提出假设—制造条件—收集证据—恢复”五步。不要只运行成功路径；数据库工程能力主要来自能解释失败发生在哪一层。

## 实验 0：建立可复现基线

目标：证明编译器、Qt 运行时、插件和测试来自预期环境。

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/examples/database/DatabaseDriverProbe
ctest --preset linux-debug --output-on-failure
```

记录 `cmake --version`、`qmake -v`、probe 输出与 `git rev-parse HEAD`。验收条件：QSQLITE 可用；未启用真实 MySQL 时，MySQL 契约测试应明确 `Skipped` 而非静默成功。

## 实验 1：SQLite 原子事务和 RAII 回滚

阅读 `TransactionGuard` 和 `test_database_core.cpp` 的 `transactionRollsBackOnScopeExit`。在事务作用域中插入两行，并在第二行前提前 `return`。确认析构回滚后计数仍为零。

思考：为什么“析构自动 commit”是危险 API？因为异常、错误返回和正常完成无法从析构器看到；安全默认应是回滚，成功必须显式表达。

验收：能说明原子性保护的是哪组业务不变量，而不只是背诵 ACID。

## 实验 2：预编译绑定与输入边界

把设备名设为 `m-01'); DROP TABLE devices;--`，通过工作台写入并查询。它应作为普通字符串保存。检查 repository：结构固定在 SQL 中，值只通过 `addBindValue`/`bindValue` 进入。

然后构造 `NaN`、16 字节以外的 event ID、空 device ID，运行测试确认在进入 SQL 前被拒绝。数据库约束仍保留，因为应用验证不能替代持久层最后防线。

验收：表仍存在，坏输入计入 `rejectedSamples`，日志不包含密码。

## 实验 3：批处理吞吐与尾延迟

分别设置：

1. `maximumBatchRows=1`、`flushIntervalMs=10`；
2. `maximumBatchRows=500`、`flushIntervalMs=250`；
3. `maximumBatchRows=5000`、`flushIntervalMs=1000`。

每种配置采集 60 秒，记录写入行数、`lastFlushMs`、WAL 大小和退出时残留。批量会摊薄事务与 fsync 成本，但批次过大增加尾延迟、锁持有时间和故障重放量。选择参数应基于采样速率与可接受数据滞留窗口：

```text
预计批次行数 ≈ 总采样速率(行/秒) × flushInterval(秒)
内存队列覆盖时间 ≈ maximumPendingSamples ÷ 峰值采样速率
```

验收：给出基于测量的参数，而不是沿用示例默认值。

## 实验 4：WAL、读写并发与 BUSY

运行：

```bash
ctest --test-dir build/linux-debug -R database_sqlite_concurrency -V
sqlite3 device-history.db "PRAGMA journal_mode; PRAGMA wal_checkpoint(PASSIVE);"
```

测试由一个连接持有 `BEGIN IMMEDIATE`，另一个写连接应在有界 busy timeout 后得到可重试错误。WAL 允许读者和写者并发，但仍只有一个写者。长读事务还会阻止 checkpoint 前进。

重要版本实验：执行 `SELECT sqlite_version()`，再核对发行版安全公告。SQLite 官方在 2026 年披露 WAL-reset 竞态，修复位于 3.51.3，并为部分旧分支提供回移版本。仅比较版本号不一定能判断 Ubuntu 包是否回移补丁；生产发布需要供应商公告或补丁清单。示例的单生产写者设计减少并发写/checkpoint 组合，但不能替代依赖治理。[SQLite WAL 公告与机制](https://www.sqlite.org/wal.html)

验收：能区分“读写并发”“多写者并行”和“busy timeout”三件事。

## 实验 5：迁移不可篡改性

第一次运行后查询：

```bash
sqlite3 device-history.db \
  "SELECT version,name,checksum,applied_at_ms FROM schema_migrations;"
```

临时修改已应用迁移中的一个字符并重跑，启动应因校验和不一致失败。恢复原迁移，新增更高版本迁移才是正确演进方式。

注意 MySQL DDL 可能隐式提交，不能把“迁移 runner 外层有 transaction”误认为所有 DDL 都可回滚。生产迁移要支持 expand/contract：先加兼容字段或表、发布兼容代码、回填、切流，最后再删除旧结构。

验收：能写出一次不阻断旧版本客户端的字段重命名计划。

## 实验 6：keyset 分页与执行计划

插入至少十万行同一设备数据，比较深页 offset 与 keyset：

```sql
EXPLAIN QUERY PLAN
SELECT event_id FROM telemetry_samples
WHERE device_id='lab-machine-01'
ORDER BY captured_at_ms DESC,event_id DESC
LIMIT 500 OFFSET 90000;

EXPLAIN QUERY PLAN
SELECT event_id FROM telemetry_samples
WHERE device_id='lab-machine-01'
  AND (captured_at_ms < :t OR (captured_at_ms=:t AND event_id<:id))
ORDER BY captured_at_ms DESC,event_id DESC LIMIT 500;
```

复合游标包含 event ID，因为时间戳可能相同；只用时间会跳行或重复。索引顺序要与等值过滤、排序和游标条件一起设计。

验收：分页过程中继续插入新数据，已遍历区间无重复、无缺失。

## 实验 7：事务 outbox 与崩溃窗口

启用 MySQL 配置但关闭 MySQL，运行采集器。SQLite 中 `telemetry_samples` 与 `outbox_events` 的 pending 数应同步增长。重启 MySQL，pending 应逐批变为 synced。

人为在“中心库提交成功、SQLite 标记 synced 前”结束进程。重启后同一事件会再次投递，但 MySQL `BINARY(16)` 主键使其仍是一行。这就是至少一次 + 幂等消费，不是假装实现分布式精确一次。

验收 SQL：

```sql
SELECT state,COUNT(*) FROM outbox_events GROUP BY state;
SELECT HEX(event_id),COUNT(*) FROM telemetry_samples
GROUP BY event_id HAVING COUNT(*)>1;
```

## 实验 8：MySQL 插件、TLS 和凭据

先用 Compose 启动隔离实验库，设置密码环境变量并运行契约测试。再依次制造：

- 不安装 `libqt5sql5-mysql`；
- 密码变量为空；
- 端口错误；
- CA 路径错误；
- 服务启动后中途停止。

检查错误是否被分类为驱动、配置或连接错误，是否标记 retryable，是否泄露密码。生产环境应配置 `SSL_CA/SSL_CERT/SSL_KEY` 并验证服务端身份；示例 Compose 只绑定环回地址，不能当生产网络模板。

验收：本地 SQLite 仍持续写入，MySQL 状态显示 offline，outbox 有界重试。

## 实验 9：死锁与完整事务重试

开启两个 MySQL 会话，以相反顺序更新两台设备，制造 1213。执行：

```sql
SHOW ENGINE INNODB STATUS\G
SELECT * FROM sys.innodb_lock_waits\G
```

应用不能只重试失败的最后一条语句；死锁牺牲者的整个事务已经回滚，应从事务入口重试。使用有限次数、指数退避和 jitter，并保持幂等键。更重要的是统一访问顺序、缩短事务、建立正确索引。[MySQL 官方死锁处理](https://dev.mysql.com/doc/refman/8.4/en/innodb-deadlocks-handling.html)

验收：重试有上限，最终错误保留 native code 和 operation，不形成热循环。

## 实验 10：背压与容量规划

把模拟器采样间隔降到 1 ms，同时让磁盘或数据库变慢。观察内存队列达到 `maximumPendingSamples` 后的最老数据丢弃和指标。

工业系统先定义数据等级：保护联锁事件不得走可丢队列；趋势遥测可以降采样；诊断日志可以滚动淘汰。对历史库至少估算：

```text
日行数 = 设备数 × 每设备点数 × 每秒采样次数 × 86400
日原始载荷 ≈ 日行数 × 单行平均字节数
保留空间 = (数据 + 索引 + WAL/redo + 临时空间) × 安全系数
同步恢复时间 ≈ backlog 行数 ÷ 可持续中心写入速率
```

验收：告警覆盖队列深度、最老 pending 年龄、写失败率、同步延迟、磁盘余量，而非只有“数据库在线”。

## 实验 11：备份必须通过恢复证明

SQLite 在线备份使用 SQLite backup API 或 CLI `.backup`，不要在 WAL 活跃时只复制主 `.db` 文件，因为 WAL 是持久状态的一部分：

```bash
sqlite3 device-history.db ".timeout 5000" ".backup /tmp/device-history.backup.db"
sqlite3 /tmp/device-history.backup.db "PRAGMA integrity_check;"
```

MySQL 实验库可使用交互密码的逻辑备份，再恢复到另一个全新 schema。生产需根据 RPO/RTO 选择物理备份、binlog/PITR 和异地副本。[SQLite Backup API](https://www.sqlite.org/backup.html)、[MySQL backup and recovery](https://dev.mysql.com/doc/refman/8.4/en/backup-and-recovery.html)。

验收：在空环境恢复，运行行数、外键、关键业务查询和应用契约测试；“生成了备份文件”不算完成。
