import path from 'node:path';
import fs from 'node:fs';
import os from 'node:os';
import { fileURLToPath } from 'node:url';
import { fetchLatest } from './fetch.js';
import { loadLocalization, checkInvalid } from './check-invalid.js';
import { extractNew } from './extract-new.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..', '..', '..');
const DEFAULT_JSON = path.join(PROJECT_ROOT, 'json', 'localization.json');

const help = `用法:
  node src/index.js <check|extract|all> [选项]

子命令:
  check    对最新 GitHub Desktop 的 main.js/renderer.js 检测 localization.json 中的失效映射项
  extract  提取最新 GitHub Desktop 中尚未被现有映射覆盖的英文 UI 文案候选
  all      check + extract

选项:
  --json <path>      localization.json 路径（默认 ${DEFAULT_JSON}）
  --workdir <dir>    下载/解压工作目录（默认系统临时目录 ghdesktop-auto-maintain）
  --keep-js          复用已有 workdir 中的 JS 文件（跳过下载，用于本地调试）
  --no-color         关闭输出颜色
  --top <n>          extract 最多输出的候选条数（默认 60）
  --write-report     将结果写入 workdir 下的 report.md
  -h, --help         显示帮助
`;

function parseArgs(argv) {
  const args = { json: DEFAULT_JSON, top: 60, writeReport: false, keepJs: false, noColor: false, cmds: [] };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--json') args.json = argv[++i];
    else if (a === '--workdir') args.workdir = argv[++i];
    else if (a === '--top') args.top = parseInt(argv[++i], 10);
    else if (a === '--write-report') args.writeReport = true;
    else if (a === '--keep-js') args.keepJs = true;
    else if (a === '--no-color') args.noColor = true;
    else if (a === '-h' || a === '--help') { console.log(help); process.exit(0); }
    else if (['check', 'extract', 'all'].includes(a)) args.cmds.push(a);
    else throw new Error(`未知参数: ${a}`);
  }
  if (args.cmds.length === 0) args.cmds = ['all'];
  return args;
}

const c = (code, s) => `\x1b[${code}m${s}\x1b[0m`;

async function getJsFiles(args) {
  if (!args.workdir) {
    args.workdir = path.join(os.tmpdir(), 'ghdesktop-auto-maintain');
    fs.mkdirSync(args.workdir, { recursive: true });
  }
  const mainJsPath = path.join(args.workdir, 'main.js');
  const rendererJsPath = path.join(args.workdir, 'renderer.js');

  if (args.keepJs && fs.existsSync(mainJsPath) && fs.existsSync(rendererJsPath)) {
    return { mainJsPath, rendererJsPath, workdir: args.workdir, version: 'local' };
  }

  console.log(c('36', '↻ 获取最新 GitHub Desktop...'));
  const release = await fetchLatest(args.workdir);

  // 将 JS 复制到工作目录顶层，便于 --keep-js 复用
  fs.copyFileSync(release.mainJsPath, mainJsPath);
  fs.copyFileSync(release.rendererJsPath, rendererJsPath);
  console.log(c('32', `✓ 已获取 GitHub Desktop ${release.version} (${release.tag})`));
  console.log(`  main.js     -> ${mainJsPath}`);
  console.log(`  renderer.js -> ${rendererJsPath}`);
  return { mainJsPath, rendererJsPath, workdir: args.workdir, version: release.version };
}

function writeReport(file, content) {
  fs.writeFileSync(file, content, 'utf8');
}

async function main() {
  let args;
  try {
    args = parseArgs(process.argv.slice(2));
  } catch (e) {
    console.error(c('31', '✗ ' + e.message));
    console.log(help);
    process.exit(2);
  }

  if (args.cmds.length === 0) {
    console.error(c('31', '✗ 请指定子命令: check / extract / all'));
    console.log(help);
    process.exit(2);
  }

  try {
    const localization = loadLocalization(args.json);
    const { mainJsPath, rendererJsPath, workdir, version } = await getJsFiles(args);
    const mainJsText = fs.readFileSync(mainJsPath, 'utf8');
    const rendererJsText = fs.readFileSync(rendererJsPath, 'utf8');

    const report = { version, generatedAt: new Date().toISOString(), checks: null, candidates: null };

    if (args.cmds.includes('check') || args.cmds.includes('all')) {
      const result = checkInvalid(localization, mainJsText, rendererJsText);
      report.checks = result;
      console.log('');
      console.log(c('1;33', '══════ 失效检测结果 ══════'));
      console.log(`  总映射项: ${result.total}`);
      console.log(c('32', `  有效项:   ${result.ok}`));
      console.log(c(result.failedCount > 0 ? '31' : '32', `  失效项:   ${result.failedCount}`));
      for (const f of result.failed) {
        const reason = f.errors.map(e => (e.reason === 'regex-error' ? '正则错误' : '匹配不到')).join('; ');
        console.log(`  ${c('31', `[${f.array}#${f.index}]`)} ${reason}: ${f.errors[0].pattern}`);
      }
    }

    if (args.cmds.includes('extract') || args.cmds.includes('all')) {
      const result = extractNew(localization, mainJsText, rendererJsText);
      report.candidates = result;
      console.log('');
      console.log(c('1;33', `══════ 未翻译候选 (共 ${result.candidates.length} 条, 展示前 ${args.top} 条) ══════`));
      console.log(`  已加载映射正则: ${result.patternsCount} 条`);
      for (const cand of result.candidates.slice(0, args.top)) {
        console.log(`  ${c('36', `[×${cand.count}]`)} ${cand.text}`);
      }
    }

    if (args.writeReport) {
      const file = path.join(workdir, 'report.md');
      writeReport(file, renderMarkdown(report));
      console.log('');
      console.log(c('32', `✓ 报告已写入: ${file}`));
    }
  } catch (e) {
    console.error(c('31', '✗ ' + (e.stack || e.message)));
    process.exit(1);
  }
}

function renderMarkdown(report) {
  const lines = [];
  lines.push(`# 自动维护报告`);
  lines.push('');
  lines.push(`- GitHub Desktop 版本: ${report.version}`);
  lines.push(`- 生成时间: ${report.generatedAt}`);
  lines.push('');
  if (report.checks) {
    const { total, ok, failedCount, failed } = report.checks;
    lines.push(`## 失效检测`);
    lines.push('');
    lines.push(`总映射项 ${total}，有效 ${ok}，失效 ${failedCount}。`);
    lines.push('');
    lines.push(`| 数组 | 序号 | 原因 | 正则 |`);
    lines.push(`| --- | --- | --- | --- |`);
    for (const f of failed) {
      const reason = f.errors.map(e => (e.reason === 'regex-error' ? 'regex-error' : 'not-found')).join('; ');
      lines.push(`| ${f.array} | ${f.index} | ${reason} | \`${f.errors[0].pattern.replace(/\|/g, '\\|')}\` |`);
    }
    lines.push('');
  }
  if (report.candidates) {
    const { candidates, patternsCount } = report.candidates;
    lines.push(`## 未翻译候选`);
    lines.push('');
    lines.push(`基于 ${patternsCount} 条映射正则，发现 ${candidates.length} 条未被覆盖的英文文案候选。`);
    lines.push('');
    lines.push(`| 次数 | 候选文本 |`);
    lines.push(`| --- | --- |`);
    for (const cand of candidates.slice(0, 200)) {
      lines.push(`| ${cand.count} | ${cand.text.replace(/\|/g, '\\|')} |`);
    }
    lines.push('');
  }
  return lines.join('\n');
}

main();