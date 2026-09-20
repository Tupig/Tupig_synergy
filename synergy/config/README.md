# TuPig Synergy MCP 配置 / MCP Configuration

本目录包含 TuPig Synergy 的 MCP (Model Context Protocol) 配置文件。

## 文件说明 / File Descriptions

| 文件 | 用途 | 平台 |
|------|------|------|
| `claude_desktop_config.json` | Claude Desktop MCP 配置模板 | 全平台 |

## 快速开始 / Quick Start

### 1. 安装 MCP 依赖

```bash
# 系统监控 MCP
uvx install mcp-system-info

# 文件系统 MCP
npm install -g @modelcontextprotocol/server-filesystem

# 安全扫描 MCP
npm install -g mcp-security-scan
```

### 2. 配置 Claude Desktop

将 `claude_desktop_config.json` 的内容复制到 Claude Desktop 配置文件：

- **macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
- **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`
- **Linux**: `~/.config/claude/claude_desktop_config.json`

### 3. 验证安装

重启 Claude Desktop，检查 MCP 服务器是否正常连接。

## 自定义配置 / Custom Configuration

### 修改监控路径

编辑 `claude_desktop_config.json` 中的 `filesystem` 服务器配置：

```json
"filesystem": {
  "command": "npx",
  "args": [
    "-y",
    "@modelcontextprotocol/server-filesystem",
    "/your/custom/path"
  ]
}
```

### 调整监控频率

修改 `system-info` 服务器的环境变量：

```json
"env": {
  "SYSINFO_CACHE_TTL": "30"
}
```

## 故障排查 / Troubleshooting

### MCP 服务器未启动

1. 检查依赖是否安装：`which uvx`、`which npx`
2. 查看 Claude Desktop 日志
3. 手动测试 MCP 服务器：`uvx mcp-system-info`

### 权限错误

确保配置文件路径有正确的读写权限：

```bash
chmod 600 ~/.config/claude/claude_desktop_config.json
```

## 相关文档 / Related Documentation

- [MCP 集成方案](../docs/mcp-integration.md)
- [MCP 官方文档](https://modelcontextprotocol.io/)
- [Claude Desktop MCP 配置](https://docs.anthropic.com/en/docs/claude-desktop/mcp)
