
#include <WiFi.h>
#include <WebServer.h>
#include "improv.h"


#define ARDUINO 1
#define LED_BUILTIN 2

//*** Web Server
std::string ssid = "";  
std::string password = "";  

WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

// HTML & CSS contents which display on web server
String HTML = "<!DOCTYPE html>\
<html>\
<body>\
<h1>Hello, world!</h1>\
</body>\
</html>";


//*** Improv
uint8_t x_buffer[15]; //TODO: 15 is enough?
uint8_t x_position = 0;


void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  //Blink 5 times means restarted
  blinkled(100, 5);

  // // Connect to your wi-fi modem
  // WiFi.begin(ssid, password);

  // // Check wi-fi is connected to wi-fi network
  // while (WiFi.status() != WL_CONNECTED) {
  // delay(1000);
  // Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi connected successfully");
  // Serial.print("Got IP: ");
  // Serial.println(WiFi.localIP());  //Show ESP32 IP on serial

  // server.on("/", handle_root);

  // server.begin();
  // Serial.println("HTTP server started");
  //delay(100); 
}

void loop() {
  server.handleClient();

  if (Serial.available() > 0) {
    
    uint8_t b = Serial.read();
    bool valid = parse_improv_serial_byte(x_position, b, x_buffer, onCommandCallback, onErrorCallback);

    if (valid) {
      x_buffer[x_position++] = b;
      ////M5.Lcd.println("valid");
    } else {
      x_position = 0;
      ////M5.Lcd.println("invalid");
    }

    
    //M5.Lcd.print((char)b);
  }
}

// *** Web Server
void handle_root() {
  server.send(200, "text/html", HTML);
}

void blinkled(int d, int times) {
  for (int j=0; j<times; j++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(d);
    digitalWrite(LED_BUILTIN, LOW);
    delay(d);
  }
  
}

// *** Improv

void onErrorCallback(improv::Error err) {
  blinkled(1000, 2);
  //M5.Lcd.println("Error");
  //M5.Lcd.println(err);
}

bool onCommandCallback(improv::ImprovCommand cmd) {
  //M5.Lcd.println("Command");
  //M5.Lcd.println(cmd.command);

  switch (cmd.command) {
    case improv::Command::GET_CURRENT_STATE:
    {

      if (ssid.length() > 0 && password.length() > 0) {
        //M5.Lcd.println("CUR_STATE");
        set_state(improv::State::STATE_PROVISIONED);
      } else {
        //M5.Lcd.println("CUR_STATE");
        set_state(improv::State::STATE_AUTHORIZED);
      }
      
      break;
    }

    case improv::Command::WIFI_SETTINGS:
    {
      //M5.Lcd.println("WIFI_SETTINGS");

      blinkled(100, 2);

      //std::vector<std::string> infos = {"https://www.google.com"};
      //std::vector<uint8_t> data1 = improv::build_rpc_response(improv::WIFI_SETTINGS, infos, false);
      //send_response(data1);

      ssid = cmd.ssid;
      password = cmd.password;

      set_state(improv::STATE_PROVISIONING);

      delay(1000);  // Try to connect to wifi here

      // If connection was successful...

      blinkled(100, 3);

      set_state(improv::STATE_PROVISIONED);

      std::vector<std::string> url = {"https://www.google.com"};
      std::vector<uint8_t> data = improv::build_rpc_response(improv::WIFI_SETTINGS, url, false);
      send_response(data);

      break;
    }

    case improv::Command::GET_DEVICE_INFO:
    {
      //M5.Lcd.println("DEV_INFO");
      //ESPHome, 2021.11.0, ESP32-C3, Temperature Monitor.

      std::vector<std::string> infos = {"ImprovWifiDemo", "1.0.0", "ESP32", "SimpleWebServer"};
      std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
      send_response(data);
      break;
    }

    case improv::Command::GET_WIFI_NETWORKS:
    {
      //M5.Lcd.println("WIFI_NET");
      //MyWirelessNetwork, -60, YES.
      getNetworks();
      break;
    }

    default: {
      //M5.Lcd.println("Unknown Improv payload");
      set_error(improv::ERROR_UNKNOWN_RPC);
      return false;
    }
      

  }


  return true;
}


void getNetworks() {
  int networkNum = WiFi.scanNetworks();
  ////M5.Lcd.println("Found " + networkNum);

  for (int id = 0; id < networkNum; ++id) { 
    std::vector<uint8_t> data = improv::build_rpc_response(
            improv::GET_WIFI_NETWORKS, {WiFi.SSID(id), String(WiFi.RSSI(id)), (WiFi.encryptionType(id) == WIFI_AUTH_OPEN ? "NO" : "YES")}, false);
    send_response(data);
    delay(1);
  }
  //final response
  std::vector<uint8_t> data =
          improv::build_rpc_response(improv::GET_WIFI_NETWORKS, std::vector<std::string>{}, false);
  send_response(data);
}

void set_state(improv::State state) {  
  
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(11);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_CURRENT_STATE;
  data[8] = 1;
  data[9] = state;

  uint8_t checksum = 0x00;
  for (uint8_t d : data)
    checksum += d;
  data[10] = checksum;

  Serial.write(data.data(), data.size());
}


void send_response(std::vector<uint8_t> &response) {
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(9);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_RPC_RESPONSE;
  data[8] = response.size();
  data.insert(data.end(), response.begin(), response.end());

  uint8_t checksum = 0x00;
  for (uint8_t d : data)
    checksum += d;
  data.push_back(checksum);

  Serial.write(data.data(), data.size());
}

void set_error(improv::Error error) {
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(11);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_ERROR_STATE;
  data[8] = 1;
  data[9] = error;

  uint8_t checksum = 0x00;
  for (uint8_t d : data)
    checksum += d;
  data[10] = checksum;

  Serial.write(data.data(), data.size());
}






