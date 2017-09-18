/**
 *    Alarm Monitor
 *        Uses a simple ESP8266 circuit to detect the sound of a (not-so-smart) alarm
 *
 *    Copyright 2017 Ross Lipenta
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *    in compliance with the License. You may obtain a copy of the License at:
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *    on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *    for the specific language governing permissions and limitations under the License.
 *
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include "AudioAlarm.h"

#ifdef DEBUG_ESP_PORT
#define DEBUG_OUTPUT DEBUG_ESP_PORT
#else
#define DEBUG_OUTPUT Serial
#endif

#define WLAN_SSID       "YOUR_SSID" //change to your Wifi SSID
#define WLAN_PASS       "YOUR_WIFI_PASSWORD" //change to your Wifi Password
#define SERVER_PORT     80
#define MDNS_NAME       "alarm-monitor"

MDNSResponder mdns;
ESP8266WebServer server(SERVER_PORT);

AudioAlarm alarm;

bool isAlarmTriggered = false;
String uuid;
String subscriberUrl;
int requestCounter = 0;

/**
 * Main setup
 *
 *  Connects to WiFi
 *  Sets up mDNS
 *  Starts SSDP Discovery
 *  Starts the Web Server
 *  Subscribes to the Alarm events
*/
void setup() {
  Serial.begin(9600);

  DEBUG_OUTPUT.println("Alarm-Monitor is Starting Up.");

  //Connect to WiFi
  DEBUG_OUTPUT.print("  Connecting to WiFi...");
  connectToNetwork();
  if (mdns.begin(MDNS_NAME, WiFi.localIP())) {
    DEBUG_OUTPUT.println("    MDNS responder started. [" + String(MDNS_NAME) + "]");
    mdns.addService("http", "tcp", SERVER_PORT);
  }

  //Setup SSDP
  DEBUG_OUTPUT.println("  Setting up SSDP...");
  SSDP.setHTTPPort(SERVER_PORT);
  SSDP.setName("RML Alarm Monitor Sensor");
  SSDP.setSchemaURL("description.xml");
  SSDP.setURL("");
  SSDP.setModelName("RML01");
  SSDP.setModelURL("http://dst.net/RML01");
  SSDP.setManufacturer("Ross Lipenta");
  SSDP.setManufacturerURL("http://dst.net/");
  SSDP.setSerialNumber(ESP.getChipId());
  SSDP.setDeviceType("urn:schemas-upnp-org:device:RmlAlarmMonitorSensor:1");
  SSDP.begin();
  uuid = getUUID();

  //Setup the Web Server
  DEBUG_OUTPUT.println("  Starting Web Server...");
  const char * headerKeys[] = { "CALLBACK", "NT", "TIMEOUT", "HOST", "User-Agent" };
  server.collectHeaders(headerKeys, sizeof(headerKeys)/sizeof(char*));
  server.on("/", handleMainRequest);
  server.on("/event", handleEventSubscriptionRequest);
  server.on("/test", handleTestRequest);
  server.on("/description.xml", HTTP_GET, handleSsdpDescriptionRequest);
  server.onNotFound(handleNotFound);
  server.begin();
  DEBUG_OUTPUT.println("    Started on port " + String(SERVER_PORT) + ".");

  //Handle alarm triggered event
  alarm.onAlarmTriggered([]() {
    DEBUG_OUTPUT.println("triggered!");
    isAlarmTriggered = true;
    sendNotificationToSubscriber(isAlarmTriggered);
  });

  //Handle alarm cleared event
  alarm.onAlarmCleared([]() {
    DEBUG_OUTPUT.println("cleared.");
    isAlarmTriggered = false;
    sendNotificationToSubscriber(isAlarmTriggered);
  });
}

/**
 * Handles the main (/) request
 *
 *  Returns a simple JSON response that includes
 *  the current state of the alarm (isAlarmedTriggered).
 *  If this is 0 the alarm is silent. If this is 1 then
 *  the alarm is triggered. The rest of the information
 *  is primarily for diagnosis purposes only and could
 *  be ignored or excluded.
*/
void handleMainRequest() {
  requestCounter++;

  String json = "{";
  json += "\"isAlarmTriggered\":"+String(isAlarmTriggered);
  json += ", \"heap\":"+String(ESP.getFreeHeap());
  json += ", \"analog\":"+String(analogRead(A0));
  json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
  json += ", \"chipId\":"+String(ESP.getChipId());
  json += ", \"subscriberUrl\": \""+subscriberUrl+"\"";
  json += "}";
  server.send(200, "text/json", json);
  json = String();

  DEBUG_OUTPUT.println("Main Request #" + String(requestCounter));
}

/**
 * Handles SSDP Event Subscription requests
 *
 *  This is currently allowing for a single subscriber.
 *  The last or most recent subscriber to subscribe wins.
 */
void handleEventSubscriptionRequest() {
  DEBUG_OUTPUT.println("Subscription Request");
  if (server.hasHeader("CALLBACK")) {
    subscriberUrl = parseSubscriptionUrl(server.header("CALLBACK"));
  }
  server.send(200);
}

/**
 * Handles requests to test/reset the alarm (/test)
 *
 *  This essentially toggles the current alarm state
 *  allowing the user to test whether or not the system is
 *  functional. It also lets them clear a false alarm.
 */
void handleTestRequest() {
  DEBUG_OUTPUT.println("Test Request");
  isAlarmTriggered = !isAlarmTriggered;
  sendNotificationToSubscriber(isAlarmTriggered);
  handleMainRequest();
}

/**
 * Handles requests for SSDP description data
 */
void handleSsdpDescriptionRequest() {
  DEBUG_OUTPUT.println("Description.xml Request");
  SSDP.schema(server.client());
}

/**
 * Handles all other requests and treats them as 404 Not Found
 */
void handleNotFound() {
  DEBUG_OUTPUT.println("404 Not Found: " + server.uri());
  server.send(404, "text/plain", "Not found");
}

/**
 * Sends a NOTIFY request to the SSDP Event Subscriber
 *
 *  This may not fully follow the SSDP specification but was
 *  enough to enable event callbacks to work with SmartThings
 *
 *  @triggered indicate state of the alarm and whether it is
 *    considered triggered (true)
 */
void sendNotificationToSubscriber(const bool triggered) {
  if (subscriberUrl && subscriberUrl.length() > 0) {
    HTTPClient http;
    http.begin(subscriberUrl);
    http.addHeader("HOST", parseHostFromSubscriptionUrl(subscriberUrl));
    http.addHeader("NT", "upnp:event");
    http.addHeader("NTS", "upnp:propchange");
    http.addHeader("SID", uuid);
    http.addHeader("Content-Type", "application/json");
    int result = http.sendRequest("NOTIFY", "{ \"isAlarmTriggered\":" + String(triggered) + " }");
    DEBUG_OUTPUT.println("--> Sent Notification to \"" + subscriberUrl + "\". Result: " + String(result) + " <--");
  }
}

/**
 * Generates the UUID for the device based on the technique
 * the SSDP library generates it (via setSerialNumber)
 *
 *  This is needed for the SSDP Event notification presumably because
 *  it's how the subscriber will match up events that arrive at a shared
 *  notification endpoint to the appropriate handler. Unfortunately the
 *  SSDP library does not expose access to this, so this code is borrowed
 *  from the library itself.
 */
String getUUID() {
  char _uuid[37];
  uint32_t chipId = ESP.getChipId();
  sprintf(_uuid, "38323636-4558-4dda-9188-cda0e6%02x%02x%02x",
    (uint16_t) ((chipId >> 16) & 0xff),
    (uint16_t) ((chipId >>  8) & 0xff),
    (uint16_t)   chipId        & 0xff  );
  return _uuid;
}

/**
 * Parses a URL sent via an SSDP Subscribe request
 *
 *  The URL is sent in surrounded with < and >
 *  This simply returns the contents in between
 *
 *  @url the subscription URL
 *  @return the actual URL from the subscription request
 */
String parseSubscriptionUrl(String url) {
  if (url && url.length() > 0) {
    return url.substring(1, url.length() - 1);
  } else {
    return url;
  }
}

/**
 * Parses out the HOST/PORT portion of the URL
 *
 * @url the subscription URL
 * @return just the HOST and PORT (if included) from the URL
 */
String parseHostFromSubscriptionUrl(String url) {
  if (url && url.length() > 0) {
    String result = url.substring(7);
    result = result.substring(0, result.indexOf("/"));
    return result;
  } else {
    return url;
  }
}

/**
 * Connects to the WiFi network
 */
void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  delay(500);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_OUTPUT.print(".");
  }

  DEBUG_OUTPUT.println();
  DEBUG_OUTPUT.println("    WiFi connected");
  DEBUG_OUTPUT.print("    IP address: ");
  DEBUG_OUTPUT.println(WiFi.localIP());
}

/**
 * The main event loop
 *
 *  Simply handle the alarm and handle the Web Server
 */
void loop() {
  alarm.handleAlarm();
  server.handleClient();
}
