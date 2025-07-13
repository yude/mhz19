#include <MHZ19_uart.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>
#include <WebServer.h>

const int rx_pin = 16; // Serial rx pin no
const int tx_pin = 17; // Serial tx pin no

const char *ssid = "";
const char *password = "";
const char *hostname = "co2.tt2.yude.jp";
char wifi_res[256];

TaskHandle_t pSubProcess;
WebServer Server(80);

int co2ppm = -1;
MHZ19_uart mhz19;

void SendMessage() {
  Serial.println("Got http request");
  char res[1024];
  
  String message = "";
  message += "# HELP co2ppm measured CO2 parts per million\n";
  message += "# TYPE co2ppm gauge\n";
  message += "co2ppm{host=" + String(hostname) + "} " + String(co2ppm) + "\n";
  
  Server.send(200, "text/plain", message);
}

void SendNotFound() {
  Serial.println("Got http request to 404");
  Server.send(400, "text/plain", "404 Not Found");
}

void subProcess(void *param) {
  while (1) {
    co2ppm = mhz19.getCO2PPM();

    Serial.print("co2: ");
    Serial.println(co2ppm);

    delay(5000);
  }
}

void setup()
{
  Serial.begin(9600);

  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Still trying to connect to Wi-Fi ...");
  }
  
  sprintf(
    wifi_res,
    "Successfully connected to %s. I am %s",
    ssid,
    WiFi.localIP().toString().c_str()
  );
  Serial.println(wifi_res);

  Server.on("/", SendMessage);
  Server.onNotFound(SendNotFound);

  mhz19.begin(rx_pin, tx_pin);
  mhz19.setAutoCalibration(true);

  Serial.println("MH-Z19 is warming up now.");
  delay(10 * 1000);

  xTaskCreatePinnedToCore(
    subProcess,
    "subProcess",
    2048,
    NULL,
    1,
    &pSubProcess,
    0);

  Server.begin();
}

void loop() {
  Server.handleClient();
}
