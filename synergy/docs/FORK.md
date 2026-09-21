# Fork 维护说明（上游同步指南）

本仓库是 [deskflow/deskflow](https://github.com/deskflow/deskflow) 的 fork，
代码位于 `synergy/` 子目录，另含 Symless 商业层残留（`synergy/extra/`，已去激活）。

## Fork 点

- 上游仓库：`https://github.com/deskflow/deskflow.git`（remote 名 `upstream`）
- Fork 基点：`8ed7a3efe1a453f024b3018882470c5d1df5d003`（2026-06-05，
  `refactor(FingerprintPreview): Adjust width...`）
- 依据：与基点做全树 `--name-only` 对比，差异 261 个文件为最低点；
  下一个窗口的 `4e121dac`（2026-06-09，版权更名，445 个文件）之后差异跳升至 630。
- 这 261 个文件的常驻差异 ≈ Symless 私有补丁（extra/ 88、图标/品牌资源、
  许可证桩代码、GUI 钩子接线）+ 本 fork 自身改动，属预期，不是定位错误。

## 历史嫁接（git replace --graft）

本仓库首个提交 `ad2c280` 是 squash 快照，与上游无共同祖先。已用 graft
把它接到上游历史上，使 `git merge-base` / 三方合并可用：

```bash
# 一次性设置（clone 后每个本地仓库都要执行）
git remote add upstream https://github.com/deskflow/deskflow.git
git fetch upstream
git fetch origin 'refs/replace/*:refs/replace/*'   # 直接获取嫁接引用（推荐）
# 或手动重建：
git replace --graft ad2c280 8ed7a3efe1a453f024b3018882470c5d1df5d003
```

验证：`git log` 应能看到 `ad2c280` 之下衔接上游历史；
`git merge-base HEAD upstream/master` 应输出基点 commit。

## 同步上游（subtree 合并）

上游文件在仓库根，本项目在 `synergy/` 子目录，合并需 subtree 策略：

```bash
git fetch upstream
git merge -Xsubtree=synergy upstream/master
# 预期冲突面 = 上述 261 个 fork 改动文件 + 双方碰过的热文件
```

注意：

- 对上游文件的修改越小，未来合并冲突越少；新功能优先加在新文件/目录。
- `git replace` 不改变 commit hash，可随时 `git replace -d ad2c280` 撤销。
- 若要做大范围清理（如 mt/ → std 迁移），请评估对合并冲突面的影响。
