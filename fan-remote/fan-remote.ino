#include <WiFi.h>

#define bit_us 400        // microseconds
#define long_pause_ms 14  // milliseconds
#define TIMES_TO_REPEAT 8

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


const char *ssid = "Jupiter";
const char *password = "JupWireless";

struct Button_map {
  char *key;
  char *bits;
};

struct Button_map button_map[] = {
  { "fan-high", "0000001110011111100010110" },
  { "fan-med", "0000001110011111100010100" },
  { "fan-low", "0000001110011111100010010" },
  { "fan-off", "0000001110011111100010000" },
  { "fan-light1-on", "0000001110011111100011010" },
  { "fan-light1-off", "0000001110011111100011100" },
  { "fan-light2", "0000001110011111100100100" },
  { "power-on", "0000001110011111100000010" },
  { "power-off", "0000001110011111100000100" },
};
void send_amp_shift_keys(char *bits, int repeats);

int TX_PIN = 5;  // pin 5 for the transmitter
void setup() {
  pinMode(TX_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}
void loop() {
  WiFiClient client = server.available();

  if (client) {  // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");                                             // print a message out in the serial port
    String currentLine = "";                                                   // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            if (!buttonPressed(client, header)) {
              mainPage(client);
            }          
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
      // delay(300);
      // send_amp_shift_keys(button_map[3].bits, TIMES_TO_REPEAT);
      // while (1) {
      //   esp_deep_sleep_start();
      //   delay(1000);
      // }
    }
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
void send_amp_shift_keys(char *bits, int repeats) {
  // bits is a string of 1s and 0s
  for (int repeat = 0; repeat < repeats; repeat++) {
    // send signal
    int num_bits = strlen(bits);
    for (int i = 0; i < num_bits; i++) {
      // always send 1 0 as first two bits
      digitalWrite(TX_PIN, HIGH);
      delayMicroseconds(bit_us);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(bit_us);

      if (!(bits[i] - '0')) {  // zero
        delayMicroseconds(bit_us);
      } else {
        digitalWrite(TX_PIN, HIGH);
        delayMicroseconds(bit_us);
        digitalWrite(TX_PIN, LOW);
      }
    }
    delay(long_pause_ms);
  }
}

void mainPage(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the buttons
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 8px 20px;");
  client.println("text-decoration: none; font-size: 16px; margin: 2px; cursor: pointer;}");
  client.println("</style></head>");

  // Web Page Heading
  client.println("<body><h1>Ceiling Fan</h1>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-high')\">Fan High</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-med')\">Fan Medium</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-low')\">Fan Low</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-off')\">Fan Off</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-light1-on')\">Light 1 On</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-light1-off')\">Light 1 Off</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('fan-light2')\">Light 2</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('power-on')\">Main On</button></p>");
  client.println("<p><button class=\"button\" onclick=\"pushButton('power-off')\">Main Off</button></p>");
  client.println("<script>");
  client.println("const pushButton = (buttonPressed) => {");
  client.println("  fetch('/' + buttonPressed)");
  client.println("  .then((response) => response.text())");
  client.println("  .then((data) => console.log(data));");
  client.println("}");
  client.println("</script>");
  
  client.println("</body></html>");
  client.println();
}

bool buttonPressed(WiFiClient client, String header) {
  for (int button = 0; button < sizeof(button_map)/sizeof(button_map[0]); button++) {
    char *key = button_map[button].key;
    if (header.indexOf(key) >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/plain");
      client.println("Connection: close");
      client.println();
      client.println(key);
      send_amp_shift_keys(button_map[button].bits, TIMES_TO_REPEAT);
      return true;
    }
  }
  return false;
}
