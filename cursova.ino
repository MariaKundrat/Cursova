#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <SPIFFS.h>

const char* ssid = "ivan";
const char* password = "........";

#define PIN 4

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(16, 16, PIN, 
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

WebServer server(80);

unsigned long previousMillis = 0;
const long interval = 5000;

DynamicJsonDocument currentMatrix(8192);

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  matrix.begin();
  matrix.setBrightness(40);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/setMatrix", HTTP_POST, handleSetMatrix);
  server.on("/getPreviousMatrix", HTTP_GET, handleGetPreviousMatrix);

  server.begin();
}

void loop() {
  server.handleClient();
  printIPAddress();
}

void handleRoot() {
  File file = SPIFFS.open("/index3.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleSetMatrix() {
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  String body = server.arg("plain");
  DeserializationError error = deserializeJson(currentMatrix, body);

  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  saveMatrixToFile("/previousMatrix.json", currentMatrix);

  matrix.fillScreen(0);
  for (int i = 0; i < 256; i++) {
    String color = currentMatrix[i];
    int x = i % 16;
    int y = i / 16;
    if (color == "red") {
      matrix.drawPixel(x, y, matrix.Color(255, 0, 0));
    } else if (color == "green") {
      matrix.drawPixel(x, y, matrix.Color(0, 255, 0));
    } else if (color == "blue") {
      matrix.drawPixel(x, y, matrix.Color(0, 0, 255));
    } else if (color == "yellow") {
      matrix.drawPixel(x, y, matrix.Color(255, 255, 0));
    } else if (color == "black") {
      matrix.drawPixel(x, y, matrix.Color(0, 0, 0));
    }
  }
  matrix.show();

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleGetPreviousMatrix() {
  DynamicJsonDocument previousMatrix(8192);
  if (loadMatrixFromFile("/previousMatrix.json", previousMatrix)) {
    String response;
    serializeJson(previousMatrix, response);
    server.send(200, "application/json", response);
  } else {
    server.send(404, "application/json", "{\"status\":\"not found\"}");
  }
}

void printIPAddress() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void saveMatrixToFile(const char* path, const JsonDocument& doc) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  serializeJson(doc, file);
  file.close();
}

bool loadMatrixFromFile(const char* path, JsonDocument& doc) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  return !error;
}
