/* Den Deco 1.0.0 — ESP32-C3 SuperMini / 20 WS2812B / four TTP223.
 * Arduino ESP32 2.0.17, Adafruit NeoPixel 1.12.5, ArduinoJson 7.4.2.
 * Select an OTA partition scheme; initial installation uses USB.
 */
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include "Config.h"
#include "Core.h"
#include "Storage.h"
#include "WebUI.h"
using namespace Deco;
const char* VERSION="1.0.0";
Controller lamp;
Renderer renderer;
Touch touch;
Storage storage;
NetworkStorage netStore;
NetworkConfig network;
WebServer server(80);
Adafruit_NeoPixel leds(Config::LED_COUNT,Config::LED_PIN,NEO_GRB+NEO_KHZ800);
String suffix,apName,hostname,sessionToken;
bool apRunning=false,mdnsRunning=false,networkPending=false,otaBusy=false,otaSuccess=false;
uint32_t disconnectedAt=0,lastReconnect=0,networkApplyAt=0,restartAt=0,lastFrame=0;
bool disconnectedTracking=false,restartPending=false;
String otaError;
size_t otaExpected=0,otaReceived=0,otaHeaderCount=0;
uint8_t otaHeader[sizeof(esp_image_header_t)+8+4];
uint32_t otaLastTraffic=0;

String randomText(size_t length){const char* alphabet="ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789";String s;for(size_t i=0;i<length;i++)s+=alphabet[esp_random()%strlen(alphabet)];return s;}
void renderFrame(uint32_t now){uint8_t frame[Config::LED_COUNT][3];renderer.frame(lamp.settings,lamp.timer,now,frame);for(uint16_t i=0;i<Config::LED_COUNT;i++)leds.setPixelColor(i,frame[i][0],frame[i][1],frame[i][2]);leds.show();}
Result execute(const Command& c,uint32_t now){
 renderFrame(now); // Sample an in-flight transition before retargeting.
 Settings before=lamp.settings;bool timerBefore=lamp.timer.active;uint32_t startBefore=lamp.timer.start,durationBefore=lamp.timer.duration;
 Result r=lamp.apply(c,now);
 if(r.changed)storage.changed(lamp.settings,now);
 if(!(before.light==lamp.settings.light)||before.power!=lamp.settings.power||before.cap!=lamp.settings.cap||timerBefore!=lamp.timer.active||startBefore!=lamp.timer.start||durationBefore!=lamp.timer.duration)renderer.retarget(lamp.settings,now);
 if(r.feedback)renderer.feedback(now);
 return r;
}
void onTouch(TouchEvent e){
 uint32_t now=millis();if(otaBusy||restartPending)return;
 if(e==TouchEvent::PowerShort){execute({Action::Power,!lamp.settings.power},now);return;}
 if(e==TouchEvent::SleepHold){execute({Action::SleepNow},now);return;}
 if(!lamp.settings.power)return;
 switch(e){
  case TouchEvent::MinusShort:execute({Action::Brightness,clampBrightness(lamp.settings.light.brightness-5)},now);break;
  case TouchEvent::PlusShort:execute({Action::Brightness,clampBrightness(lamp.settings.light.brightness+5)},now);break;
  case TouchEvent::MinusStep:execute({Action::Brightness,clampBrightness(lamp.settings.light.brightness-1)},now);break;
  case TouchEvent::PlusStep:execute({Action::Brightness,clampBrightness(lamp.settings.light.brightness+1)},now);break;
  case TouchEvent::ModeShort:execute({Action::Mode,(lamp.settings.light.mode+1)%ModeCount},now);break;
  case TouchEvent::TimerHold:execute({lamp.timer.active?Action::TimerCancel:Action::TimerStart,30},now);break;
  default:break;
 }
}
void startAP(){
 if(apRunning)return;
 WiFi.mode(WIFI_AP_STA);
 if(!WiFi.softAP(apName.c_str(),network.apPassword.c_str())){Serial.println("AP start failed");return;}
 apRunning=true;
 Serial.printf("AP: %s\nAP password: %s\nOpen http://192.168.4.1\n",apName.c_str(),network.apPassword.c_str());
}
void connectWiFi(){
 if(mdnsRunning){MDNS.end();mdnsRunning=false;}
 WiFi.disconnect(false,false);WiFi.mode(apRunning?WIFI_AP_STA:WIFI_STA);WiFi.setHostname(hostname.c_str());WiFi.setAutoReconnect(true);
 disconnectedAt=lastReconnect=millis();disconnectedTracking=true;
 if(network.ssid.length())WiFi.begin(network.ssid.c_str(),network.password.c_str());else startAP();
}
void networkTick(uint32_t now){
 if(networkPending&&uint32_t(now-networkApplyAt)>=1200){networkPending=false;connectWiFi();}
 if(WiFi.status()==WL_CONNECTED){
  disconnectedTracking=false;
  if(!mdnsRunning){mdnsRunning=MDNS.begin(hostname.c_str());if(mdnsRunning)MDNS.addService("http","tcp",80);Serial.printf("LAN: http://%s / http://%s.local\n",WiFi.localIP().toString().c_str(),hostname.c_str());}
 }else{
  if(mdnsRunning){MDNS.end();mdnsRunning=false;}
  if(!disconnectedTracking){disconnectedTracking=true;disconnectedAt=lastReconnect=now;}
  if(!network.ssid.length()||uint32_t(now-disconnectedAt)>=Config::WIFI_FALLBACK_MS)startAP();
  if(network.ssid.length()&&uint32_t(now-lastReconnect)>=30000){lastReconnect=now;WiFi.reconnect();}
 }
 // Once opened, fallback AP remains available until reboot, including while editing WiFi.
}
void noCache(){server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Content-Type-Options","nosniff");}
void sendJson(JsonDocument& d,int status=200){String body;serializeJson(d,body);noCache();server.send(status,"application/json; charset=utf-8",body);}
void fail(int code,const String& message){JsonDocument d;d["error"]=message;sendJson(d,code);}
bool authorize(){
 String origin=server.header("Origin");String host=server.hostHeader();
 if((origin.length()&&origin!="http://"+host)||server.header("Sec-Fetch-Site")=="cross-site"||server.header("X-Deco-Token")!=sessionToken){fail(403,"Phiên điều khiển không hợp lệ; tải lại trang");return false;}
 return true;
}
bool parseBody(JsonDocument& d){
 if(!authorize())return false;
 if(otaBusy||restartPending||networkPending){fail(409,"Đèn đang cập nhật hoặc chuyển mạng");return false;}
 String content=server.header("Content-Type");if(!content.startsWith("application/json")){fail(415,"Cần dữ liệu JSON");return false;}
 String body=server.arg("plain");if(body.length()>1536){fail(413,"Yêu cầu quá lớn");return false;}
 if(deserializeJson(d,body)||!d.is<JsonObject>()){fail(400,"Dữ liệu không hợp lệ");return false;}return true;
}
void stateDocument(JsonDocument& d){
 uint32_t now=millis();d["version"]=VERSION;d["name"]=network.name;d["token"]=sessionToken;d["revision"]=lamp.revision;d["power"]=lamp.settings.power;
 lightToJson(d["light"].to<JsonObject>(),lamp.settings.light);d["sleepBrightness"]=lamp.settings.sleepBrightness;d["cap"]=lamp.settings.cap;d["ledCount"]=Config::LED_COUNT;
 JsonArray a=d["favorites"].to<JsonArray>();for(auto& f:lamp.settings.favorites){JsonObject o=a.add<JsonObject>();o["valid"]=f.valid;lightToJson(o["light"].to<JsonObject>(),f.light);}
 d["timerActive"]=lamp.timer.active;d["remainingMs"]=lamp.timer.remaining(now);d["hasSaved"]=storage.hasSaved;d["savePending"]=storage.dirty;d["saveError"]=storage.error;d["saveBlocked"]=storage.blocked;d["writes"]=storage.writes;
 d["ssid"]=network.ssid;d["wifiConnected"]=WiFi.status()==WL_CONNECTED;d["ip"]=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():String("");d["apActive"]=apRunning;d["apSSID"]=apName;d["hostname"]=hostname;
 const esp_partition_t* part=esp_ota_get_next_update_partition(nullptr);d["otaMaxBytes"]=part?part->size:0;d["uptimeMs"]=now;
}
void getState(){JsonDocument d;stateDocument(d);sendJson(d);}
void handleCommand(){
 JsonDocument d;if(!parseBody(d))return;String type=d["type"].as<String>();Command c(Action::Power);int n=0;bool valid=true;
 if(type=="power"){valid=d["value"].is<bool>();c=Command(Action::Power,d["value"].as<bool>());}
 else if(type=="brightness"){valid=readInt(d["value"],1,100,n);c=Command(Action::Brightness,n);}
 else if(type=="mode"){valid=readInt(d["value"],0,ModeCount-1,n);c=Command(Action::Mode,n);}
 else if(type=="speed"){valid=readInt(d["value"],1,100,n);c=Command(Action::Speed,n);}
 else if(type=="cap"){valid=readInt(d["value"],Config::MIN_CAP_PERCENT,Config::MAX_CAP_PERCENT,n);c=Command(Action::Cap,n);}
 else if(type=="color"){int r=0,g=0,b=0;valid=readInt(d["r"],0,255,r)&&readInt(d["g"],0,255,g)&&readInt(d["b"],0,255,b);c=Command(Action::Color,0,r,g,b);}
 else if(type=="sleep"){c=Command(Action::SleepNow);}
 else if(type=="favoriteSave"||type=="favoriteApply"){valid=readInt(d["value"],0,2,n);c=Command(type=="favoriteSave"?Action::FavoriteSave:Action::FavoriteApply,n);}
 else if(type=="timerStart"){valid=readInt(d["value"],1,180,n);c=Command(Action::TimerStart,n);}
 else if(type=="timerCancel"){c=Command(Action::TimerCancel);}
 else valid=false;
 if(!valid){fail(400,"Lệnh hoặc giá trị ngoài phạm vi");return;}
 Result r=execute(c,millis());if(!r.accepted){fail(409,"Bật đèn trước khi hẹn giờ; mục yêu thích cần được lưu trước");return;}getState();
}
void handleNetwork(){
 JsonDocument d;if(!parseBody(d))return;
 if(!d["name"].is<String>()){fail(400,"Thiếu tên đèn");return;}
 NetworkConfig next=network;next.name=d["name"].as<String>();next.name.trim();
 if(next.name.length()<1||next.name.length()>48){fail(400,"Tên đèn cần 1–48 byte UTF-8");return;}
 bool changeWiFi=false;
 if(d["forget"].is<bool>()&&d["forget"].as<bool>()){next.ssid="";next.password="";changeWiFi=true;}
 else if(!d["ssid"].isNull()){
  if(!d["ssid"].is<String>()||!d["password"].is<String>()||!d["open"].is<bool>()){fail(400,"Thiếu thông tin WiFi");return;}
  String ssid=d["ssid"].as<String>(),password=d["password"].as<String>();bool open=d["open"].as<bool>();
  if(ssid.length()<1||ssid.length()>32){fail(400,"SSID cần 1–32 byte");return;}
  if(open)password="";else if(password.length()==0&&ssid==network.ssid)password=network.password;
  if(!open&&(password.length()<8||password.length()>63)){fail(400,"Mật khẩu cần 8–63 ký tự; chọn mạng mở nếu không có mật khẩu");return;}
  changeWiFi=ssid!=network.ssid||password!=network.password;next.ssid=ssid;next.password=password;
 }
 if(!netStore.save(next)){fail(500,"Không lưu được cấu hình mạng");return;}
 network=next;if(changeWiFi){networkPending=true;networkApplyAt=millis();}
 JsonDocument response;response["ok"]=true;response["reconnect"]=changeWiFi;sendJson(response);
}
void abortOTA(const String& message){otaError=message;if(Update.isRunning())Update.abort();otaBusy=false;}
bool validateOTAHeader(){esp_image_header_t h;memcpy(&h,otaHeader,sizeof(h));return h.magic==ESP_IMAGE_HEADER_MAGIC&&h.chip_id==ESP_CHIP_ID_ESP32C3&&get32(otaHeader+sizeof(h)+8)==ESP_APP_DESC_MAGIC_WORD;}
void uploadOTA(){
 HTTPUpload& u=server.upload();otaLastTraffic=millis();
 if(u.status==UPLOAD_FILE_START){
  otaSuccess=false;otaError="";otaExpected=otaReceived=otaHeaderCount=0;
  // Validate without sending a second response from the multipart callback.
  String origin=server.header("Origin");if((origin.length()&&origin!="http://"+server.hostHeader())||server.header("Sec-Fetch-Site")=="cross-site"||server.header("X-Deco-Token")!=sessionToken){otaError="Phiên OTA không hợp lệ";return;}
  if(otaBusy||restartPending||networkPending){otaError="Đèn đang bận";return;}
  String sizeText=server.header("X-Firmware-Size");if(sizeText.length()==0||sizeText.length()>8){otaError="Thiếu kích thước firmware";return;}
  for(size_t i=0;i<sizeText.length();i++)if(sizeText[i]<'0'||sizeText[i]>'9'){otaError="Kích thước không hợp lệ";return;}
  otaExpected=sizeText.toInt();const esp_partition_t* part=esp_ota_get_next_update_partition(nullptr);
  if(!part||otaExpected<sizeof(otaHeader)||otaExpected>part->size){otaError="File quá lớn hoặc bảng phân vùng không hỗ trợ OTA";return;}
  // Explicit maintenance save before update: successful OTA reboot preserves latest configuration.
  if(!storage.saveNow(lamp.settings)){otaError="Chưa lưu được trạng thái; OTA chưa bắt đầu";return;}
  otaBusy=true;renderFrame(millis());
 }else if(u.status==UPLOAD_FILE_WRITE){
  if(!otaBusy||otaError.length())return;
  if(otaReceived+u.currentSize>otaExpected){abortOTA("File dài hơn kích thước đã khai báo");return;}
  size_t offset=0;
  if(otaHeaderCount<sizeof(otaHeader)){
   size_t take=sizeof(otaHeader)-otaHeaderCount;if(take>u.currentSize)take=u.currentSize;
   memcpy(otaHeader+otaHeaderCount,u.buf,take);otaHeaderCount+=take;offset+=take;
   if(otaHeaderCount==sizeof(otaHeader)){
    if(!validateOTAHeader()){abortOTA("Cần file ứng dụng .bin cho ESP32-C3; không dùng file gộp hoặc bootloader");return;}
    if(!Update.begin(otaExpected,U_FLASH)){abortOTA(String("Không mở được phân vùng OTA: ")+Update.errorString());return;}
    if(Update.write(otaHeader,sizeof(otaHeader))!=sizeof(otaHeader)){abortOTA("Không ghi được header firmware");return;}
   }
  }
  if(offset<u.currentSize&&Update.write(u.buf+offset,u.currentSize-offset)!=u.currentSize-offset){abortOTA(String("Lỗi ghi firmware: ")+Update.errorString());return;}
  otaReceived+=u.currentSize;
 }else if(u.status==UPLOAD_FILE_END){
  if(!otaBusy)return;
  if(otaReceived!=otaExpected||otaHeaderCount!=sizeof(otaHeader)){abortOTA("File chưa được gửi đầy đủ");return;}
  if(!Update.end(false)){abortOTA(String("Firmware chưa hợp lệ: ")+Update.errorString());return;}
  otaBusy=false;otaSuccess=true;restartPending=true;restartAt=millis();
 }else if(u.status==UPLOAD_FILE_ABORTED){abortOTA("Đã ngắt tải lên; firmware hiện tại vẫn được giữ");}
}
void finishOTA(){if(otaSuccess){JsonDocument d;d["ok"]=true;d["message"]="Cập nhật thành công; đèn đang khởi động lại";sendJson(d);}else fail(otaError=="Phiên OTA không hợp lệ"?403:400,otaError.length()?otaError:String("Chưa nhận được firmware"));}
void setup(){
 Serial.begin(115200); // Do not wait for a serial monitor before restoring the lamp.
 pinMode(Config::POWER_PIN,INPUT);pinMode(Config::MINUS_PIN,INPUT);pinMode(Config::PLUS_PIN,INPUT);pinMode(Config::MODE_PIN,INPUT);
 leds.begin();leds.clear();leds.show();storage.begin(lamp.settings);renderer.begin(lamp.settings,millis());touch.begin(millis());
 uint64_t mac=ESP.getEfuseMac();char id[7];snprintf(id,sizeof(id),"%06X",uint32_t(mac>>24)&0xffffffu);suffix=id;apName="DenDeco-"+suffix;hostname="den-deco-"+suffix;hostname.toLowerCase();sessionToken=randomText(32);
 bool networkOK=netStore.begin(network);if(!networkOK)Serial.println("Network NVS unavailable or invalid; using provisioning defaults");
 if(network.apPassword.length()!=12){network.apPassword=randomText(12);if(!netStore.save(network))Serial.println("AP password could not be saved; record current serial password");}
 connectWiFi();
 const char* headers[]={"Origin","Content-Type","X-Deco-Token","Sec-Fetch-Site","X-Firmware-Size"};server.collectHeaders(headers,5);
 server.on("/",HTTP_GET,[]{noCache();server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; connect-src 'self'; img-src 'self' data:; frame-ancestors 'none'");server.send_P(200,"text/html; charset=utf-8",WEB_UI);});
 server.on("/api/state",HTTP_GET,getState);server.on("/api/command",HTTP_POST,handleCommand);server.on("/api/network",HTTP_POST,handleNetwork);
 server.on("/api/ota",HTTP_POST,finishOTA,uploadOTA);
 server.onNotFound([]{fail(404,"Không tìm thấy đường dẫn");});server.begin();Serial.printf("Den Deco %s ready; %u LEDs\n",VERSION,Config::LED_COUNT);
}
void loop(){
 uint32_t now=millis();
 if(otaBusy&&uint32_t(now-otaLastTraffic)>30000)abortOTA("Đã hết thời gian tải lên");
 if(restartPending&&uint32_t(now-restartAt)>=1800){ESP.restart();return;}
 if(!otaBusy&&!restartPending){
  bool raw[]={digitalRead(Config::POWER_PIN)==HIGH,digitalRead(Config::MINUS_PIN)==HIGH,digitalRead(Config::PLUS_PIN)==HIGH,digitalRead(Config::MODE_PIN)==HIGH};touch.scan(raw,now,onTouch);
  Result r=lamp.tick(now);if(r.changed){storage.changed(lamp.settings,now);renderer.retarget(lamp.settings,now);}
  storage.tick(lamp.settings,now);
  if(uint32_t(now-lastFrame)>=33){lastFrame=now;renderFrame(now);}
  networkTick(now);
 }
 server.handleClient();delay(1);
}
