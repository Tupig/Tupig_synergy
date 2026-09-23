# Plan — B+D1+E2+F1+G1 (confirmed 2026-09-23)

> **Language / 语言**: [English](#english) | [中文](#中文)

## English

### Decisions locked

| Code | Choice |
|---|---|
| B | Qt default + Step 5 + Step 6 delete Legacy |
| D1 | Synergy icon theme only |
| E2 | U-09 paths by design |
| F1 | Close U-17/18/19 this round |
| G1 | User runs two-machine drag; agent supplies checklist |

### Phase 0 — Naming / icons / docs

1. D1: fold deskflow SVGs into `synergy.qrc` as aliases; drop `deskflow.qrc` from GUI link.
2. U-17: display name / desktop / metainfo → `TuPig Synergy`.
3. U-18: harden i18n coupling; keep internal `deskflow` names.
4. U-09 / U-19: document by design.
5. G1: checklist in `docs/delivery.md`.

### Phase 1 — Qt adapter + TOFU (B prerequisite)

Client/Server still use `IDataSocket` + EventTypes + fingerprint DB. `NetworkTransportFactory` is unused.

1. `QtDataSocket` / `QtListenSocket` adapters (events + `IStream`).
2. Fingerprint TOFU parity with `SecureSocket`.
3. Wire apps through factory; prove with `USE_LEGACY_NETWORK=0` before flipping default.

### Phase 2 — Default Qt + Step 5 smart pointers

### Phase 3 — Step 6 delete Legacy stack

### Verify

`scripts\build.bat release` + unit tests + push per phase. User runs G1 checklist.

---

## 中文

### 已锁定决策

| 代号 | 选择 |
|---|---|
| B | Qt 默认 + Step 5 + Step 6 删除 Legacy |
| D1 | 仅保留 synergy 图标主题 |
| E2 | U-09 路径有意保留 |
| F1 | 本轮关闭 U-17/18/19 |
| G1 | 你做双机拖拽；我出清单 |

### 阶段 0 — 命名 / 图标 / 文档

### 阶段 1 — Qt 适配器 + 指纹 TOFU（B 前置）

### 阶段 2 — 默认 Qt + Step 5 智能指针

### 阶段 3 — Step 6 删除 Legacy

### 验证

每阶段 `scripts\build.bat release` + 单测 + 推送。跨屏拖拽按 `docs/delivery.md` 清单由你执行。
