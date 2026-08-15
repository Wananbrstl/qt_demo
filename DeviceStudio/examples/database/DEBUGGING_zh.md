# Qt 5 数据库调试手册

调试顺序遵循层次：配置输入 → Qt 插件 → 客户端动态库 → TCP/TLS → 服务端会话 → SQL/锁 → 磁盘。先确定失败层，避免同时修改多个参数。

## 1. 建立诊断快照

```bash
qmake -v
./build/linux-debug/examples/database/DatabaseDriverProbe
ldd ./build/linux-debug/examples/database/DatabaseDriverProbe
env | grep '^QT_'
```

不要打印 `DEVICESTUDIO_MYSQL_PASSWORD`。报告中记录 Qt、SQLite/MySQL server、QMYSQL 所链接 client library、内核、文件系统和配置的非秘密字段。

## 2. `QSQLITE driver not loaded`

```bash
QT_DEBUG_PLUGINS=1 \
./build/linux-debug/examples/database/DatabaseDriverProbe 2>&1 | tee /tmp/qt-sql-plugin.log

find /usr -path '*/sqldrivers/libqsqlite.so' -o -path '*/sqldrivers/libqsqlmysql.so'
ldd /usr/lib/x86_64-linux-gnu/qt5/plugins/sqldrivers/libqsqlite.so
```

常见原因：Qt 5 应用加载了另一个 Qt 版本的插件、插件架构不一致、依赖库缺失、`QT_PLUGIN_PATH` 污染。修复根因，不要把多个 Qt 插件目录全部塞进搜索路径。

## 3. `QMYSQL driver not loaded`

Ubuntu Qt 5 通常需要：

```bash
sudo apt-get install libqt5sql5-mysql
ldd /usr/lib/x86_64-linux-gnu/qt5/plugins/sqldrivers/libqsqlmysql.so
```

`QMYSQL` 出现在 `QSqlDatabase::drivers()` 只证明元数据发现了插件；实际加载仍可能因 `libmysqlclient`/MariaDB connector ABI 缺失而失败。用 `QT_DEBUG_PLUGINS=1` 看 loader 的第一条具体错误。

## 4. SQLite `database is locked` / `SQLITE_BUSY`

```bash
sqlite3 device-history.db \
  "PRAGMA journal_mode; PRAGMA busy_timeout; PRAGMA wal_checkpoint(PASSIVE);"
lsof device-history.db device-history.db-wal device-history.db-shm
```

检查：

1. 是否意外创建多个写连接；
2. 是否在事务中做网络请求、日志压缩或 UI 等待；
3. 是否有未结束的 `QSqlQuery` 保持读事务；
4. 长读事务是否造成 checkpoint starvation；
5. 数据库是否位于不支持 WAL 共享内存语义的网络文件系统。

busy timeout 只把立即失败变为有界等待，不能修复错误的事务设计。生产路径保持单写者；测试中的第二写者只用来验证错误分类。

## 5. WAL 变大

```bash
ls -lh device-history.db*
sqlite3 device-history.db "PRAGMA wal_checkpoint(PASSIVE);"
```

若 checkpoint 返回 busy，寻找长读者。不要直接删除 `-wal` 或 `-shm`；SQLite 官方明确说明 WAL 是数据库持久状态的一部分，错误分离可能丢失已提交事务。[SQLite WAL](https://www.sqlite.org/wal.html)

## 6. MySQL 连接失败

先独立于 Qt 验证网络和账户：

```bash
nc -vz 127.0.0.1 3306
mysql --host=127.0.0.1 --port=3306 --user=device_app \
  --password device_history
```

再检查 Compose：

```bash
cd examples/database/docker
docker compose ps
docker compose logs --tail=200 mysql
```

区分 DNS/拒绝连接/超时、认证失败、schema 不存在、TLS 验证失败。应用错误包含 `operation`、driver text、database text、native code 和 retryable；按字段定位，不用字符串包含关系决定业务逻辑。

## 7. 查询慢

SQLite：

```sql
EXPLAIN QUERY PLAN
SELECT event_id,captured_at_ms
FROM telemetry_samples
WHERE device_id='lab-machine-01'
ORDER BY captured_at_ms DESC,event_id DESC LIMIT 500;
```

MySQL：

```sql
EXPLAIN ANALYZE
SELECT event_id,captured_at_ms
FROM telemetry_samples
WHERE device_id='lab-machine-01'
ORDER BY captured_at_ms DESC,event_id DESC LIMIT 500;

SHOW SESSION STATUS LIKE 'Handler_read%';
```

先固定真实参数和数据分布，收集计划、扫描行数和耗时，再调整索引。索引增加写放大与磁盘占用；每个索引都应对应明确查询或约束。[MySQL EXPLAIN](https://dev.mysql.com/doc/refman/8.4/en/explain.html)

## 8. 死锁和锁等待

```sql
SHOW FULL PROCESSLIST;
SHOW ENGINE INNODB STATUS\G
SELECT * FROM sys.innodb_lock_waits\G
```

1213 是死锁牺牲者，需要重试整个事务；1205 是锁等待超时，默认行为与死锁回滚范围不同。代码的错误分类保留二者，不应统一成“SQL 执行失败”。先缩短事务、统一访问顺序、优化索引，再考虑退避重试。

## 9. 数据重复或丢失

按同一 event ID 从入口追踪：

```bash
sqlite3 device-history.db \
  "SELECT hex(event_id),state,attempt_count,last_error \
   FROM outbox_events ORDER BY created_at_ms DESC LIMIT 20;"
```

```sql
SELECT HEX(event_id),device_id,captured_at_ms
FROM telemetry_samples ORDER BY received_at_ms DESC LIMIT 20;
```

- SQLite 有、outbox 无：检查本地事务是否被拆开；本例应不可能。
- outbox pending：看 `next_attempt_at_ms`、MySQL 日志和环境变量。
- outbox dead：CBOR 损坏或不支持的 event type，需要人工修复/重放策略。
- MySQL 有、outbox pending：可能在远端提交后、本地确认前崩溃；再次投递应由主键幂等吸收。
- 两端都无：回到 TCP 接收、输入验证和内存背压指标。

## 10. GDB 观察线程亲和

```bash
gdb --args ./build/linux-debug/examples/database/DatabaseWorkbench \
  --config examples/database/config/database_lab.json
```

建议断点：

```gdb
set pagination off
break ds::database::DatabaseSession::open
break ds::database::HistorianWorker::flushLocalBatch
break ds::database::MysqlSyncWorker::synchronize
break ds::database::TransactionGuard::rollback
run
info threads
thread apply all bt 4
```

应看到 GUI、SQLite historian、MySQL sync 三个不同线程。`DatabaseSession::open/close` 的当前线程必须与对应 worker 相同。不要从 GDB 调用会改变数据库状态的表达式。

## 11. 系统调用与网络证据

```bash
strace -ff -tt -T -e trace=openat,fsync,fdatasync,connect,recvfrom,sendto \
  -o /tmp/historian.strace \
  ./build/linux-debug/examples/database/HistorianCollector \
  --database-config examples/database/config/database_lab.json

ss -tnp | grep ':3306'
```

`strace` 会显著扰动时序，只用于诊断。抓包可能包含业务数据和认证握手元数据，必须在授权环境中进行并保护输出。

## 12. 关闭时告警或连接泄漏

若出现 `QSqlDatabasePrivate::removeDatabase: connection ... is still in use`：

1. 确保所有 `QSqlQuery` 已离开作用域；
2. repository 先销毁；
3. `QSqlDatabase` 值句柄置空；
4. 最后按唯一连接名 `removeDatabase()`；
5. 上述操作都在连接创建线程执行。

本例停机顺序先 MySQL worker，再 SQLite worker：远端最后一个批次的确认有机会回到仍运行的本地线程；随后 SQLite flush 内存队列并关闭连接。
