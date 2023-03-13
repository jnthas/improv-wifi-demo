
#include <WiFi.h>
#include <WebServer.h>

// SSID & Password
const char* ssid = "HUAWEI-5R42NS_HiLink";  // Enter your SSID here
const char* password = "***";  //Enter your Password here

WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)
String packet;
int state = 0;
const int LED_BUILTIN = 2;


// HTML & CSS contents which display on web server
String HTML = "<!DOCTYPE html>\
<html>\
<body>\
<h1>Hello, world!</h1>\
</body>\
</html>";

void setup() {
  Serial.begin(115200);
  Serial.println("Try Connecting to ");
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println(ssid);



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
    packet = Serial.readString();
    packet.trim();
    Serial.println("Packet: " + packet);
    blinkled(1000);
  }

  if (packet != "" && state == 0) {
      std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
      data.resize(11);
      data[6] = 1; //IMPROV_SERIAL_VERSION;
      data[7] = 0x01; //TYPE_CURRENT_STATE;
      data[8] = 1;
      data[9] = 0x01;  //DATA

      uint8_t checksum = 0x00;
      for (uint8_t d : data)
        checksum += d;
      data[10] = checksum;

      Serial.write(data.data(), data.size());
      blinkled(100);
      blinkled(100);

     packet = "";
     state = 1;
  }

    //request device info
    if (packet != "" && state == 1) {
      std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
      data.resize(12);
      data[6] = 1; //IMPROV_SERIAL_VERSION;
      data[7] = 0x04; //TYPE_RPC;
      data[8] = 2;   //LENGTH
      data[9] = 0x03;  //DATA
      data[10] = 0;  //DATA

      //TODO Add device info

      uint8_t checksum = 0x00;
      for (uint8_t d : data)
        checksum += d;
      data[11] = checksum;

      Serial.write(data.data(), data.size());
      blinkled(100);
      blinkled(100);

     packet = "";
     state = 2;
  }

  //request scanned networks
  if (packet != "" && state == 2) {
      std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
      data.resize(12);
      data[6] = 1; //IMPROV_SERIAL_VERSION;
      data[7] = 0x04; //TYPE_RPC;
      data[8] = 2;   //LENGTH
      data[9] = 0x04;  //COMMAND
      data[10] = 0;  //DATA

      //TODO scan networks

      uint8_t checksum = 0x00;
      for (uint8_t d : data)
        checksum += d;
      data[11] = checksum;

      Serial.write(data.data(), data.size());
      blinkled(100);
      blinkled(100);

     packet = "";
     state = 3;
  }

}

// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}

void blinkled(int d) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(d);
  digitalWrite(LED_BUILTIN, LOW);
  delay(d);
}
