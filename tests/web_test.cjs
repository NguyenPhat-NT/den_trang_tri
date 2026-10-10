const { chromium }=require('playwright');
const fs=require('fs');
const assert=require('assert');
const path=require('path');
(async()=>{
 const root=path.resolve(__dirname,'..');
 const source=fs.readFileSync(path.join(root,'DenDeco/WebUI.h'),'utf8');
 const html=source.split('R"DECOHTML(')[1].split(')DECOHTML"')[0];
 let s={version:'1.0.0',name:'Đèn Deco',token:'test-session',revision:0,power:false,light:{mode:0,r:255,g:96,b:24,brightness:5,speed:50},sleepBrightness:5,cap:30,ledCount:20,favorites:Array.from({length:3},()=>({valid:false,light:{mode:0,brightness:5}})),timerActive:false,remainingMs:0,hasSaved:true,savePending:false,saveError:'',ssid:'Home',wifiConnected:true,ip:'192.168.1.10',apActive:false,apSSID:'DenDeco-TEST',hostname:'den-deco-test',otaMaxBytes:1310720};
 let commands=[],offline=false,otaCalls=0,networkCalls=[];
 const browser=await chromium.launch({headless:true,executablePath:process.env.DECO_BROWSER||'/usr/bin/chromium',args:['--no-sandbox']});
 const page=await browser.newPage({viewport:{width:390,height:844}});const errors=[];
 page.on('pageerror',e=>errors.push(e.message));
 await page.route('http://deco.test/**',async route=>{
  const req=route.request();const url=new URL(req.url());
  if(url.pathname==='/'){return route.fulfill({status:200,contentType:'text/html',body:html});}
  if(offline){return route.abort();}
  if(url.pathname==='/api/state'){return route.fulfill({json:s});}
  if(url.pathname==='/api/command'){
   assert.equal(req.headers()['x-deco-token'],s.token);const c=req.postDataJSON();commands.push(c);s.revision++;
   if(c.type==='power')s.power=c.value;
   if(c.type==='brightness')s.light.brightness=c.value;
   if(c.type==='color'){s.light.mode=1;s.light.r=c.r;s.light.g=c.g;s.light.b=c.b;}
   if(c.type==='favoriteSave')s.favorites[c.value]={valid:true,light:{...s.light}};
   if(c.type==='timerStart'){s.timerActive=true;s.remainingMs=c.value*60000;}
   if(c.type==='timerCancel')s.timerActive=false;
   return route.fulfill({json:s});
  }
  if(url.pathname==='/api/network'){networkCalls.push(req.postDataJSON());s.name=req.postDataJSON().name;return route.fulfill({json:{ok:true,reconnect:false}});}
  if(url.pathname==='/api/ota'){assert.equal(req.headers()['x-deco-token'],s.token);assert.equal(req.headers()['x-firmware-size'],'64');otaCalls++;return route.fulfill({json:{ok:true,message:'Cập nhật thành công; đèn đang khởi động lại'}});}
  return route.fulfill({status:404,json:{error:'Unknown'}});
 });
 await page.goto('http://deco.test/');await page.waitForFunction(()=>document.getElementById('connectionText').textContent==='Đã kết nối');
 assert.equal(await page.locator('#brightnessText').textContent(),'5');assert(await page.locator('#startTimer').isDisabled());
 await page.locator('#power').click();await page.waitForFunction(()=>document.getElementById('power').textContent==='Tắt đèn');
 await page.locator('#brightness').evaluate(e=>{e.value='38';e.dispatchEvent(new Event('input'));e.dispatchEvent(new Event('change'));});await page.waitForFunction(()=>document.getElementById('brightnessText').textContent==='38');
 await page.locator('[data-color="#529bff"]').click();await page.waitForFunction(()=>document.getElementById('mode').value==='1');
 await page.locator('[data-save="0"]').click();await page.waitForFunction(()=>!document.querySelector('[data-apply="0"]').disabled);
 await page.locator('[data-minutes="15"]').click();await page.waitForFunction(()=>document.getElementById('remaining').textContent.includes('15:00'));
 await page.locator('#cancelTimer').click();await page.waitForFunction(()=>document.getElementById('remaining').textContent==='Chưa hẹn giờ');
 await page.locator('summary').nth(0).click();await page.locator('#ssid').fill('NewWiFi');await page.locator('#password').fill('not-a-real-secret');await page.waitForTimeout(700);assert.equal(await page.locator('#ssid').inputValue(),'NewWiFi');
 await page.locator('#lampName').fill('Đèn bàn');await page.locator('#saveName').click();await page.waitForFunction(()=>document.getElementById('name').textContent==='Đèn bàn');assert.equal(networkCalls.length,1);
 assert.equal(await page.locator('#ssid').inputValue(),'NewWiFi');
 // Network failures disable controls; reconnect never submits old commands.
 offline=true;await page.waitForFunction(()=>document.getElementById('connectionText').textContent==='Mất kết nối');const before=commands.length;assert(await page.locator('#power').isDisabled());offline=false;await page.waitForFunction(()=>document.getElementById('connectionText').textContent==='Đã kết nối');assert.equal(commands.length,before);
 await page.screenshot({path:path.resolve(root,'../web-mobile.png'),fullPage:true});
 await page.setViewportSize({width:1280,height:900});await page.screenshot({path:path.resolve(root,'../web-desktop.png'),fullPage:true});await page.setViewportSize({width:390,height:844});
 await page.locator('summary').nth(1).click();page.on('dialog',d=>d.accept());
 await page.locator('#firmware').setInputFiles({name:'firmware.bin',mimeType:'application/octet-stream',buffer:Buffer.alloc(64)});await page.locator('#uploadFirmware').click();await page.waitForFunction(()=>document.getElementById('otaStatus').textContent.includes('thành công'));assert.equal(otaCalls,1);
 assert.equal(errors.length,0,errors.join('\n'));assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 await browser.close();console.log('PASS: mobile/desktop layout, power/brightness/color/favorites/timer, WiFi editing, offline recovery without replay, OTA upload UI and no JS errors');
})().catch(e=>{console.error(e);process.exit(1);});
