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
  server.on("/data", HTTP_GET, handle_Data);      // Serve sensor data
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
    "<!DOCTYPE html><html><head><title>Monitoramento de Saúde em Tempo Real</title>"
    "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"
    "<style>"
    "body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 0; }"
    "header { background-color: #007bff; color: #fff; padding: 20px; text-align: center; display: flex; align-items: center; justify-content: center; }"
    "main { padding: 20px; max-width: 1200px; margin: 0 auto; }"
    ".card { background-color: #fff; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); border-radius: 8px; padding: 20px; margin-bottom: 20px; }"
    ".card h2 { margin: 0; font-size: 1.5em; color: #333; }"
    ".value { font-size: 2em; color: #007bff; }"
    ".chart-container { position: relative; width: 100%; height: 50vh; max-width: 1000px; margin: 0 auto; }"
    "footer { text-align: center; padding: 10px; background-color: #007bff; color: #fff; position: fixed; width: 100%; bottom: 0; }"
    "canvas { width: 100% !important; height: auto !important; }"
    ".switch-container { display: flex; justify-content: center; margin-bottom: 20px; gap: 20px; }"
    ".switch { position: relative; display: inline-block; width: 100px; height: 34px; }"
    ".switch input { opacity: 0; width: 0; height: 0; }"
    ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }"
    ".slider:before { position: absolute; content: \"\"; height: 26px; width: 26px; border-radius: 50%; left: 4px; bottom: 4px; background-color: white; transition: .4s; }"
    "input:checked + .slider { background-color: #007bff; }"
    "input:checked + .slider:before { transform: translateX(66px); }"
    ".switch-label { margin: 0; display: flex; align-items: center; justify-content: center; cursor: pointer; }"
    ".switch-label span { margin-left: 10px; font-size: 1.2em; color: #333; }"
    "</style></head><body>"
    "<header><h1>Monitoramento de Saúde em Tempo Real</h1></header>"
    "<main>"
    "<div class=\"switch-container\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"oximeter-switch\"><span class=\"slider\"></span></label>"
    "<label class=\"switch-label\" for=\"oximeter-switch\"><span>Saturação de Oxigênio</span></label>"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"heart-rate-switch\"><span class=\"slider\"></span></label>"
    "<label class=\"switch-label\" for=\"heart-rate-switch\"><span>Frequência Cardíaca</span></label>"
    "</div>"
    "<div class=\"card\"><h2>Saturação de Oxigênio</h2><div class=\"value\" id=\"oximeter-value\">--</div><p>%</p></div>"
    "<div class=\"card\"><h2>Frequência Cardíaca</h2><div class=\"value\" id=\"heart-rate-value\">--</div><p>bpm</p></div>"
    "<div class=\"card\"><h2>Gráfico de Frequência Cardíaca</h2><div class=\"chart-container\"><canvas id=\"heart-rate-chart\"></canvas></div></div>"
    "</main><footer><p>Projeto de Monitoramento de Saúde - UFABC</p></footer>"
    "<script>"
    "const ctx = document.getElementById('heart-rate-chart').getContext('2d');"
    "const maxDataPoints = 20;"
    "const heartRateChart = new Chart(ctx, {"
    "  type: 'line',"
    "  data: {"
    "    labels: [],"
    "    datasets: [{"
    "      label: 'Frequência Cardíaca',"
    "      data: [],"
    "      borderColor: 'rgb(75, 192, 192)',"
    "      backgroundColor: 'rgba(75, 192, 192, 0.2)',"
    "      fill: true,"
    "      tension: 0.1"
    "    }]"
    "  },"
    "  options: {"
    "    responsive: true,"
    "    maintainAspectRatio: false,"
    "    animation: { duration: 0 },"
    "    scales: {"
    "      x: { display: true, title: { display: true, text: 'Tempo' } },"
    "      y: { display: true, title: { display: true, text: 'BPM' } }"
    "    }"
    "  }"
    "});"
    "let oximeterInterval = null;"
    "let heartRateInterval = null;"
    "function startDataCollection() {"
    "  if (document.getElementById('oximeter-switch').checked) {"
    "    if (!oximeterInterval) {"
    "      oximeterInterval = setInterval(() => {"
    "        fetch('/data').then(response => response.json()).then(data => {"
    "          document.getElementById('oximeter-value').innerText = data.oximeter.toFixed(2);"
    "        }).catch(error => console.error('Error fetching data:', error));"
    "      }, 700);"
    "    }"
    "  } else {"
    "    if (oximeterInterval) {"
    "      clearInterval(oximeterInterval);"
    "      oximeterInterval = null;"
    "    }"
    "    document.getElementById('oximeter-value').innerText = '--';"
    "  }"
    "  if (document.getElementById('heart-rate-switch').checked) {"
    "    if (!heartRateInterval) {"
    "      heartRateInterval = setInterval(() => {"
    "        fetch('/data').then(response => response.json()).then(data => {"
    "          document.getElementById('heart-rate-value').innerText = data.ecg.toFixed(2);"
    "          const now = new Date().toLocaleTimeString();"
    "          if (heartRateChart.data.labels.length >= maxDataPoints) {"
    "            heartRateChart.data.labels.shift();"
    "            heartRateChart.data.datasets[0].data.shift();"
    "          }"
    "          heartRateChart.data.labels.push(now);"
    "          heartRateChart.data.datasets[0].data.push(data.ecg);"
    "          heartRateChart.update('none');"
    "        }).catch(error => console.error('Error fetching data:', error));"
    "      }, 700);"
    "    }"
    "  } else {"
    "    if (heartRateInterval) {"
    "      clearInterval(heartRateInterval);"
    "      heartRateInterval = null;"
    "    }"
    "    document.getElementById('heart-rate-value').innerText = '--';"
    "  }"
    "}"
    "document.getElementById('oximeter-switch').addEventListener('change', startDataCollection);"
    "document.getElementById('heart-rate-switch').addEventListener('change', startDataCollection);"
    "</script></body></html>"
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

// Handle sensor data request
void handle_Data(AsyncWebServerRequest *request) {
  // Dummy data values for example
  float oximeterValue = 98.0;  // Example oximeter value
  float ecgValue = 75.0;       // Example ECG value

  String json = "{";
  json += "\"oximeter\":" + String(oximeterValue) + ",";
  json += "\"ecg\":" + String(ecgValue);
  json += "}";

  request->send(200, "application/json", json);
}

// Handle 404 errors
void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not Found");
}
