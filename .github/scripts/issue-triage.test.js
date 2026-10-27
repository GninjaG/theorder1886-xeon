// SPDX-License-Identifier: GPL-2.0-or-later
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const {formFields, plannedLabels, answered, LABEL_COLORS} = require('./issue-triage');
const report = (category, platform = 'Windows 11', log = '_No response_') =>
  '### Port version\nwindows-v0.7.9\n\n### Category\n' + category +
  '\n\n### Operating system\n' + platform + '\n\n### Log / diagnostic report\n' + log + '\n';
test('classifies a graphics report missing logs', () => {
  assert.deepEqual(plannedLabels(report('Graphics or visual glitch')), {
    add: ['area:graphics', 'needs:log', 'platform:windows'], remove: [],
  });
});
test('recognizes Linux performance logs', () => {
  const body = report('Performance (FPS, stutter, CPU/GPU/RAM)', 'Linux',
    '[last_run.log](https://github.com/user-attachments/files/123/last_run.log)');
  assert.deepEqual(plannedLabels(body), {add: ['area:performance', 'platform:linux'], remove: []});
});
test('updates managed labels without disturbing manual labels', () => {
  assert.deepEqual(
    plannedLabels(report('Controls or input', 'SteamOS'),
      ['bug', 'priority:P0', 'area:graphics', 'platform:windows', 'needs:log']),
    {add: ['area:input', 'platform:steamos'],
     remove: ['area:graphics', 'needs:log', 'platform:windows']});
});
test('is idempotent', () => {
  assert.deepEqual(plannedLabels(report('Crash or freeze'),
    ['area:crash', 'needs:log', 'platform:windows']), {add: [], remove: []});
});
test('keeps labels on unstructured legacy reports', () => {
  assert.deepEqual(plannedLabels('An old report without structured fields',
    ['area:graphics', 'bug']), {add: [], remove: []});
});
test('ignores headings embedded in code fences', () => {
  const fence = String.fromCharCode(96).repeat(3);
  const body = '### Category\r\nCrash or freeze\r\n### Log / diagnostic report\r\n' +
    fence + 'text\r\n### Category\r\nfake\r\n' + fence + '\r\n';
  assert.equal(formFields(body).get('Category'), 'Crash or freeze');
  assert.equal(plannedLabels(body).add.includes('needs:log'), false);
});
test('missing response markers are not valid logs', () => {
  for (const marker of ['', 'None', '_No response_', 'No response', 'N/A'])
    assert.equal(answered(marker), false);
});
test('label colors are predefined', () => {
  for (const color of Object.values(LABEL_COLORS))
    assert.match(color, /^[0-9a-f]{6}$/);
});
