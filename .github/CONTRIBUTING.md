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

- **Build & development workflow**: [`synergy/docs/build.md`](../synergy/docs/build.md)
  (toolchain prerequisites, `setup.bat` / `scripts/build.*`, presets, packaging).
- **Configuration reference**: [`synergy/docs/configuration.md`](../synergy/docs/configuration.md).
- **Troubleshooting**: [`synergy/docs/troubleshooting.md`](../synergy/docs/troubleshooting.md).
- **Session handoff / current status**: [`synergy/docs/HANDOFF.md`](../synergy/docs/HANDOFF.md).

Code standards:

- C++20, enforced by CMake 3.25+.
- Formatting: clang-format (Google-based), config in `.clang-format`. CI enforces
  it via `clang-format-diff` against the merge base; run
  `clang-format -i path/to/changed.cpp` locally. This repository has no
  `.pre-commit-config.yaml`.
- Tests: Qt Test + CTest (`ctest --output-on-failure`).
- Commits: Conventional Commits; one logical change per commit; commit messages
  in Chinese (see `synergy/AGENTS.md` R10).
- Windows automation is `.bat` only — no `.ps1` anywhere (AGENTS R1).

Upstream sync: this repository is grafted onto and periodically merged with
[deskflow/deskflow](https://github.com/deskflow/deskflow); internal identifiers
deliberately keep the `deskflow` spelling to stay mergeable.

Reporting:

| Purpose | Where |
|---|---|
| Bugs | [GitHub Issues](https://github.com/Tupig/TuPig_Product/issues/new?template=bug_report.yml) |
| Ideas and questions | [GitHub Discussions](https://github.com/Tupig/TuPig_Product/discussions) |
| Security vulnerabilities | [synergy/docs/security.md](../synergy/docs/security.md) — **not** public issues |

PR checklist:

- [ ] Branch targets `main`
- [ ] Commits follow Conventional Commits (Chinese messages)
- [ ] Code passes `clang-format`
- [ ] Tests pass (`ctest --output-on-failure`)
- [ ] Docs / issue tracker updated if status changed (HANDOFF §1.3)

---

## 中文

感谢您对 TuPig Synergy 的关注！错误报告、功能建议、文档改进、代码与翻译都欢迎。

在提交 issue 或 pull request 之前：

- **构建与开发工作流**：[`synergy/docs/build.md`](../synergy/docs/build.md)
  （工具链前置、`setup.bat` / `scripts/build.*`、预设、打包）。
- **配置参考**：[`synergy/docs/configuration.md`](../synergy/docs/configuration.md)。
- **故障排查**：[`synergy/docs/troubleshooting.md`](../synergy/docs/troubleshooting.md)。
- **会话交接 / 当前状态**：[`synergy/docs/HANDOFF.md`](../synergy/docs/HANDOFF.md)。

代码规范：

- C++20，由 CMake 3.25+ 强制。
- 格式化：clang-format（Google 风格），配置在 `.clang-format`。CI 通过
  `clang-format-diff` 对合并基线强制执行；本地对变更文件运行
  `clang-format -i path/to/changed.cpp`。本仓库**没有** `.pre-commit-config.yaml`。
- 测试：Qt Test + CTest（`ctest --output-on-failure`）。
- 提交：遵循 Conventional Commits；一提交一事；提交信息使用中文（见
  `synergy/AGENTS.md` R10）。
- Windows 自动化脚本只允许 `.bat` —— 全仓库禁止 `.ps1`（AGENTS R1）。

上游同步：本仓库嫁接自并定期合并
[deskflow/deskflow](https://github.com/deskflow/deskflow)；内部标识有意保留
`deskflow` 拼写以便合并。

提交渠道：

| 用途 | 位置 |
|---|---|
| Bug | [GitHub Issues](https://github.com/Tupig/TuPig_Product/issues/new?template=bug_report.yml) |
| 想法与提问 | [GitHub Discussions](https://github.com/Tupig/TuPig_Product/discussions) |
| 安全漏洞 | [synergy/docs/security.md](../synergy/docs/security.md) —— **不要**用公开 issue |

PR 检查清单：

- [ ] 分支目标为 `main`
- [ ] 提交遵循 Conventional Commits（中文信息）
- [ ] 代码通过 `clang-format`
- [ ] 测试全部通过（`ctest --output-on-failure`）
- [ ] 状态有变化时同步更新文档 / 追踪台账（HANDOFF §1.3）
