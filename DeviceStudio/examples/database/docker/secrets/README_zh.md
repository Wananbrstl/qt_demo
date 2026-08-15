# MySQL 实验密码

在本目录创建不带换行以外额外内容的 `mysql_app_password` 文件，例如：

```bash
umask 077
printf '%s\n' '请替换为随机实验密码' > mysql_app_password
```

该文件已加入 `.gitignore`，不得提交真实密码。启动 Compose 后，在运行测试或应用的终端导出同一个值：

```bash
export DEVICESTUDIO_MYSQL_PASSWORD="$(< mysql_app_password)"
```

这里只是本机实验环境。生产系统应接入专用密钥管理服务、轮换凭据，并限制数据库网络入口。
