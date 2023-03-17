#include <WiFi.h>
#include "improv.h"

#define ARDUINO 1
#define LED_BUILTIN 2

//*** Web Server
WiFiServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;


//*** Improv
#define MAX_ATTEMPTS_WIFI_CONNECTION 20

uint8_t x_buffer[16];
uint8_t x_position = 0;


void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  blink_led(100, 5); 
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    handle_request();
  }

  if (Serial.available() > 0) {
    uint8_t b = Serial.read();

    if (parse_improv_serial_byte(x_position, b, x_buffer, onCommandCallback, onErrorCallback)) {
      x_buffer[x_position++] = b;      
    } else {
      x_position = 0;
    }
  }
}


// *** Web Server
void handle_request() {

  WiFiClient client = server.available();
  if (client) 
  {
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;

        if (c == '\n' && currentLineIsBlank) 
        {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML><html><body>");
          client.println("<h1>Welcome to SimpleWebServer</h1>");
          client.println("<p>There is nothing here!</p>");
          client.println("</body></html>");
          break;
        }
      }
    }
    delay(1);
    client.stop();
  }  
}

void blink_led(int d, int times) {
  for (int j=0; j<times; j++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(d);
    digitalWrite(LED_BUILTIN, LOW);
    delay(d);
  }
  
}

bool connectWifi(std::string ssid, std::string password) {
  uint8_t count = 0;

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    blink_led(500, 1);

    if (count > MAX_ATTEMPTS_WIFI_CONNECTION) {
      WiFi.disconnect();
      set_error(improv::Error::ERROR_UNABLE_TO_CONNECT);
      return false;
    }
    count++;
  }

  return true;
}


// *** Improv

void onErrorCallback(improv::Error err) {
  blink_led(2000, 3);
}

bool onCommandCallback(improv::ImprovCommand cmd) {

  switch (cmd.command) {
    case improv::Command::GET_CURRENT_STATE:
    {
      if ((WiFi.status() == WL_CONNECTED)) {
        set_state(improv::State::STATE_PROVISIONED);
      } else {
        set_state(improv::State::STATE_AUTHORIZED);
      }
      
      break;
    }

    case improv::Command::WIFI_SETTINGS:
    {
      if (cmd.ssid.length() == 0) {
        set_error(improv::Error::ERROR_INVALID_RPC);
        break;
      }
      
      set_state(improv::STATE_PROVISIONING);
      
      if (connectWifi(cmd.ssid, cmd.password)) {

        blink_led(100, 3);

        set_state(improv::STATE_PROVISIONED);
        std::vector<std::string> url = {String("http://" + WiFi.localIP().toString()).c_str()};
        std::vector<uint8_t> data = improv::build_rpc_response(improv::WIFI_SETTINGS, url, false);
        send_response(data);        
        server.begin();

      }
      
      break;
    }

    case improv::Command::GET_DEVICE_INFO:
    {
      std::vector<std::string> infos = {"ImprovWifiDemo", "1.0.0", "ESP32", "SimpleWebServer"};
      std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
      send_response(data);
      break;
    }

    case improv::Command::GET_WIFI_NETWORKS:
    {
      getAvailableWifiNetworks();
      break;
    }

    default: {
      set_error(improv::ERROR_UNKNOWN_RPC);
      return false;
    }
  }

  return true;
}


void getAvailableWifiNetworks() {
  int networkNum = WiFi.scanNetworks();

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