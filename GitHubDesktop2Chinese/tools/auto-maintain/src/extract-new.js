/**
 * 从 GitHub Desktop 的 main.js / renderer.js 中提取"看起来像未翻译 UI 文案"的英文字符串候选。
 *
 * 策略：
 *  1. 提取所有双引号 / 单引号字符串字面量（模板字符串噪音大，暂不纳入）
 *  2. 过滤掉明显非 UI 文案的（内部库消息、URL、路径、错误信息、颜色、正则等）
 *  3. 排除已被现有映射任一正则覆盖的候选（视为已翻译）
 *  4. 对剩余候选去重、按出现频率排序输出
 */

/**
 * 判断字符串是否像一条可翻译的英文 UI 文案。
 * 过滤规则针对 GitHub Desktop bundle 的实际噪音设计。
 */
export function isLikelyUiText(s) {
  if (typeof s !== 'string' || !s) return false;
  const len = s.length;
  if (len < 4 || len > 160) return false;

  // 必须包含大小写字母和至少一个空格（句子/短语特征）
  if (!/[A-Za-z]{2}/.test(s)) return false;
  if (!/\s/.test(s)) return false;

  // 纯 ASCII（已翻译中文的不算；含非 ASCII 的混合也不可靠，跳过）
  if (!/^[\x20-\x7E]+$/.test(s)) return false;

  // 排除明显是代码/标识符/库消息的
  if (s.includes('{') || s.includes('}')) return false;        // 模板/对象
  if (s.includes('(') || s.includes(')')) return false;        // 函数调用/正则
  if (s.includes(';') || s.includes('=')) return false;        // 语句
  if (s.includes('/')) return false;                           // 路径/注释
  if (s.includes('\\')) return false;                          // 转义/正则
  if (s.includes('`') || s.includes('$')) return false;        // 模板字符串
  if (s.includes('#')) return false;                           // 颜色/锚点
  if (s.includes('_')) return false;                           // 内部标识符
  if (/^https?:/i.test(s)) return false;                       // URL
  if (/^rgba?\(/i.test(s)) return false;                       // 颜色
  if (/^\[object/i.test(s)) return false;                      // [object X]
  if (/^(error|warning|info|debug):/i.test(s)) return false;   // 日志前缀
  if (/^(use strict|object|undefined|null|true|false)$/i.test(s)) return false;

  // 排除 JS 拼接碎片：以空白开头/结尾，或整体是小写介词/连词/短词
  if (/^\s|\s$/.test(s)) return false;
  if (/^(the|a|an|to|in|on|of|for|and|or|with|from|by|at|into|onto|over|under)$/i.test(s)) return false;
  // 首尾字母均需为字母/数字（排除 `. Want to` 这类残留片段）
  if (!/^[A-Za-z0-9].*[A-Za-z0-9]$/.test(s)) return false;

  // 排除运行时错误信息（源自库内部）
  if (/ is not (defined|a function|a constructor)$/i.test(s)) return false;
  if (/^Symbol\./i.test(s)) return false;
  if (/^(Cannot|Failed to) /i.test(s)) return false;

  // 排除日期/时间格式串（如 h:mm aaa、MMMM do, y）
  if (/^[hHmsS]+(:|\s)[aA]?/i.test(s) && !/[a-z]{4,}/.test(s)) return false;

  // 排除纯小写标识符（库内部变量名）
  if (/^[a-z][a-z0-9 -]*$/.test(s) && len < 20) return false;

  // 排除看起来像 CSS/样式片段
  if (s.includes(':') && /^(margin|padding|border|color|background|font|width|height|display|position|text|line|letter|word|box|flex|grid|opacity|z-index)/i.test(s)) return false;

  // 至少包含一个多字母英文单词（>=3 字母）
  return /[A-Za-z]{3,}/.test(s);
}

/**
 * 提取字符串字面量（双引号、单引号）。
 */
export function extractStringLiterals(jsText) {
  const out = [];
  const re = /(["'])((?:\\.|(?!\1)[^\\]){3,})\1/g;
  let m;
  while ((m = re.exec(jsText)) !== null) {
    // 还原常见转义（\n \t \" \' \\），不处理 unicode 转义以保留英文判断
    let t = m[2]
      .replace(/\\n/g, '\n')
      .replace(/\\t/g, '\t')
      .replace(/\\"/g, '"')
      .replace(/\\'/g, "'")
      .replace(/\\\\/g, '\\');
    if (isLikelyUiText(t)) {
      out.push(t);
    }
  }
  return out;
}

/**
 * 从映射数组中收集所有正则 pattern（item[0] 和可选的 item[2]），
 * 用于判断某候选是否已被现有映射覆盖。
 */
export function collectPatterns(localization) {
  const patterns = [];
  for (const arrayName of ['main', 'renderer', 'main_dev', 'renderer_dev']) {
    const arr = localization[arrayName];
    if (!Array.isArray(arr)) continue;
    for (const item of arr) {
      for (const p of [item?.[0], item?.[2]]) {
        if (typeof p === 'string' && p && p !== '""') {
          try {
            new RegExp(p);
            patterns.push(p);
          } catch {
            // 忽略非法正则
          }
        }
      }
    }
  }
  // select 中的 replace 项也纳入
  const selects = localization.select;
  if (Array.isArray(selects)) {
    for (const sel of selects) {
      const replaces = sel?.replace;
      if (!Array.isArray(replaces)) continue;
      for (const item of replaces) {
        for (const p of [item?.[0], item?.[2]]) {
          if (typeof p === 'string' && p && p !== '""') {
            try {
              new RegExp(p);
              patterns.push(p);
            } catch {
              /* ignore */
            }
          }
        }
      }
    }
  }
  return patterns;
}

/**
 * 判断候选字符串是否已被任一现有映射正则覆盖。
 */
export function isCoveredByPatterns(candidate, patterns) {
  for (const p of patterns) {
    try {
      const re = new RegExp(p);
      if (re.test(candidate)) return true;
    } catch {
      /* ignore */
    }
  }
  return false;
}

/**
 * 主流程：提取未翻译候选。
 * 返回 { candidates: [{ text, count }], patternsCount }
 */
export function extractNew(localization, mainJsText, rendererJsText) {
  const patterns = collectPatterns(localization);

  const countMap = new Map();
  for (const jsText of [mainJsText, rendererJsText]) {
    for (const lit of extractStringLiterals(jsText)) {
      countMap.set(lit, (countMap.get(lit) ?? 0) + 1);
    }
  }

  const candidates = [];
  for (const [text, count] of countMap) {
    if (isCoveredByPatterns(text, patterns)) continue;
    candidates.push({ text, count });
  }

  // 按出现次数降序，再按长度升序
  candidates.sort((a, b) => b.count - a.count || a.text.length - b.text.length);

  return { candidates, patternsCount: patterns.length };
}