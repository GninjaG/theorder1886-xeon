// SPDX-License-Identifier: GPL-2.0-or-later
// Parse issue forms only; never execute user-supplied content.
'use strict';
const CATEGORY_LABELS = Object.freeze({
  'Graphics or visual glitch': 'area:graphics',
  'Performance (FPS, stutter, CPU/GPU/RAM)': 'area:performance',
  'Crash or freeze': 'area:crash',
  'Launcher or installation': 'area:launcher',
  'Controls or input': 'area:input',
  'Audio or subtitles': 'area:audio',
  'Gameplay or progression': 'area:gameplay',
  'Save or load': 'area:saves',
  'Hardware or platform compatibility': 'area:compatibility',
});
const PLATFORM_LABELS = Object.freeze({
  'Windows 10': 'platform:windows',
  'Windows 11': 'platform:windows',
  'Linux': 'platform:linux',
  'SteamOS': 'platform:steamos',
});
const LABEL_COLORS = Object.freeze({
  'area:graphics': '5319e7', 'area:performance': '5319e7',
  'area:crash': '5319e7', 'area:launcher': '5319e7',
  'area:input': '5319e7', 'area:audio': '5319e7',
  'area:gameplay': '5319e7', 'area:saves': '5319e7',
  'area:compatibility': '5319e7',
  'platform:windows': '1d76db', 'platform:linux': '1d76db',
  'platform:steamos': '1d76db', 'needs:log': 'fbca04',
});
const REQUIRED_LOG_CATEGORIES = new Set([
  'Graphics or visual glitch',
  'Performance (FPS, stutter, CPU/GPU/RAM)',
  'Crash or freeze', 'Audio or subtitles', 'Gameplay or progression',
]);
function formFields(body) {
  const fields = new Map();
  let inFence = false, key = null, lines = [];
  const flush = () => { if (key !== null) fields.set(key, lines.join('\n').trim()); };
  for (const line of String(body ?? '').replace(/\r\n?/g, '\n').split('\n')) {
    if (/^\s*(?:\x60{3}|~{3})/.test(line)) inFence = !inFence;
    const heading = !inFence && /^### (.+?)\s*$/.exec(line);
    if (heading) { flush(); key = heading[1].trim(); lines = []; }
    else if (key !== null) lines.push(line);
  }
  flush();
  return fields;
}
function answered(value) {
  const text = String(value ?? '').trim();
  return text.length > 0 && !/^(?:_No response_|No response|None|N\/A)$/i.test(text);
}
function plannedLabels(body, existingLabels = []) {
  const fields = formFields(body);
  const category = fields.get('Category');
  const platform = fields.get('Operating system');
  const understoodCategory = Object.hasOwn(CATEGORY_LABELS, category);
  const understoodPlatform = Object.hasOwn(PLATFORM_LABELS, platform);
  const desired = new Set();
  if (understoodCategory) desired.add(CATEGORY_LABELS[category]);
  if (understoodPlatform) desired.add(PLATFORM_LABELS[platform]);
  if (REQUIRED_LOG_CATEGORIES.has(category) && !answered(fields.get('Log / diagnostic report'))) {
    desired.add('needs:log');
  }
  const managed = new Set([
    ...(understoodCategory ? Object.values(CATEGORY_LABELS) : []),
    ...(understoodPlatform ? Object.values(PLATFORM_LABELS) : []),
    ...(understoodCategory ? ['needs:log'] : []),
  ]);
  const existing = new Set(existingLabels.map(label => typeof label === 'string' ? label : label.name));
  return {
    add: [...desired].filter(label => !existing.has(label)).sort(),
    remove: [...managed].filter(label => existing.has(label) && !desired.has(label)).sort(),
  };
}
module.exports = {CATEGORY_LABELS, PLATFORM_LABELS, LABEL_COLORS, formFields, answered, plannedLabels};
