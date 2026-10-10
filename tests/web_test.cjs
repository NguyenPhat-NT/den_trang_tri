const { chromium } = require('playwright');
const fs = require('fs');
const assert = require('assert');
const path = require('path');

(async () => {
  const root = path.resolve(__dirname, '..');
  const source = fs.readFileSync(path.join(root, 'DenDeco/WebUI.h'), 'utf8');
  const html = source.split('R"DECOHTML(')[1].split(')DECOHTML"')[0];
  const s = {
    version: '1.1.0', name: 'Đèn Deco', token: 'test-session', revision: 0,
    power: false, light: { mode: 0, r: 255, g: 96, b: 24, brightness: 5, speed: 50 },
    sleepBrightness: 5, cap: 30, ledCount: 20,
    favorites: Array.from({ length: 3 }, () => ({ valid: false, light: { mode: 0, brightness: 5 } })),
    timerActive: false, remainingMs: 0, hasSaved: true, savePending: false, saveError: '',
    ssid: 'Home', wifiOpen: false, wifiConnected: true, ip: '192.168.1.10',
    apActive: false, apSSID: 'DenDeco-TEST', hostname: 'den-deco', requestedHostname: 'den-deco',
    localUrl: 'http://den-deco.local', mdnsReady: true,
    networkStatus: 'connected', networkMessage: 'Đã kết nối WiFi nhà.', disconnectReason: 0,
    otaMaxBytes: 1310720
  };
  const commands = [], networkCalls = [], errors = [];
  let offline = false, otaCalls = 0, scanStarts = 0, scanGets = 0;
  let scanError = '', scanGetError = '', networkError = '', networkReconnect = false, networkDelay = 0;
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
      window.__copiedValue = document.getElementById('localAddress').value;
      return window.__copyWorks;
    };
  });
  await page.route('http://deco.test/**', async route => {
    const req = route.request(), url = new URL(req.url());
    if (url.pathname === '/') return route.fulfill({ status: 200, contentType: 'text/html', body: html });
    if (offline) return route.abort();
    if (url.pathname === '/api/state') return route.fulfill({ json: s });
    if (url.pathname === '/api/command') {
      assert.equal(req.headers()['x-deco-token'], s.token);
      const c = req.postDataJSON();
      commands.push(c); s.revision++;
      if (c.type === 'power') s.power = c.value;
      if (c.type === 'brightness') s.light.brightness = c.value;
      if (c.type === 'color') Object.assign(s.light, { mode: 1, r: c.r, g: c.g, b: c.b });
      if (c.type === 'favoriteSave') s.favorites[c.value] = { valid: true, light: { ...s.light } };
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
      return route.fulfill({ json: { ok: true, message: 'Cập nhật thành công; đèn đang khởi động lại' } });
    }
    return route.fulfill({ status: 404, json: { error: 'Unknown' } });
  });

  await page.goto('http://deco.test/');
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  assert.equal(await page.locator('#brightnessText').textContent(), '5');
  assert(await page.locator('#startTimer').isDisabled());
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

  await page.locator('summary').nth(0).click();
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

  // Disconnect cancels an active scan and never replays scans, commands or network writes.
  await page.locator('#scanWiFi').click();
  offline = true;
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Mất kết nối');
  const commandCount = commands.length, networkCount = networkCalls.length, scansBefore = scanStarts;
  assert(await page.locator('#power').isDisabled());
  offline = false;
  await page.waitForFunction(() => document.getElementById('connectionText').textContent === 'Đã kết nối');
  await page.waitForTimeout(1000);
  assert.equal(commands.length, commandCount); assert.equal(networkCalls.length, networkCount);
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
  await page.locator('summary').nth(1).click();
  await page.locator('#firmware').setInputFiles({ name: 'firmware.bin', mimeType: 'application/octet-stream', buffer: Buffer.alloc(64) });
  await page.locator('#uploadFirmware').click();
  await page.waitForFunction(() => document.getElementById('otaStatus').textContent.includes('thành công'));
  assert.equal(otaCalls, 1);
  assert(await page.locator('#scanWiFi').isDisabled());
  assert.equal(errors.length, 0, errors.join('\n'));
  await browser.close();
  console.log('PASS: light/favorites/timer/OTA, asynchronous scan/select/security/RSSI, hidden SSID, hostname validation/collision/copy fallback, WiFi failure/AP status, draft preservation, open-network retry, offline recovery without replay, mobile/desktop layout, no JS errors');
})().catch(e => { console.error(e); process.exit(1); });
