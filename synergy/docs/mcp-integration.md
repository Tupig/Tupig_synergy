# MCP 集成方案 / MCP Integration Plan

> **文档版本**: v1.0  
> **编制日期**: 2026-09-20  
> **状态**: 规划中

---

## 1. 概述 / Overview

### 1.1 什么是 MCP

Model Context Protocol (MCP) 是一种标准化协议，让 AI 助手能够安全地连接外部数据源和工具。通过 MCP 服务器，AI 可以：

- 访问系统资源和监控数据
- 管理文件和配置
- 执行安全扫描
- 收集可观测性数据

### 1.2 TuPig Synergy 接入 MCP 的价值

| 价值维度 | 具体收益 |
|----------|----------|
| **智能监控** | AI 实时分析系统资源使用，自动发现性能瓶颈 |
| **配置管理** | AI 辅助管理跨机器配置，自动同步设置 |
| **安全审计** | AI 扫描配置漏洞，检测异常行为 |
| **故障诊断** | AI 分析日志和网络状态，快速定位问题 |
| **运维自动化** | AI 自动执行常规维护任务 |

---

## 2. 推荐 MCP 服务选型 / Recommended MCP Services

### 2.1 系统监控 MCP

#### mcp-system-info

**用途**: 监控 TuPig Synergy 运行时的系统资源

| 工具 | 功能 | TuPig Synergy 用途 |
|------|------|-------------------|
| `get_cpu_info_tool` | CPU 使用率 | 检测键鼠共享对 CPU 的影响 |
| `get_memory_info_tool` | 内存统计 | 监控内存泄漏或异常占用 |
| `get_disk_info_tool` | 磁盘信息 | 检查配置文件和日志存储 |
| `get_network_info_tool` | 网络状态 | 监控网络连接和延迟 |
| `get_process_list_tool` | 进程列表 | 追踪 synergy 进程状态 |

**安装方式**:
```bash
# 推荐使用 uvx
uvx install mcp-system-info

# 或使用 pip
pip install mcp-system-info
```

**Claude Desktop 配置**:
```json
{
  "mcpServers": {
    "system-info": {
      "command": "uvx",
      "args": ["mcp-system-info"]
    }
  }
}
```

---

### 2.2 文件系统 MCP

#### @modelcontextprotocol/server-filesystem

**用途**: 安全管理 TuPig Synergy 的配置文件和日志

| 工具 | 功能 | TuPig Synergy 用途 |
|------|------|-------------------|
| `read_text_file` | 读取文件 | 读取 Synergy.conf 配置 |
| `write_file` | 写入文件 | 更新配置项 |
| `list_directory` | 列出目录 | 查看日志文件列表 |
| `search_files` | 搜索文件 | 快速定位配置项 |
| `get_file_info` | 获取文件信息 | 检查文件权限和时间 |

**安装方式**:
```bash
# 使用 npx
npx -y @modelcontextprotocol/server-filesystem /path/to/allowed/dir

# 使用 Docker
docker build -t mcp/filesystem -f src/filesystem/Dockerfile .
```

**配置示例**:
```json
{
  "mcpServers": {
    "filesystem": {
      "command": "npx",
      "args": [
        "-y",
        "@modelcontextprotocol/server-filesystem",
        "~/.config/TuPig Synergy"
      ]
    }
  }
}
```

---

### 2.3 安全扫描 MCP

#### mcp-security-scan

**用途**: 扫描 TuPig Synergy 的 MCP 配置安全性

| 检查项 | 功能 | TuPig Synergy 用途 |
|--------|------|-------------------|
| 认证检查 | 检测远程服务器无认证 | 确保 MCP 服务安全 |
| 凭证暴露 | 发现硬编码的 API 密钥 | 防止密钥泄露 |
| SSRF 风险 | 识别服务器端请求伪造 | 保护网络边界 |
| 权限检查 | 检测文件系统访问限制 | 限制敏感文件访问 |

**安装方式**:
```bash
npm install -g mcp-security-scan
```

**使用方式**:
```bash
# 扫描 Claude Desktop 配置
mcp-security-scan --client claude-desktop

# 扫描自定义配置
mcp-security-scan --config /path/to/mcp-config.json
```

---

### 2.4 可观测性 MCP

#### observability-mcp

**用途**: 统一监控 TuPig Synergy 的所有 MCP 服务器

| 工具 | 功能 | TuPig Synergy 用途 |
|------|------|-------------------|
| `query_metrics` | 查询 Prometheus 指标 | 分析性能趋势 |
| `query_logs` | 查询 Loki 日志 | 聚合日志分析 |
| `detect_anomalies` | 异常检测 | 自动发现异常模式 |
| `get_topology` | 获取拓扑结构 | 可视化服务依赖 |

**安装方式**:
```bash
git clone https://github.com/ThoTischner/observability-mcp.git
cd observability-mcp
docker compose --profile demo up --build
```

**配置示例**:
```json
{
  "mcpServers": {
    "observability": {
      "command": "npx",
      "args": ["@thotischner/observability-mcp"]
    }
  }
}
```

---

## 3. 集成架构 / Integration Architecture

### 3.1 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                     TuPig Synergy 应用层                        │
├─────────────────────────────────────────────────────────────────┤
│   GUI (Qt6)   │   CLI 工具   │   守护进程   │   核心库         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      MCP 客户端层                                │
├─────────────────────────────────────────────────────────────────┤
│  • 系统监控 MCP  │  • 文件系统 MCP  │  • 安全扫描 MCP          │
│  • 可观测性 MCP  │  • 网络监控 MCP  │  • 日志管理 MCP          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      MCP 服务器层                                │
├─────────────────────────────────────────────────────────────────┤
│  mcp-system-info  │  server-filesystem  │  mcp-security-scan   │
│  observability-mcp │  网络诊断工具       │  日志聚合器          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      系统资源层                                  │
├─────────────────────────────────────────────────────────────────┤
│  CPU/内存  │  磁盘/网络  │  进程/服务  │  配置文件/日志        │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 数据流

```
用户请求 → AI 助手 → MCP 客户端 → MCP 服务器 → 系统资源
                ↑                                    │
                └────────────── 响应 ←───────────────┘
```

---

## 4. 具体集成场景 / Integration Scenarios

### 4.1 智能性能监控

**场景**: AI 自动检测 TuPig Synergy 性能问题

```bash
# AI 可以执行的查询
"当前 CPU 使用率是多少？"
"内存占用是否正常？"
"网络延迟是多少？"
"synergy 进程是否在运行？"
```

**工作流程**:
1. 用户询问系统状态
2. AI 调用 `mcp-system-info` 获取实时数据
3. AI 分析数据，识别异常
4. AI 提供优化建议

### 4.2 配置文件管理

**场景**: AI 辅助管理跨机器配置

```bash
# AI 可以执行的操作
"读取当前配置"
"更新服务器 IP 地址"
"备份配置文件"
"比较两台机器的配置差异"
```

**工作流程**:
1. 用户请求修改配置
2. AI 调用 `server-filesystem` 读取配置
3. AI 验证配置格式
4. AI 写入新配置并备份旧配置

### 4.3 安全审计

**场景**: AI 自动检测配置漏洞

```bash
# AI 可以执行的检查
"扫描 MCP 配置安全性"
"检查是否有硬编码的密钥"
"验证 TLS 证书配置"
"检测异常网络连接"
```

**工作流程**:
1. 用户请求安全检查
2. AI 调用 `mcp-security-scan` 扫描配置
3. AI 分析扫描结果
4. AI 提供修复建议

### 4.4 故障诊断

**场景**: AI 快速定位连接问题

```bash
# AI 可以执行的诊断
"检查 24800 端口是否监听"
"测试网络连接"
"分析日志中的错误"
"检查防火墙规则"
```

**工作流程**:
1. 用户报告连接问题
2. AI 调用多个 MCP 服务器收集数据
3. AI 分析数据，定位问题根源
4. AI 提供解决方案

---

## 5. 优先级与实施计划 / Priority & Implementation Plan

### 5.1 优先级排序

| 优先级 | MCP 服务 | 理由 | 实施时间 |
|--------|----------|------|----------|
| **P0** | mcp-system-info | 监控系统资源，检测性能问题 | 第 1 周 |
| **P0** | server-filesystem | 安全管理配置文件 | 第 1 周 |
| **P1** | mcp-security-scan | 安全审计，检测配置漏洞 | 第 2 周 |
| **P1** | observability-mcp | 统一监控，集中日志 | 第 2 周 |
| **P2** | mcp-system-monitor | 详细系统诊断 | 第 3 周 |
| **P2** | 网络监控 MCP | 网络连接质量监控 | 第 3 周 |

### 5.2 实施步骤

#### 第 1 阶段: 基础监控 (第 1 周)

1. **安装 mcp-system-info**
   ```bash
   uvx install mcp-system-info
   ```

2. **配置 Claude Desktop**
   ```json
   {
     "mcpServers": {
       "system-info": {
         "command": "uvx",
         "args": ["mcp-system-info"]
       }
     }
   }
   ```

3. **测试集成**
   ```bash
   echo '{"name": "get_cpu_info_tool", "arguments": {}}' | uvx mcp-system-info
   ```

#### 第 2 阶段: 配置管理 (第 1 周)

1. **安装 server-filesystem**
   ```bash
   npx -y @modelcontextprotocol/server-filesystem ~/.config/TuPig\ Synergy
   ```

2. **配置访问权限**
   ```json
   {
     "mcpServers": {
       "filesystem": {
         "command": "npx",
         "args": [
           "-y",
           "@modelcontextprotocol/server-filesystem",
           "~/.config/TuPig Synergy"
         ]
       }
     }
   }
   ```

#### 第 3 阶段: 安全扫描 (第 2 周)

1. **安装 mcp-security-scan**
   ```bash
   npm install -g mcp-security-scan
   ```

2. **运行安全扫描**
   ```bash
   mcp-security-scan --client claude-desktop
   ```

#### 第 4 阶段: 可观测性 (第 2 周)

1. **部署 observability-mcp**
   ```bash
   git clone https://github.com/ThoTischner/observability-mcp.git
   cd observability-mcp
   docker compose --profile demo up --build
   ```

2. **配置 Prometheus 和 Loki**
   ```yaml
   # ~/.observability-mcp/sources.yaml
   sources:
     - name: prometheus
       type: prometheus
       url: http://localhost:9090
     - name: loki
       type: loki
       url: http://localhost:3100
   ```

---

## 6. 验证清单 / Verification Checklist

- [ ] mcp-system-info 安装成功
- [ ] server-filesystem 配置正确
- [ ] mcp-security-scan 扫描通过
- [ ] observability-mcp 部署完成
- [ ] 所有 MCP 服务器正常运行
- [ ] AI 助手能够调用所有工具
- [ ] 性能监控数据准确
- [ ] 配置文件管理正常
- [ ] 安全扫描无误报
- [ ] 日志聚合正常工作

---

## 7. 注意事项 / Considerations

### 7.1 安全性

- 限制 MCP 服务器的文件系统访问权限
- 使用 TLS 加密 MCP 通信
- 定期更新 MCP 服务器版本
- 监控 MCP 服务器的访问日志

### 7.2 性能

- 合理设置监控频率，避免过度采集
- 使用缓存减少重复查询
- 监控 MCP 服务器本身的资源使用
- 设置合理的超时时间

### 7.3 兼容性

- 确保 MCP 服务器支持所有目标平台
- 测试不同操作系统的行为差异
- 验证与现有工具的集成
- 保持向后兼容性

---

## 8. 相关资源 / Related Resources

- [MCP 官方文档](https://modelcontextprotocol.io/)
- [MCP 服务器注册表](https://registry.modelcontextprotocol.io/)
- [mcp-system-info](https://github.com/dknell/mcp-system-info)
- [server-filesystem](https://github.com/modelcontextprotocol/servers)
- [mcp-security-scan](https://github.com/cc-fuyu/mcp-security-scan)
- [observability-mcp](https://github.com/ThoTischner/observability-mcp)

---

> **注意**: 本文档随 MCP 生态发展持续更新。
