import fs from 'node:fs';
import path from 'node:path';
import { execSync } from 'node:child_process';
import { pipeline } from 'node:stream/promises';
import { Readable } from 'node:stream';

const GITHUB_DESKTOP_REPO = 'desktop/desktop';
const API_BASE = 'https://api.github.com';
const ASSET_PATTERN = /^GitHub\.Desktop-(x64|arm64)\.zip$/;

/**
 * 获取最新 GitHub Desktop 的 release 信息
 * 返回 { tag, version, zipAssetUrl }
 */
export async function getLatestRelease() {
  const res = await fetch(`${API_BASE}/repos/${GITHUB_DESKTOP_REPO}/releases/latest`, {
    headers: { 'User-Agent': 'githubdesktop2chinese-auto-maintain' },
  });
  if (!res.ok) {
    throw new Error(`获取 GitHub Desktop 最新 release 失败: HTTP ${res.status}`);
  }
  const data = await res.json();
  const tag = data.tag_name; // 形如 release-3.6.6
  const version = tag.replace(/^release-/, '');
  const asset = data.assets.find((a) => ASSET_PATTERN.test(a.name) && a.name.includes('x64'))
    ?? data.assets.find((a) => ASSET_PATTERN.test(a.name));
  if (!asset) {
    throw new Error(`未在 release ${tag} 中找到 ${ASSET_PATTERN} 资产`);
  }
  return { tag, version, assetName: asset.name, zipUrl: asset.browser_download_url, zipSize: asset.size };
}

/**
 * 下载 release zip（支持断点续传），返回 zip 路径
 */
export async function downloadZip(url, destDir) {
  fs.mkdirSync(destDir, { recursive: true });
  const zipPath = path.join(destDir, 'github-desktop.zip');
  const tmpPath = zipPath + '.part';

  // 已经完整下载则跳过
  if (fs.existsSync(zipPath)) {
    return zipPath;
  }

  // 支持 Range 续传
  const headers = { 'User-Agent': 'githubdesktop2chinese-auto-maintain' };
  if (fs.existsSync(tmpPath)) {
    const size = fs.statSync(tmpPath).size;
    headers.Range = `bytes=${size}-`;
  }
  const res = await fetch(url, { headers });
  if (res.status !== 200 && res.status !== 206) {
    throw new Error(`下载 GitHub Desktop 失败: HTTP ${res.status}`);
  }

  const file = fs.createWriteStream(tmpPath, { flags: 'a' });
  await pipeline(Readable.fromWeb(res.body), file);
  fs.renameSync(tmpPath, zipPath);
  return zipPath;
}

/**
 * 解压 zip，提取 main.js 和 renderer.js
 * zip 内含 "GitHub Desktop.app/Contents/Resources/app/{main,renderer}.js"
 * 返回 { mainJsPath, rendererJsPath, appDir }
 */
export function extractJs(zipPath, workDir) {
  const extractDir = path.join(workDir, 'extracted');
  fs.rmSync(extractDir, { recursive: true, force: true });
  fs.mkdirSync(extractDir, { recursive: true });

  // 优先用系统 unzip / tar，跨平台
  try {
    execSync(`unzip -q -o "${zipPath}" -d "${extractDir}"`, { stdio: 'pipe' });
  } catch {
    execSync(`tar -xf "${zipPath}" -C "${extractDir}"`, { stdio: 'pipe' });
  }

  const appDir = findAppDir(extractDir);
  if (!appDir) {
    throw new Error('解压后未找到 app 资源目录');
  }
  const mainJsPath = path.join(appDir, 'main.js');
  const rendererJsPath = path.join(appDir, 'renderer.js');
  if (!fs.existsSync(mainJsPath) || !fs.existsSync(rendererJsPath)) {
    throw new Error(`app 目录中缺少 main.js / renderer.js: ${appDir}`);
  }
  return { mainJsPath, rendererJsPath, appDir };
}

function findAppDir(root) {
  const SKIP = new Set(['node_modules', 'Frameworks', 'MacOS', 'copilot', 'git', 'static']);
  const walk = (dir) => {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
      if (!entry.isDirectory()) continue;
      if (SKIP.has(entry.name)) continue;
      const full = path.join(dir, entry.name);
      if (entry.name === 'app' && fs.existsSync(path.join(full, 'main.js')) && fs.existsSync(path.join(full, 'renderer.js'))) {
        return full;
      }
      const deeper = walk(full);
      if (deeper) return deeper;
    }
    return null;
  };
  return walk(root);
}

/**
 * 一键流程：获取最新版 -> 下载 -> 解压 -> 提取 JS
 */
export async function fetchLatest(workDir) {
  const release = await getLatestRelease();
  const zipPath = await downloadZip(release.zipUrl, workDir);
  const js = extractJs(zipPath, workDir);
  return { ...release, ...js };
}