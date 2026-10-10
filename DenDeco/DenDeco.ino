/* Den Deco 1.1.0 — ESP32-C3 SuperMini / 20 WS2812B / four TTP223.
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
#include <atomic>
#include <mdns.h>
#include <esp_wifi.h>
#include "Config.h"
#include "Core.h"
#include "Storage.h"
#include "WebUI.h"
using namespace Deco;
const char* VERSION="1.1.0";
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
ConnectionPolicy connection;
std::atomic<int> disconnectReason{0};
uint32_t networkApplyAt=0,restartAt=0,lastFrame=0;
bool restartPending=false;
bool wifiBeginPending=false;
uint32_t wifiBeginAt=0;
String scanStatus="idle",scanMessage;
JsonDocument scanResults;
bool scanRunning=false,scanStarted=false;
uint32_t scanStartedAt=0;
uint32_t mdnsCheckedAt=0;
uint16_t hostnameCollision=0;
String otaError;
size_t otaExpected=0,otaReceived=0,otaHeaderCount=0;
uint8_t otaHeader[sizeof(esp_image_header_t)+8+4];
uint32_t otaLastTraffic=0;

bool homeOnline(){return network.ssid.length()>0&&!wifiBeginPending&&WiFi.status()==WL_CONNECTED&&uint32_t(WiFi.localIP())!=0;}
void stopMDNS(){MDNS.end();mdnsRunning=false;hostname=network.hostname;hostnameCollision=0;mdnsCheckedAt=millis()-2000;}
void clearScan(){if(scanRunning){esp_wifi_scan_stop();WiFi.scanDelete();}scanRunning=false;scanStatus="idle";scanMessage="";scanResults.clear();}
void startAP(){
 if(apRunning)return;
 WiFi.mode(WIFI_AP_STA);
 apRunning=WiFi.softAP(apName.c_str()); // Open AP is intentional; only enabled for recovery.
 if(apRunning)Serial.printf("AP: %s (open) / http://192.168.4.1\n",apName.c_str());
 else Serial.println("AP start failed; retrying");
}
void stopAP(){
 // Arduino ESP32 2.0.17 softAPdisconnect(true) inverts enableAP's bool return.
 // Disable the AP interface directly so runtime state matches the radio mode.
 if(apRunning&&WiFi.enableAP(false)){apRunning=false;Serial.println("Home WiFi ready; AP disabled");}
}
void connectWiFi(){
 stopMDNS();clearScan();
 WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);
 WiFi.mode(apRunning?WIFI_AP_STA:WIFI_STA);
 WiFi.setHostname(network.hostname.c_str());WiFi.setSleep(false);
 disconnectReason.store(0);connection.begin(millis(),network.ssid.length()>0);
 // Disconnect is asynchronous. Waiting for its event avoids begin() treating
 // stale WL_CONNECTED as a successful retry of the same saved credentials.
 wifiBeginPending=network.ssid.length()>0;wifiBeginAt=millis();
 if(!wifiBeginPending)startAP();
}
void resetWiFi(){
 NetworkConfig next=network;next.ssid="";next.password="";
 if(!netStore.save(next)){Serial.println("WiFi reset failed: NVS write unavailable");return;}
 network=next;networkPending=false;connectWiFi();
 // Preserve lamp settings, favorites, name and requested hostname.
 renderer.feedback(millis());
}
const char* networkStatus(){
 switch(connection.phase){case ConnectionPolicy::Unconfigured:return "unconfigured";case ConnectionPolicy::Connecting:return "connecting";case ConnectionPolicy::Connected:return "connected";default:return "failed";}
}
String networkMessage(){
 if(connection.phase==ConnectionPolicy::Unconfigured)return "Chưa lưu WiFi nhà. Kết nối AP mở để cài đặt.";
 if(connection.phase==ConnectionPolicy::Connected)return "Đã kết nối WiFi nhà và nhận địa chỉ IP. AP đã tắt.";
 if(connection.phase==ConnectionPolicy::Connecting)return "Đang kết nối WiFi 2,4 GHz và chờ địa chỉ IP (tối đa 30 giây).";
 int reason=disconnectReason.load();
 if(reason==201)return "Không tìm thấy mạng đã lưu. Kiểm tra SSID và WiFi 2,4 GHz; AP dự phòng đang mở.";
 if(reason==202||reason==15||reason==204)return "Xác thực WiFi thất bại. Kiểm tra mật khẩu hoặc bảo mật router; AP dự phòng đang mở.";
 return "Chưa kết nối được hoặc chưa nhận IP sau 30 giây. AP dự phòng đang mở; đèn sẽ thử lại.";
}
const char* securityName(wifi_auth_mode_t auth){
 switch(auth){case WIFI_AUTH_OPEN:return "Mở";case WIFI_AUTH_WEP:return "WEP";case WIFI_AUTH_WPA_PSK:return "WPA";case WIFI_AUTH_WPA2_PSK:return "WPA2";case WIFI_AUTH_WPA_WPA2_PSK:return "WPA/WPA2";case WIFI_AUTH_WPA2_ENTERPRISE:return "Enterprise";case WIFI_AUTH_WPA3_PSK:return "WPA3";case WIFI_AUTH_WPA2_WPA3_PSK:return "WPA2/WPA3";default:return "Bảo mật";}
}
void scanTick(uint32_t now){
 if(!scanRunning)return;
 int count=WiFi.scanComplete();
 if(count==WIFI_SCAN_RUNNING&&uint32_t(now-scanStartedAt)<15000)return;
 if(count<0){esp_wifi_scan_stop();WiFi.scanDelete();scanRunning=false;scanStatus="error";scanMessage="Quét chưa thành công; thử lại khi WiFi ổn định.";return;}
 struct Found {String ssid;int rssi,channel;wifi_auth_mode_t auth;};
 Found found[20];int used=0;
 for(int i=0;i<count;i++){
  String ssid=WiFi.SSID(i);if(!ssid.length())continue;
  int rssi=WiFi.RSSI(i),slot=-1;
  for(int j=0;j<used;j++)if(found[j].ssid==ssid){slot=j;break;}
  if(slot>=0&&rssi<=found[slot].rssi)continue;
  if(slot<0){if(used<20)slot=used++;else{slot=0;for(int j=1;j<used;j++)if(found[j].rssi<found[slot].rssi)slot=j;if(rssi<=found[slot].rssi)continue;}}
  found[slot]={ssid,rssi,WiFi.channel(i),WiFi.encryptionType(i)};
 }
 for(int i=1;i<used;i++){Found current=found[i];int j=i;while(j&&found[j-1].rssi<current.rssi){found[j]=found[j-1];j--;}found[j]=current;}
 scanResults.clear();JsonArray list=scanResults["networks"].to<JsonArray>();
 for(int i=0;i<used;i++){JsonObject item=list.add<JsonObject>();item["ssid"]=found[i].ssid;item["rssi"]=found[i].rssi;item["channel"]=found[i].channel;item["secure"]=found[i].auth!=WIFI_AUTH_OPEN;item["security"]=securityName(found[i].auth);}
 WiFi.scanDelete();scanRunning=false;scanStatus="ready";scanMessage=used?"Chọn mạng WiFi 2,4 GHz trong danh sách.":"Chưa tìm thấy mạng; bạn vẫn có thể nhập SSID mạng ẩn.";
}
void networkTick(uint32_t now){
 now=millis(); // Touch reset/WiFi calls may have advanced time since loop began.
 if(networkPending&&uint32_t(now-networkApplyAt)>=1200){networkPending=false;connectWiFi();return;}
 scanTick(now);
 if(wifiBeginPending&&uint32_t(now-wifiBeginAt)>=50&&WiFi.status()!=WL_CONNECTED){
  wifiBeginPending=false;WiFi.begin(network.ssid.c_str(),network.password.length()?network.password.c_str():nullptr);
 }
 ConnectionPolicy::Phase before=connection.phase;
 ConnectionPolicy::Action action=connection.update(now,homeOnline(),scanRunning);
 if(connection.phase==ConnectionPolicy::Connected){
  stopAP();
  if(uint32_t(now-mdnsCheckedAt)>=1000){
   mdnsCheckedAt=now;
   if(!mdnsRunning){mdnsRunning=MDNS.begin(hostname.c_str());if(mdnsRunning){MDNS.setInstanceName(network.name);MDNS.addService("http","tcp",80);Serial.printf("LAN: http://%s.local / %s\n",hostname.c_str(),WiFi.localIP().toString().c_str());}}
   else if(!mdns_hostname_exists(hostname.c_str())){
    // The SDK may rename during probing. Choose an explicit per-device name
    // so the web UI can report the address that is actually advertised.
    ++hostnameCollision;String candidate=network.hostname+"-"+suffix;candidate.toLowerCase();
    if(hostnameCollision>1)candidate+="-"+String(hostnameCollision);
    if(mdns_hostname_set(candidate.c_str())==ESP_OK){hostname=candidate;Serial.printf("mDNS name collision: http://%s.local\n",hostname.c_str());}
   }
  }
 }else{
  if(before==ConnectionPolicy::Connected){connectWiFi();return;}
  if(action==ConnectionPolicy::OpenAP){if(before==ConnectionPolicy::Connecting){wifiBeginPending=false;WiFi.disconnect(false,false);}startAP();}
  else if(action==ConnectionPolicy::Retry)connectWiFi();
 }
}

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
 if(e==TouchEvent::WiFiResetHold){resetWiFi();return;}
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
 d["ssid"]=network.ssid;d["wifiOpen"]=network.ssid.length()>0&&network.password.length()==0;d["wifiConnected"]=homeOnline();d["ip"]=homeOnline()?WiFi.localIP().toString():String("");d["apActive"]=apRunning;d["apSSID"]=apName;d["hostname"]=hostname;
 d["requestedHostname"]=network.hostname;d["localUrl"]="http://"+hostname+".local";d["mdnsReady"]=mdnsRunning;
 d["networkStatus"]=networkStatus();d["networkMessage"]=networkMessage();d["disconnectReason"]=disconnectReason.load();
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
 if(!d["hostname"].isNull()){
  if(!d["hostname"].is<String>()){fail(400,"Tên địa chỉ không hợp lệ");return;}
  next.hostname=d["hostname"].as<String>();next.hostname.trim();next.hostname.toLowerCase();
  if(!validHostname(next.hostname.c_str())){fail(400,"Tên địa chỉ cần 1–48 ký tự a–z, 0–9, dấu -; không thêm .local");return;}
 }
 bool changeWiFi=false;
 if(d["forget"].is<bool>()&&d["forget"].as<bool>()){next.ssid="";next.password="";changeWiFi=true;}
 else if(!d["ssid"].isNull()){
  if(!d["ssid"].is<String>()||!d["password"].is<String>()||!d["open"].is<bool>()){fail(400,"Thiếu thông tin WiFi");return;}
  String ssid=d["ssid"].as<String>(),password=d["password"].as<String>();bool open=d["open"].as<bool>();
  if(ssid.length()<1||ssid.length()>32){fail(400,"SSID cần 1–32 byte");return;}
  if(open)password="";else if(password.length()==0&&ssid==network.ssid)password=network.password;
  if(!open&&(password.length()<8||password.length()>63)){fail(400,"Mật khẩu cần 8–63 ký tự; chọn mạng mở nếu không có mật khẩu");return;}
  changeWiFi=true;next.ssid=ssid;next.password=password; // Explicit retry even with unchanged credentials.
 }
 if(scanRunning){fail(409,"Chờ quét WiFi hoàn tất trước khi lưu cấu hình");return;}
 if(!netStore.save(next)){fail(500,"Không lưu được cấu hình mạng");return;}
 bool rename=next.hostname!=network.hostname;bool renameLamp=next.name!=network.name;network=next;
 if(rename){stopMDNS();WiFi.setHostname(network.hostname.c_str());}
 else if(renameLamp&&mdnsRunning)MDNS.setInstanceName(network.name);
 if(changeWiFi){networkPending=true;networkApplyAt=millis();}
 JsonDocument response;response["ok"]=true;response["reconnect"]=changeWiFi;response["localUrl"]="http://"+hostname+".local";
 response["message"]=changeWiFi?"Đã lưu. Đèn sẽ thử kết nối; khi thành công AP sẽ tắt. Chuyển điện thoại sang WiFi nhà và mở liên kết .local.":"Đã lưu cấu hình đèn.";sendJson(response);
}
void getScan(){
 if(!authorize())return;
 JsonDocument response;response["status"]=scanStatus;response["message"]=scanMessage;
 if(scanStatus=="ready")response["networks"]=scanResults["networks"];
 else response["networks"].to<JsonArray>();
 sendJson(response);
}
void startScan(){
 JsonDocument d;if(!parseBody(d))return;
 if(connection.phase==ConnectionPolicy::Connecting){fail(409,"Đang thử kết nối WiFi; chờ tối đa 30 giây rồi quét lại");return;}
 if(scanRunning){getScan();return;}
 uint32_t now=millis();if(scanStarted&&uint32_t(now-scanStartedAt)<2000){fail(429,"Chờ 2 giây trước khi quét lại");return;}
 clearScan();scanStarted=true;scanStartedAt=now;
 int result=WiFi.scanNetworks(true,false,false,120);
 scanRunning=result==WIFI_SCAN_RUNNING||result>=0;
 scanStatus=scanRunning?"scanning":"error";scanMessage=scanRunning?"Đang quét WiFi 2,4 GHz…":"Chưa bắt đầu được quét; vui lòng thử lại.";
 getScan();
}
void abortOTA(const String& message){otaError=message;if(Update.isRunning())Update.abort();otaBusy=false;}
bool validateOTAHeader(){esp_image_header_t h;memcpy(&h,otaHeader,sizeof(h));return h.magic==ESP_IMAGE_HEADER_MAGIC&&h.chip_id==ESP_CHIP_ID_ESP32C3&&get32(otaHeader+sizeof(h)+8)==ESP_APP_DESC_MAGIC_WORD;}
void uploadOTA(){
 HTTPUpload& u=server.upload();otaLastTraffic=millis();
 if(u.status==UPLOAD_FILE_START){
  otaSuccess=false;otaError="";otaExpected=otaReceived=otaHeaderCount=0;
  // Validate without sending a second response from the multipart callback.
  String origin=server.header("Origin");if((origin.length()&&origin!="http://"+server.hostHeader())||server.header("Sec-Fetch-Site")=="cross-site"||server.header("X-Deco-Token")!=sessionToken){otaError="Phiên OTA không hợp lệ";return;}
  if(otaBusy||restartPending||networkPending||scanRunning||connection.phase==ConnectionPolicy::Connecting){otaError="Đèn đang chuyển mạng hoặc quét WiFi; chờ ổn định rồi OTA";return;}
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
 pinMode(Config::POWER_PIN,INPUT_PULLDOWN);pinMode(Config::MINUS_PIN,INPUT_PULLDOWN);pinMode(Config::PLUS_PIN,INPUT_PULLDOWN);pinMode(Config::MODE_PIN,INPUT_PULLDOWN);
 leds.begin();leds.clear();leds.show();storage.begin(lamp.settings);renderer.begin(lamp.settings,millis());touch.begin(millis());
 uint64_t mac=ESP.getEfuseMac();char id[7];snprintf(id,sizeof(id),"%06X",uint32_t(mac>>24)&0xffffffu);suffix=id;apName="DenDeco-"+suffix;sessionToken=randomText(32);
 bool networkOK=netStore.begin(network);if(!networkOK)Serial.println("Network NVS unavailable or invalid; using provisioning defaults");
 hostname=network.hostname;
 WiFi.persistent(false);WiFi.mode(WIFI_STA);
 WiFi.onEvent([](WiFiEvent_t event,WiFiEventInfo_t info){if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED&&info.wifi_sta_disconnected.reason!=8)disconnectReason.store(info.wifi_sta_disconnected.reason);});
 connectWiFi();
 const char* headers[]={"Origin","Content-Type","X-Deco-Token","Sec-Fetch-Site","X-Firmware-Size"};server.collectHeaders(headers,5);
 server.on("/",HTTP_GET,[]{noCache();server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; connect-src 'self'; img-src 'self' data:; frame-ancestors 'none'");server.send_P(200,"text/html; charset=utf-8",WEB_UI);});
 server.on("/api/state",HTTP_GET,getState);server.on("/api/command",HTTP_POST,handleCommand);server.on("/api/network",HTTP_POST,handleNetwork);
 server.on("/api/scan",HTTP_GET,getScan);server.on("/api/scan",HTTP_POST,startScan);
 server.on("/api/ota",HTTP_POST,finishOTA,uploadOTA);
 server.onNotFound([]{fail(404,"Không tìm thấy đường dẫn");});server.begin();Serial.printf("Den Deco %s ready; %u LEDs\n",VERSION,Config::LED_COUNT);
}
void loop(){
 uint32_t now=millis();
 if(otaBusy&&uint32_t(now-otaLastTraffic)>30000)abortOTA("Đã hết thời gian tải lên");
 if(restartPending&&uint32_t(now-restartAt)>=1800){ESP.restart();return;}
 if(!otaBusy&&!restartPending){
  bool raw[]={digitalRead(Config::POWER_PIN)==HIGH,digitalRead(Config::MINUS_PIN)==HIGH,digitalRead(Config::PLUS_PIN)==HIGH,digitalRead(Config::MODE_PIN)==HIGH};touch.scan(raw,now,onTouch);
  now=millis(); // Touch callbacks may start a timer/reset at a newer timestamp.
  Result r=lamp.tick(now);if(r.changed){storage.changed(lamp.settings,now);renderer.retarget(lamp.settings,now);}
  storage.tick(lamp.settings,now);
  if(uint32_t(now-lastFrame)>=33){lastFrame=now;renderFrame(now);}
  networkTick(now);
 }
 server.handleClient();delay(1);
}
