# EMQX Broker Deployment

## English Version

This directory documents MQTT Broker deployment, permissions, and verification. The tested baseline is **EMQX 5.3.2 for Windows**.

Do not commit a complete EMQX distribution, `data/`, `log/`, runtime databases, or real certificates. The repository should retain this README, sanitized configuration examples, and only the deployment scripts that are genuinely maintained.

## Role

EMQX forwards MQTT messages between the PC and M100M:

```text
PC publisher -> EMQX -> M100M subscriber
```

The Broker does not parse the five-field control payload, but it must enforce authentication, topic authorization, and connection management.

## Baseline Ports

| Service | Default Port | Publication Guidance |
| --- | ---: | --- |
| MQTT TCP | 1883 | Acceptable on a controlled research network; restrict access or use TLS on public infrastructure |
| EMQX Dashboard HTTP | 18083 | Allow administrators only from a trusted network |

For cloud deployment, configure both the operating-system firewall and the cloud security group. Never expose the Dashboard openly to the Internet.

## Local Windows Start

The current working copy contains an unpacked EMQX 5.3.2 directory for local historical reproduction:

```powershell
cd "server\emqx-5.3.2-windows-amd64"
.\bin\emqx.cmd start
.\bin\emqx.cmd ping
```

Stop it with:

```powershell
.\bin\emqx.cmd stop
```

Local Dashboard:

```text
http://127.0.0.1:18083
```

For a formal handover, download the selected version again from a trusted source instead of relying on an old binary copied into the repository.

## Authentication and Authorization

Create at least two MQTT roles:

| Role | Minimum Permission |
| --- | --- |
| PC controller | Publish only to `cone/<DEVICE_ID>/command` |
| M100M DTU | Subscribe only to its own `cone/<DEVICE_ID>/command` |

Reserve the following for future uplink:

| Role | Minimum Permission |
| --- | --- |
| M100M DTU | Publish `cone/<DEVICE_ID>/status` |
| Monitoring client | Subscribe only to authorized `cone/+/status` topics |

Requirements:

- Use a unique Client ID for every device.
- Use different accounts for PC and DTU.
- Disable anonymous access.
- Generate strong random passwords and hand them over through a secure channel.
- Never store real passwords in README files, screenshots, scripts, or Git history.
- Enable TLS and define certificate rotation before production deployment.

## Topic Convention

Recommended:

```text
cone/<DEVICE_ID>/command
cone/<DEVICE_ID>/status
```

The historical script uses `cone1d`. PC and M100M must temporarily match when reproducing that setup. Change the PC, M100M, ACL, and documentation together when migrating to the recommended convention.

## Deployment Procedure

1. Choose a local, campus-network, or cloud deployment location.
2. Install the selected EMQX version and record its source and checksum.
3. Replace the default Dashboard administrator password.
4. Configure the MQTT listener; prefer TLS for any public deployment.
5. Create separate PC and DTU authentication accounts.
6. Configure per-device publish/subscribe ACL rules.
7. Open only required ports and restrict Dashboard sources.
8. Verify authentication and ACL rules with two ordinary MQTT clients.
9. Configure the M100M and confirm its unique Client ID in the Dashboard.
10. Configure the PC controller last.

## Verification Checklist

- The PC connects and publishes to its command topic.
- The PC cannot publish to another device's topic.
- The M100M subscribes to its own command topic.
- The M100M cannot subscribe to another device's topic.
- Command Retain is false.
- Duplicate Client IDs are detected and corrected.
- Authentication and ACL rules survive a Broker restart.
- Logs do not show persistent authentication failures or reconnect loops.

## Configuration and Runtime Data

In the current Windows package:

- `etc/emqx.conf`: static override configuration.
- `data/configs/cluster.hocon`: settings written by Dashboard or API.
- `data/mnesia/`: runtime database that may contain accounts and state.
- `log/`: runtime logs.
- `etc/certs/`: example or deployment certificates.

Do not publish these runtime directories. Export only reviewed and sanitized templates:

```text
server/
├── README.md
└── config/
    └── emqx.example.conf
```

## Deployment Handover Record

| Item | Record |
| --- | --- |
| EMQX version | 5.3.2 or actual deployment version |
| Host operating system | To be completed |
| MQTT transport | TCP / TLS |
| Dashboard access scope | To be completed |
| Topic convention | `cone/<DEVICE_ID>/...` |
| Backup location | Record without credentials |
| Last verification date | YYYY-MM-DD |
| Related Git commit | Commit SHA |

## Current Security Warning

The historical PC script and EMQX runtime directory contain test credentials and runtime state. Remove them and rotate every exposed credential before publication. Updating this README alone does not make those credentials safe.

Return to the [project home](../README.md).

---

# 中文版

本目录记录 MQTT Broker 的部署、权限和验证方式。当前测试基线是 **EMQX 5.3.2 for Windows**。

完整 EMQX 发行包、`data/`、`log/`、运行数据库和真实证书不应提交到源码仓库。仓库只应长期保留本 README、脱敏配置模板和必要的部署脚本。

## 作用

EMQX 负责在 PC 和 M100M 之间转发 MQTT 消息：

```text
PC publisher -> EMQX -> M100M subscriber
```

Broker 不解析五字段控制协议，但必须正确实施认证、Topic 权限和连接管理。

## 当前基线端口

| 服务 | 默认端口 | 发布建议 |
| --- | ---: | --- |
| MQTT TCP | 1883 | 研究内网可用；公网应限制来源或启用 TLS |
| EMQX Dashboard HTTP | 18083 | 仅允许管理员从受信网络访问 |

若使用云服务器，还需在系统防火墙和云安全组中配置对应端口。不要把 Dashboard 无限制暴露到公网。

## Windows 本地启动

当前工作副本中存在 EMQX 5.3.2 解压目录，可用于本地回溯：

```powershell
cd "server\emqx-5.3.2-windows-amd64"
.\bin\emqx.cmd start
.\bin\emqx.cmd ping
```

停止：

```powershell
.\bin\emqx.cmd stop
```

本地 Dashboard：

```text
http://127.0.0.1:18083
```

正式接手时应从可信来源重新获取指定版本，不要依赖仓库中的旧二进制副本。

## 认证与权限

至少创建两类 MQTT 账号：

| 账号角色 | 最小权限 |
| --- | --- |
| PC controller | 只允许发布 `cone/<DEVICE_ID>/command` |
| M100M DTU | 只允许订阅自己的 `cone/<DEVICE_ID>/command` |

可为未来上行预留：

| 账号角色 | 最小权限 |
| --- | --- |
| M100M DTU | 发布 `cone/<DEVICE_ID>/status` |
| Monitoring client | 订阅允许查看的 `cone/+/status` |

要求：

- 每个设备使用唯一 Client ID。
- PC 与 DTU 使用不同账号。
- 禁止匿名访问。
- 密码使用随机高强度值，并通过安全渠道交接。
- 不要在 README、截图、脚本或 Git 历史中保存真实密码。
- 生产部署启用 TLS，并设置证书轮换流程。

## Topic 约定

推荐：

```text
cone/<DEVICE_ID>/command
cone/<DEVICE_ID>/status
```

当前历史脚本默认使用 `cone1d`。为保持现有系统可复现，PC 与 M100M 必须暂时保持一致；切换到推荐格式时应一次性更新 PC、M100M、ACL 和文档。

## 部署步骤

1. 选择本地、校园网或云服务器部署位置。
2. 安装指定 EMQX 版本并记录下载来源和校验值。
3. 修改默认 Dashboard 管理密码。
4. 配置 MQTT Listener；公网场景优先启用 TLS。
5. 创建 PC 和 DTU 的独立认证账号。
6. 设置按设备隔离的发布/订阅 ACL。
7. 只开放必要端口，并限制 Dashboard 来源。
8. 用两个普通 MQTT 客户端验证认证与 ACL。
9. 配置 M100M 并确认 Dashboard 可看到唯一 Client ID。
10. 最后配置 PC 控制程序。

## 验证清单

- PC 可以连接并发布自己的 command Topic。
- PC 不能发布其他设备 Topic。
- M100M 可以订阅自己的 command Topic。
- M100M 不能订阅其他设备 Topic。
- Retain 为 false。
- 重复 Client ID 会被发现并修正。
- Broker 重启后认证和 ACL 仍有效。
- 日志中没有持续认证失败或异常断连。

## 配置文件与数据

当前 Windows 包中：

- `etc/emqx.conf`：静态覆盖配置。
- `data/configs/cluster.hocon`：Dashboard/API 写入的运行配置。
- `data/mnesia/`：运行数据库，可能包含账号和状态。
- `log/`：运行日志。
- `etc/certs/`：示例或部署证书。

上传仓库时不要直接提交这些运行目录。只导出经过脱敏、可审查的配置模板，例如：

```text
server/
├── README.md
└── config/
    └── emqx.example.conf
```

## 部署交接记录

| 项目 | 记录 |
| --- | --- |
| EMQX 版本 | 5.3.2 或实际部署版本 |
| 部署系统 | 待填写 |
| MQTT 传输 | TCP / TLS |
| Dashboard 访问范围 | 待填写 |
| Topic 规则 | `cone/<DEVICE_ID>/...` |
| 备份位置 | 待填写，不写密码 |
| 最后验证日期 | YYYY-MM-DD |
| 对应 Git 提交 | commit SHA |

## 当前安全提醒

历史 PC 脚本和 EMQX 运行目录中存在测试凭据及运行状态。仓库发布前必须删除这些数据并轮换所有曾经暴露的密码，仅修改 README 不代表凭据已经安全。

返回[项目主页](../README.md)。
