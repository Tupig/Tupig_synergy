import fs from 'node:fs';

/**
 * 读取 localization.json，返回解析后的对象
 */
export function loadLocalization(jsonPath) {
  if (!fs.existsSync(jsonPath)) {
    throw new Error(`localization.json 不存在: ${jsonPath}`);
  }
  return JSON.parse(fs.readFileSync(jsonPath, 'utf8'));
}

/**
 * 对单个映射项执行失效检测（与 C++ std::regex 语义对齐）。
 * 规则（对应 GitHubDesktop2Chinese.cpp 的 --invalidcheck）：
 *  - item[0] 为查找正则，为空跳过
 *  - item[2]（可选）为第三个参数正则，需额外匹配
 * 返回 { ok, errors: [{ reason, pattern }] }，reason 为 'not-found' | 'regex-error'
 */
export function checkEntry(jsText, item) {
  const errors = [];
  const patterns = [];
  if (item && typeof item[0] === 'string') {
    patterns.push(item[0]);
  }
  if (item && item.length >= 3 && typeof item[2] === 'string') {
    patterns.push(item[2]);
  }

  for (const p of patterns) {
    if (!p || p === '""') continue;
    try {
      const re = new RegExp(p);
      if (!re.test(jsText)) {
        errors.push({ reason: 'not-found', pattern: p });
      }
    } catch {
      errors.push({ reason: 'regex-error', pattern: p });
    }
  }
  return { ok: errors.length === 0, errors };
}

/**
 * 遍历映射数组，对每个条目做失效检测。
 * 返回 { total, ok, failed, summary }，failed 为失效条目（含来源数组名）。
 */
export function checkInvalid(localization, mainJsText, rendererJsText) {
  const failed = [];
  let total = 0;
  let okCount = 0;

  const checkArray = (arrayName, jsText) => {
    const arr = localization[arrayName];
    if (!Array.isArray(arr)) return;
    for (let i = 0; i < arr.length; i++) {
      total++;
      const { ok, errors } = checkEntry(jsText, arr[i]);
      if (ok) {
        okCount++;
      } else {
        failed.push({
          array: arrayName,
          index: i,
          item: arr[i],
          errors,
        });
      }
    }
  };

  checkArray('main', mainJsText);
  checkArray('renderer', rendererJsText);
  checkArray('main_dev', mainJsText);
  checkArray('renderer_dev', rendererJsText);

  // 检测 select 中的替换项（对应 C++ 的 invalidcheck 对 select 的遍历）
  const selects = localization.select;
  if (Array.isArray(selects)) {
    for (let s = 0; s < selects.length; s++) {
      const sel = selects[s];
      const replaces = sel?.replace;
      if (!Array.isArray(replaces)) continue;
      const targetJs = sel.replaceFile === 'main.js' ? mainJsText : rendererJsText;
      for (let j = 0; j < replaces.length; j++) {
        total++;
        const { ok, errors } = checkEntry(targetJs, replaces[j]);
        if (ok) {
          okCount++;
        } else {
          failed.push({
            array: `select[${s}].replace`,
            index: j,
            item: replaces[j],
            errors,
          });
        }
      }
    }
  }

  return { total, ok: okCount, failed, failedCount: failed.length };
}