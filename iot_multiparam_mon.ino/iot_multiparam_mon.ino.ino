#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Server and network settings
AsyncWebServer server(80);
const char* ssid = "ESP32";  
const char* password = "12345678";
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Control variable for sensor reading (dummy for now)
bool isReading = false;

// Initialize the ESP32 Wi-Fi and Web Server
void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi in AP mode
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("Wi-Fi started in AP mode");

  // Define server routes
  server.on("/", HTTP_GET, handle_OnConnect);     // Serve the main page
  server.on("/start", HTTP_GET, handle_Start);    // Start reading (dummy)
  server.on("/stop", HTTP_GET, handle_Stop);      // Stop reading (dummy)
  server.onNotFound(handle_NotFound);             // Handle unknown requests

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

// Main loop - nothing to do for this example
void loop() {
  // AsyncWebServer handles requests asynchronously
}

// Handle the main page request
void handle_OnConnect(AsyncWebServerRequest *request) {
  String html = F(
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Monitoramento de Saúde</title>"
    "<style>body{font-family:Arial,sans-serif;background-color:#f4f4f4;margin:0;padding:0}"
    "header{background-color:#007bff;color:#fff;padding:20px;text-align:center;}"
    "main{padding:20px;max-width:1200px;margin:auto}.card{background-color:#fff;"
    "box-shadow:0 4px 8px rgba(0,0,0,0.2);border-radius:8px;padding:20px;margin-bottom:20px}"
    "footer{text-align:center;padding:10px;background-color:#007bff;color:#fff;position:fixed;width:100%;bottom:0}"
    ".button-container{display:flex;justify-content:center;gap:20px;margin-bottom:20px}"
    ".button{padding:10px 20px;border:none;border-radius:5px;color:#fff;cursor:pointer}"
    ".start{background-color:#28a745} .stop{background-color:#dc3545}</style></head>"
    "<body><header><h1>Monitoramento de Saúde em Tempo Real</h1></header>"
    "<main>"
    "<div class=\"button-container\">"
    "<button class=\"button start\" onclick=\"startReading()\">Iniciar Leitura</button>"
    "<button class=\"button stop\" onclick=\"stopReading()\">Parar Leitura</button>"
    "</div>"
    "<div class=\"card\"><h2>Saturação de Oxigênio</h2><div id=\"oximeter-value\">--</div></div>"
    "<div class=\"card\"><h2>Leitura do ECG</h2><div id=\"ecg-value\">--</div></div></main>"
    "<footer><p>Projeto de Monitoramento de Saúde - UFABC</p></footer>"
    "<script>"
    "function startReading() {"
    "  fetch('/start').then(response => response.text()).then(data => {"
    "    console.log('Sensor reading started');"
    "  }).catch(error => console.error('Error:', error));"
    "}"
    "function stopReading() {"
    "  fetch('/stop').then(response => response.text()).then(data => {"
    "    console.log('Sensor reading stopped');"
    "  }).catch(error => console.error('Error:', error));"
    "}"
    "</script>"
    "</body></html>"
  );
  request->send(200, "text/html; charset=utf-8", html);
}

// Dummy start sensor reading handler
void handle_Start(AsyncWebServerRequest *request) {
  isReading = true;  // This is just a placeholder
  request->send(200, "text/plain", "Sensor reading started");
}

// Dummy stop sensor reading handler
void handle_Stop(AsyncWebServerRequest *request) {
  isReading = false;  // This is just a placeholder
  request->send(200, "text/plain", "Sensor reading stopped");
}

// Handle 404 errors
void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not Found");
}
