# auto-maintain — localization.json 自动维护工具

自动维护 GitHubDesktop2Chinese 的 `json/localization.json`：

1. **失效检测** (`check`)：下载最新 GitHub Desktop，对现有映射逐条检测是否能匹配到 `main.js` / `renderer.js`，找出失效（匹配不到）的映射项。
2. **未翻译候选提取** (`extract`)：从最新 GitHub Desktop 中提取尚未被现有映射覆盖的英文 UI 文案候选。

> 该工具**只产出报告，不直接修改** `localization.json`。所有改动仍需人工确认，避免正则写坏导致 GitHub Desktop 无法打开。

## 运行环境

- Node.js >= 18
- 解压工具：Linux/macOS 需 `unzip`（或 `tar`），Windows 自带 `tar`

## 用法

```bash
cd GitHubDesktop2Chinese/tools/auto-maintain

# 失效检测（默认使用 ./json/localization.json）
node src/index.js check

# 未翻译候选提取
node src/index.js extract

# 两者都跑，并输出报告到 workdir/report.md
node src/index.js check extract --write-report

# 指定映射文件路径
node src/index.js check --json /path/to/localization.json

# 本地调试：复用已有的 main.js/renderer.js（跳过 328MB 下载）
node src/index.js check extract --workdir /tmp/ghdesktop-auto-maintain --keep-js
```

## 参数

| 参数 | 说明 |
| --- | --- |
| `check` / `extract` / `all` | 子命令，可组合（如 `check extract`） |
| `--json <path>` | localization.json 路径（默认 `json/localization.json`） |
| `--workdir <dir>` | 下载/解压工作目录（默认系统临时目录） |
| `--keep-js` | 复用 workdir 中已有的 `main.js`/`renderer.js`，跳过下载 |
| `--top <n>` | extract 输出候选条数（默认 60） |
| `--write-report` | 将完整报告写入 `workdir/report.md` |
| `--no-color` | 关闭彩色输出 |

## 工作原理

### 数据源

从 `desktop/desktop` 的最新 release 下载 `GitHub.Desktop-x64.zip`（约 328MB）。
zip 内含 `GitHub Desktop.app/Contents/Resources/app/{main.js,renderer.js}`，即打包前（未 asar 压缩）的 Electron 资源，可直接读取。

### 失效检测

与 C++ 主程序的 `--invalidcheck` 逻辑对齐（`GitHubDesktop2Chinese.cpp`）：

- 对 `main`/`renderer`/`main_dev`/`renderer_dev` 每个数组的每一项：
  - `item[0]` 作为正则对对应 JS 文本执行 `test`
  - 若 `item[2]`（第三个查找参数）存在，也一并测试
  - 匹配不到记为 `not-found`；正则编译失败记为 `regex-error`

### 未翻译候选提取

1. 从 `main.js`/`renderer.js` 提取双引号/单引号字符串字面量
2. `isLikelyUiText()` 过滤明显非 UI 文案（内部库消息、URL、路径、错误信息、颜色、正则、模板串、拼接碎片等）
3. 用现有映射的所有正则（含 `select` 的 `replace` 项）做覆盖检测，被任一正则匹配到的视为"已翻译"并排除
4. 剩余候选按出现次数排序输出

> 提取是启发式的，会包含少量噪音（如库内部提示语）。候选用于辅助维护，不代表一定需要翻译。

## CI 自动维护

工作流位于 `../../.github/workflows/ghdesktop2chinese-auto-maintain.yml`（仓库根目录）：

- **触发**：每天 04:00 UTC 定时；手动 `workflow_dispatch`；或推送变更 `localization.json` / 本工具代码时
- **流程**：查询最新版本 → 缓存 zip（key 绑定版本号）→ 跑 `check extract` → 生成报告 → 创建/更新 `[自动维护]` 标签的 Issue
- **自动关 Issue**：当失效项降为 0 时，自动关闭历史维护 Issue
- **不自动合并**：所有映射改动仍需人工确认，避免破坏 GitHub Desktop

> ⚠️ zip 缓存 key 绑定 GitHub Desktop 版本号，发新版后自动失效并重新下载。

## 目录结构

```
tools/auto-maintain/
├── package.json
├── src/
│   ├── index.js           # CLI 入口
│   ├── fetch.js           # 获取/下载/解压最新 GitHub Desktop，提取 main.js/renderer.js
│   ├── check-invalid.js   # 失效检测
│   └── extract-new.js     # 未翻译候选提取
```