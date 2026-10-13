const { chromium } = require('playwright');
const fs = require('fs');
const assert = require('assert');
const path = require('path');

(async () => {
  const root = path.resolve(__dirname, '..');
  const source = fs.readFileSync(path.join(root, 'DenDeco/WebUI.h'), 'utf8');
  const html = source.split('R"DECOHTML(')[1].split(')DECOHTML"')[0];
  const modeLabels = ['Ngủ', 'Một màu', 'Chuyển màu chậm', 'Ánh nến', 'Cầu vồng chậm', 'Ánh sáng dâng lên', 'Thở nhẹ', 'Lấp lánh', 'Cực quang', 'Biển xanh', 'Rừng xanh', 'Hoàng hôn', 'Sao băng', 'Sóng màu'];
  const modeIds = ['Sleep', 'Solid', 'SlowColor', 'Candle', 'Rainbow', 'Rise', 'Breath', 'Twinkle', 'Aurora', 'Ocean', 'Forest', 'Sunset', 'Comet', 'ColorWaves'];
  const core = fs.readFileSync(path.join(root, 'DenDeco/Core.h'), 'utf8');
  modeIds.forEach((name, id) => assert(new RegExp('\\b' + name + '\\s*=\\s*' + id + '\\b').test(core), 'Firmware mode ID drift: ' + name));
  assert(!/<(?:script|link)\b[^>]*(?:src|href)=["']https?:\/\//i.test(html), 'UI/help must be self-contained');
  const s = {
    version: '1.3.1', name: 'Đèn Deco', token: 'test-session', revision: 0,
    power: false, light: { mode: 0, r: 255, g: 96, b: 24, brightness: 5, speed: 50 },
    sleepBrightness: 5, cap: 30, ledCount: 20,
    favorites: Array.from({ length: 3 }, () => ({ valid: false, light: { mode: 0, brightness: 5 } })),
    timerActive: false, remainingMs: 0, hasSaved: true, savePending: false, saveError: '',
    ssid: 'Home', wifiOpen: false, wifiConnected: true, ip: '192.168.1.10',
    apActive: false, apSSID: 'DenDeco-TEST', hostname: 'den-deco', requestedHostname: 'den-deco',
    localUrl: 'http://den-deco.local', mdnsReady: true,
    networkStatus: 'connected', networkMessage: 'Đã kết nối WiFi nhà.', disconnectReason: 0,
    wifiDiagnostics: { status: 3, interfaceReady: true, associated: true, beginResult: 6, beginAttempts: 1, pending: false, cancelTimedOut: false, disconnectError: '', lastEvent: 7, lastDisconnectReason: 0, lastDisconnectAtMs: 0, disconnectCount: 0, powerSave: 0 },
    resources: { resetReason: 1, freeHeap: 220000, minFreeHeap: 210000, largestFreeBlock: 150000, loopStackFree: 3800 },
    planSaveDiagnostics: { kind: '', stage: 'idle', count: 0, startedAtMs: 0, totalMs: 0, lightMs: 0, planMs: 0, applyMs: 0, responseMs: 0, successful: false, lightWritten: false, planWritten: false, onlineBefore: false, onlineAfter: false, associatedBefore: false, associatedAfter: false, interfaceBefore: false, interfaceAfter: false, ipBefore: '', ipAfter: '', heapBefore: 0, heapAfter: 0, stackFree: 0 },
    uptimeMs: 120000,
    otaMaxBytes: 1310720,
    clock: { ready: false, online: true, syncing: true, utc: 0, localTime: '', localDate: '', lastSyncUtc: 0, zone: 'UTC+7' },
    schedules: { enabled: false, entries: Array.from({ length: 8 }, () => ({ enabled: false, days: 0, minute: 0, power: false, favorite: -1 })) },
    sun: {
      latitudeE6: 10776000, longitudeE6: 106700000,
      dawn: { enabled: false, days: 127, offset: 0, minutes: 30, favorite: -1, brightness: 35 },
      dusk: { enabled: false, days: 127, offset: 0, minutes: 30, favorite: -1, brightness: 35 },
      sunrise: '', sunset: ''
    },
    automation: { active: false, kind: 'dawn', brightness: 0, target: 35, remainingMs: 0, error: '' },
    scheduleSaveError: '', scheduleSaved: true
  };
  const commands = [], networkCalls = [], scheduleCalls = [], sunCalls = [], errors = [];
  let offline = false, otaCalls = 0, scanStarts = 0, scanGets = 0;
  let scanError = '', scanGetError = '', networkError = '', networkReconnect = false, networkDelay = 0;
  let commandDelay = 0, otaError = '', otaDelay = 0;
  let scheduleError = '', sunError = '', automationDelay = 0, automationAbort = '';
  let stateGets = 0;
  const networks = [
    { ssid: 'Home 2.4G', rssi: -35, channel: 6, secure: true, security: 'WPA2' },
    { ssid: 'Cafe mở', rssi: -60, channel: 1, secure: false, security: 'OPEN' },
    { ssid: '<img src=x onerror=alert(1)>', rssi: -85, channel: 11, secure: true, security: 'WPA3' }
  ];
  const browser = await chromium.launch({
    headless: true,
    executablePath: process.env.DECO_BROWSER || '/usr/bin/chromium',
    args: ['--no-sandbox']
  });
  const page = await browser.newPage({ viewport: { width: 390, height: 844 } });
  page.on('pageerror', e => errors.push(e.message));
  await page.addInitScript(() => {
    // HTTP on a LAN usually lacks Clipboard API. Exercise its explicit fallback.
    window.__copyCalls = 0;
    window.__copyWorks = true;
    document.execCommand = command => {
      if (command !== 'copy') return false;
      window.__copyCalls++;
      window.__copiedValue = document.activeElement.value;
      return window.__copyWorks;
    };
  });
  await page.route('http://deco.test/**', async route => {
    const req = route.request(), url = new URL(req.url());
    if (url.pathname === '/') return route.fulfill({ status: 200, contentType: 'text/html', body: html });
    if (offline) return route.abort();
    if (url.pathname === '/api/state') { stateGets++; return route.fulfill({ json: s }); }
    if (url.pathname === '/api/schedules' || url.pathname === '/api/sun') {
      assert.equal(req.method(), 'POST');
      assert.equal(req.headers()['x-deco-token'], s.token);
      const body = req.postDataJSON(), weekly = url.pathname === '/api/schedules';
      (weekly ? scheduleCalls : sunCalls).push(body);
      if (automationAbort === (weekly ? 'schedules' : 'sun')) { offline = true; return route.abort(); }
      const failure = weekly ? scheduleError : sunError;
      if (automationDelay) await new Promise(resolve => setTimeout(resolve, automationDelay));
      if (failure) return route.fulfill({ status: 400, json: { error: failure } });
      if (weekly) {
        assert.deepEqual(Object.keys(body).sort(), ['enabled', 'entries']);
        assert.equal(body.entries.length, 8);
        body.entries.forEach(entry => {
          assert.deepEqual(Object.keys(entry).sort(), ['days', 'enabled', 'favorite', 'minute', 'power']);
          assert(Number.isInteger(entry.days) && entry.days >= 0 && entry.days <= 127);
          assert(!entry.enabled || entry.days !== 0);
          assert(Number.isInteger(entry.minute) && entry.minute >= 0 && entry.minute < 1440);
          assert(entry.power ? entry.favorite >= 0 && entry.favorite <= 2 && s.favorites[entry.favorite].valid : entry.favorite === -1);
        });
        s.schedules = structuredClone(body);
      } else {
        assert.deepEqual(Object.keys(body).sort(), ['dawn', 'dusk', 'latitudeE6', 'longitudeE6']);
        assert(Number.isInteger(body.latitudeE6) && Math.abs(body.latitudeE6) <= 89000000);
        assert(Number.isInteger(body.longitudeE6) && Math.abs(body.longitudeE6) <= 180000000);
        for (const event of [body.dawn, body.dusk]) {
          assert.deepEqual(Object.keys(event).sort(), ['brightness', 'days', 'enabled', 'favorite', 'minutes', 'offset']);
          assert(Number.isInteger(event.days) && event.days >= 0 && event.days <= 127);
          assert(!event.enabled || event.days !== 0);
          assert(Number.isInteger(event.offset) && event.offset >= -120 && event.offset <= 120);
          assert(Number.isInteger(event.minutes) && event.minutes >= 1 && event.minutes <= 120);
          assert(Number.isInteger(event.brightness) && event.brightness >= 1 && event.brightness <= 100);
          assert(event.favorite === -1 || event.favorite >= 0 && event.favorite <= 2 && s.favorites[event.favorite].valid);
        }
        s.sun = { ...structuredClone(body), sunrise: s.sun.sunrise, sunset: s.sun.sunset };
      }
      s.revision++; s.scheduleSaved = true; s.scheduleSaveError = '';
      return route.fulfill({ json: s });
    }
    if (url.pathname === '/api/command') {
      assert.equal(req.headers()['x-deco-token'], s.token);
      const c = req.postDataJSON();
      if (commandDelay) await new Promise(resolve => setTimeout(resolve, commandDelay));
      commands.push(c); s.revision++;
      if (c.type === 'power') s.power = c.value;
      if (c.type === 'brightness') s.light.brightness = c.value;
      if (c.type === 'mode') { s.light.mode = c.value; if (c.value === 0) s.light.brightness = s.sleepBrightness; }
      if (c.type === 'color') Object.assign(s.light, { mode: 1, r: c.r, g: c.g, b: c.b });
      if (c.type === 'tint') {
        if (![1, 6, 7, 12].includes(s.light.mode)) return route.fulfill({ status: 400, json: { error: 'Chế độ này không cho đổi màu riêng.' } });
        Object.assign(s.light, { r: c.r, g: c.g, b: c.b });
      }
      if (c.type === 'speed') s.light.speed = c.value;
      if (c.type === 'sleep') { s.power = true; s.light.mode = 0; s.light.brightness = s.sleepBrightness; }
      if (c.type === 'favoriteSave') s.favorites[c.value] = { valid: true, light: { ...s.light } };
      if (c.type === 'favoriteApply') s.light = { ...s.favorites[c.value].light };
      if (c.type === 'timerStart') { s.timerActive = true; s.remainingMs = c.value * 60000; }
      if (c.type === 'timerCancel') s.timerActive = false;
      return route.fulfill({ json: s });
    }
    if (url.pathname === '/api/network') {
      assert.equal(req.headers()['x-deco-token'], s.token);
      const body = req.postDataJSON(); networkCalls.push(body);
      if (networkError) return route.fulfill({ status: 400, json: { error: networkError } });
      if (body.name !== undefined) s.name = body.name;
      if (body.hostname !== undefined) {
        s.requestedHostname = s.hostname = body.hostname;
        s.localUrl = 'http://' + s.hostname + '.local';
      }
      if (body.ssid !== undefined) { s.ssid = body.ssid; s.wifiOpen = body.open; }
      if (body.forget) {
        s.ssid = ''; s.wifiOpen = false; s.wifiConnected = false; s.apActive = true;
        s.networkStatus = 'unconfigured'; s.networkMessage = 'Chưa lưu WiFi. AP không mật khẩu đang mở.';
      }
      s.revision++;
      if (networkDelay) await new Promise(resolve => setTimeout(resolve, networkDelay));
      return route.fulfill({ json: {
        ok: true, reconnect: networkReconnect, localUrl: s.localUrl,
        message: networkReconnect ? 'Đã lưu. Kết nối WiFi nhà và mở ' + s.localUrl + '. AP tự tắt khi kết nối thành công.' : 'Đã lưu tên và địa chỉ truy cập.'
      } });
    }
    if (url.pathname === '/api/scan') {
      assert.equal(req.headers()['x-deco-token'], s.token);
      if (req.method() === 'POST') {
        scanStarts++; scanGets = 0;
        if (scanError) return route.fulfill({ status: 409, json: { error: scanError } });
        return route.fulfill({ json: { ok: true, status: 'scanning' } });
      }
      scanGets++;
      if (scanGetError) return route.fulfill({ json: { status: 'error', message: scanGetError, networks: [] } });
      return route.fulfill({ json: scanGets < 3 ? { status: 'scanning', networks: [] } : { status: 'ready', networks } });
    }
    if (url.pathname === '/api/ota') {
      assert.equal(req.headers()['x-deco-token'], s.token);
      assert.equal(req.headers()['x-firmware-size'], '64'); otaCalls++;
      if (otaDelay) await new Promise(resolve => setTimeout(resolve, otaDelay));
      if (otaError) return route.fulfill({ status: 400, json: { error: otaError } });
      return route.fulfill({ json: { ok: true, message: 'Cập nhật thành công; đèn đang khởi động lại' } });
    }
    return route.fulfill({ status: 404, json: { error: 'Unknown' } });
  });

  // Without a successful state response, diagnostics have no snapshot to copy.
  const idlePage = await browser.newPage();
  await idlePage.route('http://deco-idle.test/**', route => new URL(route.request().url()).pathname === '/' ? route.fulfill({ status: 200, contentType: 'text/html', body: html }) : route.abort());
  await idlePage.goto('http://deco-idle.test/');
  await idlePage.waitForFunction(() => document.getElementById('connectionText').textContent === 'Mất kết nối');
  assert(await idlePage.locator('#copyDiagnostics').isDisabled());
  assert.equal(await idlePage.locator('#diagnosticData').inputValue(), '');
  await idlePage.close();

  await page.goto('http://deco.test/');
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  assert.equal(await page.locator('#brightnessText').textContent(), '5');
  assert(await page.locator('#startTimer').isDisabled());
  assert((await page.locator('#footer').textContent()).includes('firmware 1.3.1'));
  assert((await page.locator('#planSaveDiagnostics').textContent()).includes('Chưa có thao tác lưu lịch'));
  assert(!(await page.locator('#planSaveDiagnostics').textContent()).includes('undefined'));
  assert(await page.locator('#diagnosticData').evaluate(e => e.readOnly));
  assert.deepEqual(await page.locator('#mode option').allTextContents(), modeLabels);
  assert.deepEqual(await page.locator('#mode option').evaluateAll(options => options.map(option => +option.value)), Array.from({ length: 14 }, (_, i) => i));
  assert.deepEqual(await page.locator('#effectGuide li strong').allTextContents(), modeLabels);
  const twinkleDescription = await page.locator('#effectGuide li p').nth(7).textContent();
  assert(twinkleDescription.includes('cụm sáng') && twinkleDescription.includes('tan dần trên nền tối'));
  assert(!twinkleDescription.includes('nền sáng dịu'));
  assert.equal(await page.locator('#helpCard').evaluate(e => e.open), false);
  await page.locator('#openHelp').focus(); await page.keyboard.press('Enter');
  await page.waitForFunction(() => document.getElementById('helpCard').open);
  assert.equal(await page.locator('#openHelp').getAttribute('aria-expanded'), 'true');
  assert.equal(await page.evaluate(() => document.activeElement.id), 'helpSummary');
  const guideText = await page.locator('#helpCard').textContent();
  for (const required of ['5 điểm phần trăm', '0,6 giây', '0,05 giây', '2 giây', '30 phút', '1–180 phút', '10 giây', '30 giây', '5 giây', 'http://den-deco.local', 'http://192.168.4.1', 'khôi phục trạng thái bật/tắt', 'Reset chỉ xóa mạng WiFi', 'Cập nhật thành công giữ cấu hình']) assert(guideText.includes(required), 'Help missing: ' + required);
  assert(!guideText.includes('đèn bắt đầu ở trạng thái tắt'), 'Help must match saved power restoration');
  assert(guideText.includes('Nếu vỏ làm hiệu ứng nhạt') && guideText.includes('tăng độ sáng vừa và giảm tốc độ') && guideText.includes('chỉnh theo phòng và vỏ'), 'Help adapts brightness/speed advice to the lamp enclosure');
  for (const forbidden of ['GPIO', 'API', 'NVS', 'Serial Monitor', 'unit test']) assert(!guideText.includes(forbidden), 'Implementation detail leaked into product help: ' + forbidden);
  await page.locator('#helpSummary').click();
  await page.waitForFunction(() => !document.getElementById('helpCard').open);
  await page.locator('#power').click();
  await page.waitForFunction(() => document.getElementById('power').textContent === 'Tắt đèn');
  await page.locator('#brightness').evaluate(e => {
    e.value = '38'; e.dispatchEvent(new Event('input')); e.dispatchEvent(new Event('change'));
  });
  await page.waitForFunction(() => document.getElementById('brightnessText').textContent === '38');
  await page.locator('[data-color="#529bff"]').click();
  await page.waitForFunction(() => document.getElementById('mode').value === '1');
  await page.locator('[data-save="0"]').click();
  await page.waitForFunction(() => !document.querySelector('[data-apply="0"]').disabled);
  await page.locator('[data-minutes="15"]').click();
  await page.waitForFunction(() => document.getElementById('remaining').textContent.includes('15:00'));
  await page.locator('#cancelTimer').click();
  await page.waitForFunction(() => document.getElementById('remaining').textContent === 'Chưa hẹn giờ');

  // Exercise all 14 IDs through the API, their descriptions, and mode-specific controls.
  for (let mode = 0; mode < 14; mode++) {
    await page.locator('#mode').selectOption(String(mode));
    await page.waitForFunction(expected => state.light.mode === expected, mode);
    const description = await page.locator('#effectGuide li p').nth(mode).textContent();
    assert(description.length > 20); assert.equal(await page.locator('#modeHelp').textContent(), description);
    assert.equal(await page.locator('#speedRow').isVisible(), ![0, 1, 3].includes(mode));
    const tint = [6, 7, 12].includes(mode);
    if (tint) {
      assert((await page.locator('#colorLabel').textContent()).includes(modeLabels[mode]));
      assert((await page.locator('#colorHint').textContent()).includes('vẫn giữ hiệu ứng'));
      const colorByMode = { 6: ['#986dff', 152, 109, 255], 7: ['#ff6991', 255, 105, 145], 12: ['#61ddac', 97, 221, 172] };
      const chosen = colorByMode[mode];
      await page.locator('[data-color="' + chosen[0] + '"]').click();
      await page.waitForFunction(expected => state.light.r === expected[1] && state.light.g === expected[2] && state.light.b === expected[3], chosen);
      assert.equal(commands.at(-1).type, 'tint'); assert.equal(s.light.mode, mode);
    } else if (mode !== 1) assert((await page.locator('#colorHint').textContent()).includes('chuyển sang Một màu'));
  }
  await page.locator('#speed').evaluate(e => { e.value = '72'; e.dispatchEvent(new Event('change')); });
  await page.waitForFunction(() => state.light.speed === 72); assert.equal(s.light.mode, 13);
  // Fixed-palette color selection intentionally switches to Solid.
  await page.locator('[data-color="#529bff"]').click();
  await page.waitForFunction(() => state.light.mode === 1);
  assert.equal(commands.at(-1).type, 'color');

  // A mode response can be delayed while the user immediately chooses a color.
  commandDelay = 400;
  let queueStart = commands.length;
  await page.locator('#mode').selectOption('6');
  await page.locator('[data-color="#61ddac"]').click();
  await page.waitForFunction(() => state.light.mode === 6 && state.light.r === 97 && state.light.g === 221);
  assert.deepEqual(commands.slice(queueStart).map(c => c.type), ['mode', 'tint']);
  assert.equal(s.light.mode, 6);
  queueStart = commands.length;
  await page.locator('#mode').selectOption('8');
  await page.locator('#color').evaluate(e => { e.value = '#ff6991'; e.dispatchEvent(new Event('change')); });
  await page.waitForFunction(() => state.light.mode === 1 && state.light.g === 105);
  assert.deepEqual(commands.slice(queueStart).map(c => c.type), ['mode', 'color']);
  await page.locator('#mode').selectOption('12');
  await page.waitForFunction(() => state.light.mode === 12);
  queueStart = commands.length;
  await page.locator('[data-color="#ffc078"]').click();
  await page.locator('#mode').selectOption('9');
  await page.waitForFunction(() => state.light.mode === 9);
  assert.deepEqual(commands.slice(queueStart).map(c => c.type), ['tint', 'mode']);
  commandDelay = 0;
  await page.locator('#mode').selectOption('1'); await page.waitForFunction(() => state.light.mode === 1);

  await page.locator('details').filter({ has: page.locator('#wifiForm') }).locator(':scope > summary').click();
  assert.equal(await page.locator('#localAddress').inputValue(), 'http://den-deco.local');
  assert((await page.locator('#apHelp').textContent()).includes('không có mật khẩu'));
  assert((await page.locator('#apHelp').textContent()).includes('5 giây'));
  assert(!html.includes('Mật khẩu AP riêng'));
  assert.equal(await page.evaluate(() => window.isSecureContext), false);
  await page.locator('#copyLocalUrl').click();
  assert.equal(await page.evaluate(() => window.__copyCalls), 1);
  assert.equal(await page.evaluate(() => window.__copiedValue), 'http://den-deco.local');
  assert((await page.locator('#notice').textContent()).includes('Đã sao chép'));
  await page.evaluate(() => { window.__copyWorks = false; });
  await page.locator('#copyLocalUrl').click();
  assert((await page.locator('#notice').textContent()).includes('Ctrl+C'));
  assert.equal(await page.locator('#localAddress').evaluate(e => e.selectionEnd - e.selectionStart), 'http://den-deco.local'.length);

  // Polling must preserve all drafts, including the password and open-network checkbox.
  await page.locator('#ssid').fill('HiddenSSID');
  await page.locator('#password').fill('not-a-real-secret');
  await page.locator('#lampName').fill('Đèn bàn');
  await page.locator('#hostname').fill('den-ban');
  await page.waitForTimeout(700);
  assert.equal(await page.locator('#ssid').inputValue(), 'HiddenSSID');
  assert.equal(await page.locator('#hostname').inputValue(), 'den-ban');
  assert.equal(await page.locator('#password').inputValue(), 'not-a-real-secret');
  await page.locator('#saveName').click();
  await page.waitForFunction(() => document.getElementById('name').textContent === 'Đèn bàn');
  assert.deepEqual(networkCalls[0], { name: 'Đèn bàn', hostname: 'den-ban' });
  assert.equal(await page.locator('#ssid').inputValue(), 'HiddenSSID');
  assert.equal(await page.locator('#password').inputValue(), 'not-a-real-secret');
  await page.waitForFunction(() => document.getElementById('localAddress').value === 'http://den-ban.local');

  // Reject malformed hostnames locally without a request.
  for (const invalid of ['den.ban.local', '-den', 'den-', 'den ban', 'đèn']) {
    await page.locator('#hostname').fill(invalid); await page.locator('#saveName').click();
    assert.equal(networkCalls.length, 1);
    assert((await page.locator('#notice').textContent()).includes('Tên truy cập cần'));
  }
  await page.locator('#hostname').evaluate(e => { e.value = 'a'.repeat(49); });
  await page.locator('#saveName').click(); assert.equal(networkCalls.length, 1);
  await page.locator('#hostname').fill('den-ban');

  // Scan asynchronously while light controls keep working; result text is escaped.
  await page.locator('#scanWiFi').click();
  assert(await page.locator('#scanWiFi').isDisabled());
  assert(!(await page.locator('#power').isDisabled()));
  assert.equal(await page.locator('#ssid').inputValue(), 'HiddenSSID');
  await page.locator('#power').click();
  await page.waitForFunction(() => document.getElementById('power').textContent === 'Bật đèn');
  await page.waitForFunction(() => document.getElementById('wifiResults').options.length === 4);
  assert.equal(scanStarts, 1); assert(scanGets >= 3);
  assert((await page.locator('#wifiResults option').nth(1).textContent()).includes('-35 dBm · WPA2'));
  assert((await page.locator('#wifiResults option').nth(2).textContent()).includes('Không mật khẩu'));
  assert((await page.locator('#wifiResults option').nth(3).textContent()).includes('<img src=x'));
  assert.equal(await page.locator('img').count(), 0);
  assert.equal(await page.locator('#password').inputValue(), 'not-a-real-secret');

  await page.locator('#wifiResults').selectOption('1');
  assert.equal(await page.locator('#ssid').inputValue(), 'Cafe mở');
  assert(await page.locator('#openNetwork').isChecked());
  assert(await page.locator('#password').isDisabled());
  assert.equal(await page.locator('#password').inputValue(), '');
  await page.waitForTimeout(600);
  assert(await page.locator('#openNetwork').isChecked());
  assert.equal(await page.locator('#ssid').inputValue(), 'Cafe mở');
  // Choosing secure WiFi resets the open-network choice and allows its password.
  await page.locator('#wifiResults').selectOption('0');
  assert.equal(await page.locator('#ssid').inputValue(), 'Home 2.4G');
  assert(!(await page.locator('#openNetwork').isChecked()));
  assert(!(await page.locator('#password').isDisabled()));
  await page.locator('#ssid').fill('HiddenSSID');
  assert.equal(await page.locator('#wifiResults').inputValue(), '');
  await page.locator('#password').fill('new-secret');
  networkError = 'Mật khẩu WiFi cần 8–63 ký tự.';
  await page.locator('#applyWiFi').click();
  await page.waitForFunction(() => document.getElementById('notice').textContent.includes('Mật khẩu WiFi cần'));
  assert.equal(networkCalls.length, 2);
  assert.equal(await page.locator('#password').inputValue(), 'new-secret');
  assert.equal(await page.locator('#ssid').inputValue(), 'HiddenSSID');
  networkError = ''; networkReconnect = true;
  await page.locator('#applyWiFi').click();
  await page.waitForFunction(() => document.getElementById('notice').textContent.includes('AP tự tắt'));
  assert.deepEqual(networkCalls[2], { name: 'Đèn bàn', hostname: 'den-ban', ssid: 'HiddenSSID', password: 'new-secret', open: false });
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  // An explicit second submit retries even when the saved SSID is unchanged.
  await page.locator('#applyWiFi').click();
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  assert.equal(networkCalls.length, 4); assert.equal(networkCalls[3].ssid, 'HiddenSSID');
  assert.equal(networkCalls[3].password, '');
  networkReconnect = false;

  // Remember a saved open network after fresh state arrives; submit it as open.
  s.ssid = 'Cafe mở'; s.wifiOpen = true; s.revision++;
  await page.waitForFunction(() => document.getElementById('ssid').value === 'Cafe mở' && document.getElementById('openNetwork').checked);
  await page.locator('#applyWiFi').click();
  await page.waitForTimeout(350);
  assert.equal(networkCalls[4].open, true); assert.equal(networkCalls[4].password, '');

  s.networkStatus = 'failed'; s.networkMessage = 'Không kết nối được: sai mật khẩu hoặc mạng từ chối kết nối.';
  s.disconnectReason = 202; s.wifiConnected = false; s.apActive = true; s.mdnsReady = false; s.revision++;
  await page.waitForFunction(() => document.getElementById('networkStatus').textContent.includes('sai mật khẩu'));
  assert((await page.locator('#networkInfo').textContent()).includes('không mật khẩu · http://192.168.4.1'));
  assert((await page.locator('#localHelp').textContent()).includes('mDNS'));
  s.hostname = 'den-ban-2'; s.localUrl = 'http://den-ban-2.local';
  s.networkStatus = 'connected'; s.networkMessage = 'Đã kết nối WiFi nhà.';
  s.wifiConnected = true; s.apActive = false; s.mdnsReady = true; s.revision++;
  await page.waitForFunction(() => document.getElementById('localAddress').value === 'http://den-ban-2.local');
  assert((await page.locator('#localHelp').textContent()).includes('tránh trùng'));
  assert.equal(await page.locator('#hostname').inputValue(), 'den-ban');

  scanError = 'Đang kết nối WiFi; hãy chờ rồi quét lại.';
  await page.locator('#scanWiFi').click();
  await page.waitForFunction(() => document.getElementById('scanStatus').textContent.includes('hãy chờ'));
  assert(!(await page.locator('#scanWiFi').isDisabled()));
  assert.equal(await page.locator('#connectionText').textContent(), 'Đã kết nối');
  scanError = ''; scanGetError = 'Không quét được WiFi. Thử lại sau.';
  await page.locator('#scanWiFi').click();
  await page.waitForFunction(() => document.getElementById('scanStatus').textContent.includes('Thử lại sau'));
  assert(!(await page.locator('#scanWiFi').isDisabled()));
  scanGetError = '';

  // Radio diagnostics distinguish a stale cached status from a real link,
  // without requiring the user to copy the session token from the full API.
  const radioBefore = structuredClone({ wifiConnected: s.wifiConnected, ip: s.ip, apActive: s.apActive, networkStatus: s.networkStatus, networkMessage: s.networkMessage, disconnectReason: s.disconnectReason, wifiDiagnostics: s.wifiDiagnostics });
  s.wifiConnected = false; s.ip = ''; s.apActive = true; s.networkStatus = 'failed';
  s.networkMessage = 'Xác thực với router hết thời gian (mã 2), chưa nhận IP. AP dự phòng đang mở; đèn sẽ thử lại.';
  s.disconnectReason = 2; s.wifiDiagnostics = { ...s.wifiDiagnostics, status: 3, interfaceReady: false, associated: false, beginAttempts: 4 };
  s.revision++;
  await page.waitForFunction(() => document.getElementById('networkStatus').textContent.includes('mã 2'));
  await page.locator('#wifiDiagnosticCard > summary').click();
  assert((await page.locator('#wifiDiagnostics').textContent()).includes('liên kết router: Chưa'));
  assert((await page.locator('#wifiDiagnostics').textContent()).includes('trạng thái: 3'));
  assert((await page.locator('#wifiDiagnostics').textContent()).includes('mã ngắt: 2'));
  s.wifiDiagnostics.cancelTimedOut = true; s.revision++;
  await page.waitForFunction(() => document.getElementById('wifiDiagnostics').textContent.includes('Chưa hủy được kết nối trước'));
  Object.assign(s, radioBefore); s.revision++;
  await page.waitForFunction(() => document.getElementById('wifiDiagnostics').textContent.includes('liên kết router: Có'));
  // A recovered link clears the current reason, while RAM keeps the preceding
  // disconnect and completed save measurements. Only allowlisted scalars are copied.
  s.disconnectReason = 0;
  s.wifiDiagnostics = { ...s.wifiDiagnostics, lastDisconnectReason: 2, lastDisconnectAtMs: 114000, disconnectCount: 1, powerSave: 0, token: 'hidden-wifi-token' };
  s.resources.password = 'hidden-resource-password';
  s.planSaveDiagnostics = { ...s.planSaveDiagnostics, kind: 'sun', stage: 'complete', count: 1, startedAtMs: 113970, totalMs: 28, lightMs: 0, planMs: 17, applyMs: 1, responseMs: 10, successful: true, lightWritten: false, planWritten: true, onlineBefore: true, onlineAfter: true, associatedBefore: true, associatedAfter: true, interfaceBefore: true, interfaceAfter: true, ipBefore: s.ip, ipAfter: s.ip, heapBefore: 220000, heapAfter: 217000, stackFree: 3700, token: 'hidden-save-token' };
  s.revision++;
  await page.waitForFunction(() => document.getElementById('wifiDiagnostics').textContent.includes('lần ngắt gần nhất: mã 2'));
  assert((await page.locator('#wifiDiagnostics').textContent()).includes('mã ngắt: 0'));
  assert((await page.locator('#planSaveDiagnostics').textContent()).includes('lịch mặt trời #1'));
  assert((await page.locator('#planSaveDiagnostics').textContent()).includes('lịch 17 ms'));
  assert((await page.locator('#resourceDiagnostics').textContent()).includes('stack còn thấp nhất: 3800 byte'));
  await page.evaluate(() => { window.__copyWorks = true; });
  const diagnosticCopyBefore = await page.evaluate(() => window.__copyCalls);
  await page.locator('#copyDiagnostics').click();
  assert.equal(await page.evaluate(() => window.__copyCalls), diagnosticCopyBefore + 1);
  const copiedDiagnostic = await page.evaluate(() => window.__copiedValue);
  const diagnostic = JSON.parse(copiedDiagnostic);
  assert.deepEqual(Object.keys(diagnostic).sort(), ['version', 'uptimeMs', 'hostname', 'wifiConnected', 'ip', 'networkStatus', 'apActive', 'apSSID', 'disconnectReason', 'wifiDiagnostics', 'resources', 'planSaveDiagnostics'].sort());
  assert.equal(diagnostic.version, '1.3.1');
  assert.equal(diagnostic.disconnectReason, 0);
  assert.equal(diagnostic.wifiDiagnostics.lastDisconnectReason, 2);
  assert.equal(diagnostic.wifiDiagnostics.lastDisconnectAtMs, 114000);
  assert.equal(diagnostic.wifiDiagnostics.powerSave, 0);
  assert.equal(diagnostic.planSaveDiagnostics.planMs, 17);
  assert.equal(diagnostic.planSaveDiagnostics.responseMs, 10);
  assert.equal(diagnostic.planSaveDiagnostics.ipAfter, s.ip);
  assert.equal(diagnostic.resources.loopStackFree, 3800);
  for (const secret of [s.token, 'hidden-wifi-token', 'hidden-save-token', 'hidden-resource-password', 'not-a-real-secret']) assert(!copiedDiagnostic.includes(secret));
  assert(!/"(?:token|password|ssid|light|favorites|sun)"\s*:/.test(copiedDiagnostic));
  // Failed automatic clipboard copy leaves a stable selection for Ctrl+C,
  // including while background polling continues.
  await page.evaluate(() => { window.__copyWorks = false; });
  await page.locator('#copyDiagnostics').click();
  assert((await page.locator('#notice').textContent()).includes('Ctrl+C'));
  await page.waitForTimeout(550);
  assert.equal(await page.locator('#diagnosticData').evaluate(e => e.selectionEnd - e.selectionStart), copiedDiagnostic.length);
  offline = true;
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Mất kết nối');
  assert(!(await page.locator('#copyDiagnostics').isDisabled()));
  await page.evaluate(() => { window.__copyWorks = true; });
  await page.locator('#copyDiagnostics').click();
  assert.equal(await page.evaluate(() => window.__copiedValue), copiedDiagnostic);
  offline = false;
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  // Earlier firmware lacks all three diagnostic objects; keep a small snapshot
  // and clear old summaries instead of showing previous firmware measurements.
  const diagnosticsBeforeLegacy = structuredClone({ wifiDiagnostics: s.wifiDiagnostics, resources: s.resources, planSaveDiagnostics: s.planSaveDiagnostics });
  delete s.wifiDiagnostics; delete s.resources; delete s.planSaveDiagnostics; s.revision++;
  await page.waitForFunction(() => document.getElementById('planSaveDiagnostics').textContent.includes('Firmware chưa có'));
  assert((await page.locator('#wifiDiagnostics').textContent()).includes('Firmware chưa có'));
  assert.equal(await page.locator('#resourceDiagnostics').textContent(), '');
  await page.locator('#copyDiagnostics').click();
  const oldDiagnostic = JSON.parse(await page.evaluate(() => window.__copiedValue));
  assert(!('wifiDiagnostics' in oldDiagnostic));
  assert(!('resources' in oldDiagnostic));
  assert(!('planSaveDiagnostics' in oldDiagnostic));
  assert(!('token' in oldDiagnostic));
  Object.assign(s, diagnosticsBeforeLegacy); s.revision++;
  await page.waitForFunction(() => document.getElementById('planSaveDiagnostics').textContent.includes('lịch mặt trời #1'));
  await page.locator('#wifiDiagnosticCard > summary').click();

  // Automation lives in independent drafts: the 250 ms full-state poll may update
  // the clock/status, but must never replace a schedule or solar edit.
  await page.locator('#automationCard > summary').click();
  assert(!(await page.locator('#schedulesEnabled').isChecked()));
  assert(!(await page.locator('#dawnEnabled').isChecked()));
  assert(!(await page.locator('#duskEnabled').isChecked()));
  assert((await page.locator('#clockStatus').textContent()).match(/đồng bộ|chờ|giờ/i));
  const clockRequestsBefore = stateGets;
  s.clock = { ready: true, online: true, syncing: false, utc: 1791783000, localTime: '09:50:00', localDate: '12/10/2026', lastSyncUtc: 1791783000, zone: 'UTC+7' };
  s.sun.sunrise = '05:43'; s.sun.sunset = '17:37'; s.revision++;
  await page.waitForFunction(() => document.getElementById('clockTime').textContent.includes('09:50:00'));
  assert((await page.locator('#clockDate').textContent()).includes('12/10/2026'));
  assert((await page.locator('#sunriseTime').textContent()).includes('05:43'));
  assert((await page.locator('#sunsetTime').textContent()).includes('17:37'));
  assert(stateGets > clockRequestsBefore);

  const visibleScheduleSlots = () => page.locator('#scheduleEntries [data-slot]').evaluateAll(rows => rows.filter(row => row.getClientRects().length).map(row => +row.dataset.slot));
  for (let slot = 0; slot < 8; slot++) await page.locator('#addSchedule').click();
  assert.deepEqual(await visibleScheduleSlots(), [0, 1, 2, 3, 4, 5, 6, 7]);
  assert(await page.locator('#addSchedule').isDisabled());
  await page.locator('#scheduleTime1').fill('23:00');
  await page.locator('[data-remove-slot="0"]').click();
  assert.deepEqual(await visibleScheduleSlots(), [1, 2, 3, 4, 5, 6, 7]);
  assert.equal(await page.locator('#scheduleTime1').inputValue(), '23:00');
  assert(!(await page.locator('#addSchedule').isDisabled()));
  await page.locator('#addSchedule').click();
  assert.deepEqual(await visibleScheduleSlots(), [0, 1, 2, 3, 4, 5, 6, 7]);
  for (let slot = 2; slot < 8; slot++) await page.locator('[data-remove-slot="' + slot + '"]').click();
  await page.locator('#schedulesEnabled').check();
  await page.locator('#scheduleEnabled0').check();
  await page.locator('#scheduleTime0').fill('07:15');
  await page.locator('#schedulePower0').selectOption('on');
  const favoriteAvailability = await page.locator('#scheduleFavorite0 option').evaluateAll(options => options.map(o => ({ value: o.value, disabled: o.disabled })));
  assert(favoriteAvailability.some(o => o.value === '0' && !o.disabled));
  assert(!favoriteAvailability.some(o => ['1', '2'].includes(o.value) && !o.disabled), 'Weekly ON must offer only existing valid favorites');
  await page.locator('#scheduleFavorite0').selectOption('0');
  for (let day = 0; day < 7; day++) await page.locator('#scheduleDay0_' + day).setChecked([0, 4].includes(day));
  await page.locator('#scheduleEnabled1').check();
  await page.locator('#schedulePower1').selectOption('off');
  await page.locator('#scheduleFavorite1').evaluate(e => { e.value = '0'; e.dispatchEvent(new Event('change', { bubbles: true })); });
  for (let day = 0; day < 7; day++) await page.locator('#scheduleDay1_' + day).setChecked(day === 6);
  const weeklyDraft = await page.locator('#schedulesForm').evaluate(form => Array.from(form.querySelectorAll('input,select')).map(e => [e.id, e.type === 'checkbox' ? e.checked : e.value]));
  const scheduleDraftText = await page.locator('#scheduleDraftStatus').textContent();
  assert(scheduleDraftText.length > 0);
  const pollCountBeforeDraft = stateGets;
  await page.waitForTimeout(800);
  assert(stateGets >= pollCountBeforeDraft + 2, 'Exercise repeated 250 ms state polling');
  assert.deepEqual(await page.locator('#schedulesForm').evaluate(form => Array.from(form.querySelectorAll('input,select')).map(e => [e.id, e.type === 'checkbox' ? e.checked : e.value])), weeklyDraft);
  assert.equal(scheduleCalls.length, 0, 'Editing schedules must not save automatically');

  // Required days, HH:MM and a valid favorite are checked before any write.
  for (let day = 0; day < 7; day++) await page.locator('#scheduleDay0_' + day).uncheck();
  await page.locator('#saveSchedules').click();
  assert.equal(scheduleCalls.length, 0);
  await page.locator('#scheduleDay0_0').check(); await page.locator('#scheduleDay0_4').check();
  await page.locator('#scheduleTime0').fill('');
  await page.locator('#saveSchedules').click(); assert.equal(scheduleCalls.length, 0);
  await page.locator('#scheduleTime0').fill('07:15');
  await page.locator('#scheduleFavorite0').evaluate(e => { e.value = '1'; e.dispatchEvent(new Event('change', { bubbles: true })); });
  await page.locator('#saveSchedules').click(); assert.equal(scheduleCalls.length, 0);
  await page.locator('#scheduleFavorite0').selectOption('0');
  scheduleError = 'Không lưu được lịch. Hãy thử lại.';
  await page.locator('#saveSchedules').click();
  await page.waitForFunction(() => document.getElementById('notice').textContent.includes('Không lưu được lịch'));
  assert.equal(scheduleCalls.length, 1);
  assert.equal(await page.locator('#scheduleTime0').inputValue(), '07:15');
  assert.equal(await page.locator('#scheduleDraftStatus').textContent(), scheduleDraftText);
  assert.equal(s.schedules.enabled, false, 'A rejected save must not mutate persisted state');
  scheduleError = '';
  await page.locator('#saveSchedules').click();
  await page.waitForFunction(() => state.schedules.enabled && state.schedules.entries[0].minute === 435);
  assert.equal(scheduleCalls.length, 2);
  assert.deepEqual(scheduleCalls[1].entries[0], { enabled: true, days: 17, minute: 435, power: true, favorite: 0 });
  assert.deepEqual(scheduleCalls[1].entries[1], { enabled: true, days: 64, minute: 1380, power: false, favorite: -1 });
  scheduleCalls[1].entries.slice(2).forEach(entry => assert.deepEqual(entry, { enabled: false, days: 0, minute: 0, power: false, favorite: -1 }));

  // A delayed save acknowledges only the submitted snapshot, not newer edits.
  automationDelay = 450;
  await page.locator('#scheduleTime0').fill('07:20');
  await page.locator('#saveSchedules').click();
  assert(await page.locator('#scheduleTime0').isDisabled());
  await page.locator('#scheduleTime0').evaluate(e => { e.value = '07:25'; e.dispatchEvent(new Event('input', { bubbles: true })); });
  await page.waitForTimeout(750);
  assert.equal(s.schedules.entries[0].minute, 440);
  assert.equal(await page.locator('#scheduleTime0').inputValue(), '07:25');
  automationDelay = 0;
  // Reload/discard requires confirmation and does not write to the device.
  let reloadPrompt = '';
  page.once('dialog', async dialog => { reloadPrompt = dialog.message(); await dialog.dismiss(); });
  await page.locator('#reloadSchedules').click();
  assert(reloadPrompt.length > 0);
  assert.equal(await page.locator('#scheduleTime0').inputValue(), '07:25');
  const writesBeforeReload = scheduleCalls.length;
  page.once('dialog', dialog => dialog.accept());
  await page.locator('#reloadSchedules').click();
  assert.equal(await page.locator('#scheduleTime0').inputValue(), '07:20');
  assert.equal(scheduleCalls.length, writesBeforeReload);

  // Solar configuration uses degrees in the form and fixed-point E6 in storage.
  assert.equal(await page.locator('#duskMinutes').getAttribute('type'), 'hidden');
  assert(!(await page.locator('#duskMinutes').isVisible()));
  assert(await page.locator('#dawnMinutes').isVisible());
  await page.locator('#latitude').fill('10.123456');
  await page.locator('#longitude').fill('106.654321');
  await page.locator('#dawnEnabled').check();
  await page.locator('#duskEnabled').check();
  await page.locator('#dawnOffset').fill('-15');
  await page.locator('#duskOffset').fill('20');
  await page.locator('#dawnMinutes').fill('45');
  await page.locator('#dawnBrightness').fill('42');
  await page.locator('#duskBrightness').fill('28');
  for (let day = 0; day < 7; day++) {
    await page.locator('#dawnDay' + day).setChecked([0, 1, 2, 3, 4].includes(day));
    await page.locator('#duskDay' + day).setChecked([5, 6].includes(day));
  }
  await page.locator('#dawnFavorite').selectOption('-1');
  await page.locator('#duskFavorite').selectOption('-1');
  assert(await page.locator('#dawnBrightnessRow').isVisible());
  assert(await page.locator('#duskBrightnessRow').isVisible());
  for (const id of ['dawnFavorite', 'duskFavorite']) {
    const available = await page.locator('#' + id + ' option').evaluateAll(options => options.filter(o => !o.disabled).map(o => o.value));
    assert(available.includes('-1') && available.includes('0'));
    assert(!available.includes('1') && !available.includes('2'), 'Solar actions must gate empty favorites');
  }
  const solarDraft = await page.locator('#sunForm').evaluate(form => Array.from(form.querySelectorAll('input,select')).map(e => [e.id, e.type === 'checkbox' ? e.checked : e.value]));
  await page.waitForTimeout(750);
  assert.deepEqual(await page.locator('#sunForm').evaluate(form => Array.from(form.querySelectorAll('input,select')).map(e => [e.id, e.type === 'checkbox' ? e.checked : e.value])), solarDraft);
  assert.equal(sunCalls.length, 0);
  sunError = 'Không lưu được vị trí và bình minh. Hãy thử lại.';
  await page.locator('#saveSun').click();
  await page.waitForFunction(() => document.getElementById('notice').textContent.includes('Không lưu được vị trí'));
  assert.equal(sunCalls.length, 1);
  assert.equal(await page.locator('#latitude').inputValue(), '10.123456');
  assert.equal(await page.locator('#dawnMinutes').inputValue(), '45');
  sunError = '';
  await page.locator('#saveSun').click();
  await page.waitForFunction(() => state.sun.latitudeE6 === 10123456);
  assert.equal(sunCalls.length, 2);
  assert.equal(sunCalls[1].longitudeE6, 106654321);
  assert.deepEqual(sunCalls[1].dawn, { enabled: true, days: 31, offset: -15, minutes: 45, favorite: -1, brightness: 42 });
  assert.deepEqual(sunCalls[1].dusk, { enabled: true, days: 96, offset: 20, minutes: 30, favorite: -1, brightness: 28 });
  await page.locator('#dawnFavorite').selectOption('0');
  await page.locator('#duskFavorite').selectOption('0');
  assert(!(await page.locator('#dawnBrightnessRow').isVisible()));
  assert(!(await page.locator('#duskBrightnessRow').isVisible()));
  await page.locator('#saveSun').click();
  await page.waitForFunction(() => state.sun.dawn.favorite === 0 && state.sun.dusk.favorite === 0);
  assert.equal(sunCalls[2].dawn.brightness, 42, 'Choosing a favorite preserves the built-in warm brightness draft');
  await page.locator('#dawnFavorite').selectOption('-1');
  assert(await page.locator('#dawnBrightnessRow').isVisible());
  assert.equal(await page.locator('#dawnBrightness').inputValue(), '42');

  // Reject out-of-range/partial coordinates and malformed solar actions locally.
  const solarCallsBeforeValidation = sunCalls.length;
  for (const [id, value] of [['latitude', '89.000001'], ['latitude', '-89.000001'], ['latitude', ''], ['longitude', '180.000001'], ['longitude', '-180.000001']]) {
    await page.locator('#latitude').fill('10.123456'); await page.locator('#longitude').fill('106.654321');
    await page.locator('#' + id).fill(value); await page.locator('#saveSun').click();
    assert.equal(sunCalls.length, solarCallsBeforeValidation, 'Coordinate rejected: ' + id + '=' + value);
  }
  await page.locator('#latitude').fill('10.123456'); await page.locator('#longitude').fill('106.654321');
  for (const [id, value, valid] of [['dawnOffset', '121', '-15'], ['dawnMinutes', '0', '45'], ['dawnMinutes', '121', '45'], ['dawnBrightness', '0', '42']]) {
    await page.locator('#' + id).fill(value); await page.locator('#saveSun').click();
    assert.equal(sunCalls.length, solarCallsBeforeValidation, 'Solar value rejected: ' + id + '=' + value);
    await page.locator('#' + id).fill(valid);
  }
  for (let day = 0; day < 7; day++) await page.locator('#dawnDay' + day).uncheck();
  await page.locator('#saveSun').click(); assert.equal(sunCalls.length, solarCallsBeforeValidation);
  for (let day = 0; day < 5; day++) await page.locator('#dawnDay' + day).check();
  await page.locator('#dawnFavorite').evaluate(e => { e.value = '2'; e.dispatchEvent(new Event('change', { bubbles: true })); });
  await page.locator('#saveSun').click(); assert.equal(sunCalls.length, solarCallsBeforeValidation);
  await page.locator('#dawnFavorite').selectOption('-1');
  // The inclusive coordinate limits are accepted and still produce integer E6.
  await page.locator('#latitude').fill('-89'); await page.locator('#longitude').fill('180');
  await page.locator('#saveSun').click();
  await page.waitForFunction(() => state.sun.latitudeE6 === -89000000 && state.sun.longitudeE6 === 180000000);
  await page.locator('#latitude').fill('89'); await page.locator('#longitude').fill('-180');
  await page.locator('#saveSun').click();
  await page.waitForFunction(() => state.sun.latitudeE6 === 89000000 && state.sun.longitudeE6 === -180000000);
  await page.locator('#useHCM').click();
  const hcmLatitude = +await page.locator('#latitude').inputValue(), hcmLongitude = +await page.locator('#longitude').inputValue();
  assert(hcmLatitude > 10 && hcmLatitude < 11 && hcmLongitude > 106 && hcmLongitude < 107);
  await page.locator('#saveSun').click();
  await page.waitForFunction(({ lat, lon }) => state.sun.latitudeE6 === lat && state.sun.longitudeE6 === lon, { lat: Math.round(hcmLatitude * 1e6), lon: Math.round(hcmLongitude * 1e6) });
  await page.locator('#longitude').fill('105.5');
  page.once('dialog', dialog => dialog.dismiss()); await page.locator('#reloadSun').click();
  assert.equal(await page.locator('#longitude').inputValue(), '105.5');
  const solarWritesBeforeReload = sunCalls.length;
  page.once('dialog', dialog => dialog.accept()); await page.locator('#reloadSun').click();
  assert.equal(+await page.locator('#longitude').inputValue(), hcmLongitude);
  assert.equal(sunCalls.length, solarWritesBeforeReload);

  // WiFi loss after a valid NTP clock must not be confused with clock failure.
  s.clock.online = false; s.wifiConnected = false; s.clock.localTime = '09:51:00'; s.revision++;
  await page.waitForFunction(() => document.getElementById('clockTime').textContent.includes('09:51:00'));
  assert(!(await page.locator('#saveSchedules').isDisabled()));
  assert(!(await page.locator('#saveSun').isDisabled()));
  assert(!(await page.locator('#clockStatus').textContent()).match(/chưa.*(?:giờ|đồng bộ)/i));
  s.automation = { active: true, kind: 'dawn', brightness: 12.5, target: 42, remainingMs: 600000, error: '' }; s.revision++;
  await page.waitForFunction(() => document.getElementById('automationStatus').textContent.includes('12.5%'));
  assert((await page.locator('#automationStatus').textContent()).includes('42'));
  s.automation = { active: false, kind: 'dawn', brightness: 0, target: 42, remainingMs: 0, error: 'Mục yêu thích đã bị xóa.' }; s.revision++;
  await page.waitForFunction(() => document.getElementById('automationStatus').textContent.includes('Mục yêu thích đã bị xóa'));
  s.clock.online = true; s.wifiConnected = true; s.automation.error = ''; s.revision++;

  // Failed writes are uncertain: retain drafts and require an explicit retry,
  // rather than replaying a schedule/solar POST when polling reconnects.
  for (const section of [
    { endpoint: 'schedules', field: 'scheduleTime0', value: '07:30', button: 'saveSchedules', calls: scheduleCalls, reload: 'reloadSchedules' },
    { endpoint: 'sun', field: 'longitude', value: '106.2', button: 'saveSun', calls: sunCalls, reload: 'reloadSun' }
  ]) {
    await page.locator('#' + section.field).fill(section.value);
    const count = section.calls.length;
    automationAbort = section.endpoint;
    await page.locator('#' + section.button).click();
    await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Mất kết nối');
    assert.equal(section.calls.length, count + 1);
    assert.equal(await page.locator('#' + section.field).inputValue(), section.value);
    assert(await page.locator('#' + section.button).isDisabled());
    automationAbort = ''; offline = false;
    await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
    await page.waitForTimeout(750);
    assert.equal(section.calls.length, count + 1, 'Reconnect must not replay ' + section.endpoint);
    assert.equal(await page.locator('#' + section.field).inputValue(), section.value);
    page.once('dialog', dialog => dialog.accept());
    await page.locator('#' + section.reload).click();
  }

  // Disconnect cancels an active scan and never replays scans, commands or network writes.
  await page.locator('#scanWiFi').click();
  offline = true;
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Mất kết nối');
  const commandCount = commands.length, networkCount = networkCalls.length, scansBefore = scanStarts;
  const scheduleCount = scheduleCalls.length, solarCount = sunCalls.length;
  assert(await page.locator('#power').isDisabled());
  assert(await page.locator('#saveSchedules').isDisabled());
  assert(await page.locator('#saveSun').isDisabled());
  // Instructions stay available when the lamp cannot be reached.
  await page.locator('#openHelp').click();
  await page.waitForFunction(() => document.getElementById('helpCard').open);
  await page.locator('#helpSummary').click();
  offline = false;
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  await page.waitForTimeout(1000);
  assert.equal(commands.length, commandCount); assert.equal(networkCalls.length, networkCount);
  assert.equal(scheduleCalls.length, scheduleCount); assert.equal(sunCalls.length, solarCount);
  assert.equal(scanStarts, scansBefore); assert(!(await page.locator('#scanWiFi').isDisabled()));

  // A late save response must not erase text the user changed during that save.
  networkDelay = 450;
  await page.locator('#lampName').fill('Đèn đã gửi');
  await page.locator('#hostname').fill('den-da-gui');
  await page.locator('#saveName').click();
  await page.locator('#lampName').fill('Tên đang nhập');
  await page.locator('#hostname').fill('den-dang-nhap');
  await page.waitForTimeout(750);
  assert.equal(s.name, 'Đèn đã gửi');
  assert.equal(await page.locator('#lampName').inputValue(), 'Tên đang nhập');
  assert.equal(await page.locator('#hostname').inputValue(), 'den-dang-nhap');
  networkDelay = 0;

  const shotDir = path.resolve(root, '../work/web-previews');
  fs.mkdirSync(shotDir, { recursive: true });
  await page.screenshot({ path: path.join(shotDir, 'web-mobile.png'), fullPage: true });
  assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth));
  await page.setViewportSize({ width: 1280, height: 900 });
  await page.screenshot({ path: path.join(shotDir, 'web-desktop.png'), fullPage: true });
  await page.setViewportSize({ width: 390, height: 844 });
  page.on('dialog', d => d.accept());
  await page.locator('details').filter({ has: page.locator('#otaForm') }).locator(':scope > summary').click();
  await page.locator('#firmware').setInputFiles({ name: 'firmware.bin', mimeType: 'application/octet-stream', buffer: Buffer.alloc(64) });
  // An OTA rejection must not leave a queued, unexecuted mode displayed as active.
  commandDelay = 400; otaDelay = 850; otaError = 'File cập nhật không hợp lệ.';
  queueStart = commands.length;
  await page.locator('#mode').selectOption('6');
  await page.locator('#mode').selectOption('7');
  await page.locator('#uploadFirmware').click();
  assert(await page.locator('#copyDiagnostics').isDisabled());
  await page.waitForFunction(() => document.getElementById('otaStatus').textContent.includes('không hợp lệ'));
  await page.waitForFunction(() => state.light.mode === 6 && document.getElementById('mode').value === '6');
  assert.deepEqual(commands.slice(queueStart).map(c => [c.type, c.value]), [['mode', 6]]);
  commandDelay = 0; otaDelay = 0; otaError = '';
  await page.locator('#uploadFirmware').click();
  await page.waitForFunction(() => document.getElementById('otaStatus').textContent.includes('thành công'));
  assert.equal(otaCalls, 2);
  assert(await page.locator('#scanWiFi').isDisabled());
  assert(await page.locator('#copyDiagnostics').isDisabled());
  assert.equal(errors.length, 0, errors.join('\n'));
  await browser.close();
  console.log('PASS: weekly 8-slot schedules, HH:MM/day/favorite validation, solar E6/range/HCM/warm/favorite controls, draft preservation/error/discard/late-save races, clock sync and WiFi-loss status, offline no-replay; private RAM save/resource/radio diagnostics, HTTP clipboard fallback and offline/legacy/OTA copy behavior; retained 14-mode/tint/help/light/favorites/timer/OTA/WiFi/hostname/clipboard/layout checks, no JS errors');
})().catch(e => { console.error(e); process.exit(1); });
