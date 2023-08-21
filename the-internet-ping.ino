#include <ESP_Color.h>
#include <Pinger.h>
#include <ESP8266WiFi.h>
#include "settings.h"
extern "C"
{
#include <lwip/icmp.h> // needed for icmp packet definitions
}

#define pwmsteps 1000

// Set global to avoid object removing after setup() routine
Pinger pinger;

void setRGB(float rf, float gf, float bf) {
  rf = min(rf, 1.0f);
  gf = min(gf, 1.0f);
  bf = min(bf, 1.0f);

  rf = max(rf, 0.0f);
  gf = max(gf, 0.0f);
  bf = max(bf, 0.0f);

  analogWrite(LED_R, pwmsteps - rf * pwmsteps);
  analogWrite(LED_G, pwmsteps - gf * pwmsteps);
  analogWrite(LED_B, pwmsteps - bf * pwmsteps);
}

void setHSV(float H, float S, float V) {
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  ESP_Color::Color rgb(1.0f, 0.0f, 0.0f);
  rgb = rgb.FromHsv(H / 360.0f, S, V);
  analogWrite(LED_R, pwmsteps - rgb.R * pwmsteps);
  analogWrite(LED_G, pwmsteps - rgb.G * pwmsteps);
  analogWrite(LED_B, pwmsteps - rgb.B * pwmsteps);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  analogWriteRange(pwmsteps);

  setRGB(0, 0, 0);
  // Turn the LED on (Note that LOW is the voltage level
  analogWrite(LED_BUILTIN, 0);
  Serial.begin(115200);
  bool stationConnected = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  if (!stationConnected) {
    Serial.println("Error, unable to connect specified WiFi network.");
  }

  Serial.print("Connecting to AP...");
  float h = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    // Serial.print(".");

    setHSV(h, 1.0, 1.0);
    h += 1;
    if (h >= 360) {
      h = 0;
    }
  }
  Serial.print("Ok\n");

  // analogWrite(LED_BUILTIN, pwmsteps);
  for (float v = 1; v > 0; v -= 0.01) {
    delay(20);
    setHSV(h, 1, v);
  }

  pinger.OnReceive([](const PingerResponse& response) {
    if (response.ReceivedResponse) {

      float h = (response.ResponseTime * 1.0f);
      if (false) {
        // Green to Red mode
        h = max(0.0f, h);
        h = min(120.0f, h);
        h = 120 - h;
      } else {
        // Red to Mauve mode: https://tardis.fandom.com/wiki/Mauve_alert
        h = h - 10;
        h = max(0.0f, h);
        h = min(40.0f, h);
        h = 360 + -1 * h;
      }
      Serial.printf(
        "Reply from %s: bytes=%d time=%lums TTL=%d, h=%f\n",
        response.DestIPAddress.toString().c_str(),
        response.EchoMessageSize - sizeof(struct icmp_echo_hdr),
        response.ResponseTime,
        response.TimeToLive, h);

      setHSV(h, 1, 1);

    } else {
      Serial.printf("Request timed out.\n");
      setRGB(1, 0, 1);
    }

    // Return true to continue the ping sequence.
    // If current event returns false, the ping sequence is interrupted.
    return true;
    });
}

void loop() {
  pinger.Ping(PING_TARGET, 1);
  delay(200);
}
