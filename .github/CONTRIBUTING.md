# TuPig Synergy Contributing Guide / 贡献指南

> **Language / 语言**: [English](#english) | [中文](#中文)
>
> GitHub looks for a contributing guide at this path. This file is the canonical
> contributing guide for the repository.
>
> GitHub 会在本路径查找贡献指南，本文件即仓库的权威贡献指南。

---

## English

Thank you for your interest in TuPig Synergy! Bug reports, feature requests,
documentation improvements, code and translations are all welcome.

Before opening an issue or a pull request:

- **Build & development workflow**: [`docs/build.md`](../docs/build.md)
  (toolchain prerequisites, `setup.bat` / `scripts/build.*`, presets, packaging).
- **Configuration, settings keys, and command-line flags**: [`docs/configuration.md`](../docs/configuration.md).
  Flags come from `CoreArgs.h` and `synergy-gui.cpp`. There is no `--debug` or `--no-daemon`.
- **Troubleshooting**: [`docs/troubleshooting.md`](../docs/troubleshooting.md).

Code standards:

- C++20, enforced by CMake 3.25+.
- Formatting: clang-format (Google-based), config in `.clang-format`. CI installs
  clang-format 20.1.0 and checks the diff against the merge base. A newer
  clang-format (for example 22) wraps the same files differently and fails that
  job. This repository has no `.pre-commit-config.yaml`.
- Tests: Qt Test + CTest. From a Windows Release build,
  `ctest --test-dir build/src/unittests -C Release --output-on-failure`.
  Linux and macOS omit `-C Release`. `ctest --test-dir build` finds nothing.
- Commits: Conventional Commits; one logical change per commit; commit messages
  in Chinese (see `AGENTS.md` R10).
- Windows automation is `.bat` only — no `.ps1` anywhere (AGENTS R1).

Upstream sync: this repository is grafted onto and periodically merged with
[deskflow/deskflow](https://github.com/deskflow/deskflow). Internal identifiers
were unified under the `synergy` name in 2026-10 (directories, namespaces,
strings); upstream references remain only where they are attribution (SPDX
headers, thanks) or historical notes.

Reporting:

| Purpose | Where |
|---|---|
| Bugs | [GitHub Issues](https://github.com/Tupig/Tupig_synergy/issues/new?template=bug_report.yml) |
| Ideas and questions | [Feature request](https://github.com/Tupig/Tupig_synergy/issues/new?template=feature_request.yml) |
| Security vulnerabilities | [docs/security.md](../docs/security.md) — **not** public issues |

PR checklist:

- [ ] Branch targets `main`
- [ ] Commits follow Conventional Commits (Chinese messages)
- [ ] Code passes `clang-format`
- [ ] Tests pass (`ctest --test-dir build/src/unittests`, plus `-C Release` on Windows)
- [ ] Docs and GitHub Issues updated if user-facing behavior changed

---

## 中文

感谢您对 TuPig Synergy 的关注！错误报告、功能建议、文档改进、代码与翻译都欢迎。

在提交 issue 或 pull request 之前：

- **构建与开发工作流**：[`docs/build.md`](../docs/build.md)
  （工具链前置、`setup.bat` / `scripts/build.*`、预设、打包）。
- **配置、设置键与命令行参数**：[`docs/configuration.md`](../docs/configuration.md)。
  参数以 `CoreArgs.h` 和 `synergy-gui.cpp` 为准。没有 `--debug`，也没有 `--no-daemon`。
- **故障排查**：[`docs/troubleshooting.md`](../docs/troubleshooting.md)。

代码规范：

- C++20，由 CMake 3.25+ 强制。
- 格式化：clang-format（Google 风格），配置在 `.clang-format`。CI 安装的是
  clang-format 20.1.0，并对照合并基线检查 diff。更新的 clang-format（例如 22）
  会把同一文件折行成另一种结果，lint 会失败。本仓库没有 `.pre-commit-config.yaml`。
- 测试：Qt Test + CTest。Windows Release 构建使用
  `ctest --test-dir build/src/unittests -C Release --output-on-failure`。
  Linux 与 macOS 去掉 `-C Release`。`ctest --test-dir build` 找不到用例。
- 提交：遵循 Conventional Commits；一提交一事；提交信息使用中文（见
  `AGENTS.md` R10）。
- Windows 自动化脚本只允许 `.bat` —— 全仓库禁止 `.ps1`（AGENTS R1）。

上游同步：本仓库嫁接自并定期合并
[deskflow/deskflow](https://github.com/deskflow/deskflow)。内部标识已于 2026-10
统一为 `synergy`（目录、命名空间、字符串）；上游引用仅保留在归属署名（SPDX、致谢）与历史说明中。

提交渠道：

| 用途 | 位置 |
|---|---|
| Bug | [GitHub Issues](https://github.com/Tupig/Tupig_synergy/issues/new?template=bug_report.yml) |
| 想法与提问 | [功能请求](https://github.com/Tupig/Tupig_synergy/issues/new?template=feature_request.yml) |
| 安全漏洞 | [docs/security.md](../docs/security.md) —— **不要**用公开 issue |

PR 检查清单：

- [ ] 分支目标为 `main`
- [ ] 提交遵循 Conventional Commits（中文信息）
- [ ] 代码通过 `clang-format`
- [ ] 测试全部通过（`ctest --test-dir build/src/unittests`，Windows 加 `-C Release`）
- [ ] 用户可见行为有变化时同步更新文档与 GitHub Issues
