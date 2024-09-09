#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

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

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // List files in SPIFFS for debugging
  listFiles();
  
  File chartjs_file = SPIFFS.open("/chart.js", "r");
  if (!chartjs_file) {
    Serial.println("Failed to open chart.js file");
  } else {
    Serial.println("Successfully opened chart.js file");
    chartjs_file.close();
  }

  // Initialize Wi-Fi in AP mode
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("Wi-Fi started in AP mode");

  // Define server routes
  server.on("/", HTTP_GET, handle_OnConnect);     // Serve the main page
  server.on("/start", HTTP_GET, handle_Start);    // Start reading (dummy)
  server.on("/stop", HTTP_GET, handle_Stop);      // Stop reading (dummy)
  server.on("/data", HTTP_GET, handle_Data);      // Serve sensor data
  server.on("/chart.js", HTTP_GET, handle_ChartJS); // Serve Chart.js file
   // Route to handle file uploads
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "File Uploaded");
  }, handle_Upload);
  server.on("/files", HTTP_GET, handle_FileDownload);
  server.onNotFound(handle_NotFound);             // Handle unknown requests

  // Handle admin page
  server.on("/admin", HTTP_GET, handle_Admin);

  // Handle file deletion
  server.on("/delete", HTTP_GET, handle_Delete);

  // Handle file upload
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
          Serial.printf("UploadStart: %s\n", filename.c_str());
          // Open file for writing
          File file = SPIFFS.open("/" + filename, "w");
          if (!file) {
              Serial.println("Failed to open file for writing");
              return;
          }
          file.close();
      }
      File file = SPIFFS.open("/" + filename, "a");
      if (file) {
          file.write(data, len);
          file.close();
      }
      if (final) {
          Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index + len);
      }
    });


  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(200, "text/html", 
  //   "<form method='POST' action='/upload' enctype='multipart/form-data'>"
  //   "<input type='file' name='file' accept='.js,.html,.css'>"
  //   "<input type='submit' value='Upload'>"
  //   "</form>");
  // });

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

// Main loop - nothing to do for this example
void loop() {
  // AsyncWebServer handles requests asynchronously
}

void listFiles(){
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print("Found file: ");
    Serial.print(file.name());
    Serial.print(", size: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}
// Handle the main page request
void handle_OnConnect(AsyncWebServerRequest *request) {
  String html = F(
    "<!DOCTYPE html><html><head><title>Monitoramento de Saúde em Tempo Real</title>"
    "<script src=\"/chart.js\"></script>"
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

// Handle the Chart.js file request
void handle_ChartJS(AsyncWebServerRequest *request) {
  File chartfile = SPIFFS.open("/chart.js.gz", "r");
  if (!chartfile) {
    Serial.println("Failed to open chart.js.gz file");
    request->send(404, "text/plain", "File not found");
    return;
  }
  Serial.println("Serving compressed chart.js file");

  // Debug: Read and log file content
  // String content = file.readString();
  // Serial.println("File content:");
  // Serial.println(content);

  // request->send(chartfile, "application/javascript");
  // chartfile.close();

  // send compressed in chunks
  AsyncWebServerResponse *response = request->beginResponse(chartfile, "application/javascript");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
  chartfile.close();
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

bool checkAvailableSpace(size_t fileSize) {
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;

  Serial.printf("Total space: %u bytes, Used space: %u bytes, Free space: %u bytes\n", totalBytes, usedBytes, freeBytes);
  return freeBytes >= fileSize;
}

// File upload handler
void handle_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File file;  // Keep file object in static to retain its state between calls

  if (index == 0) { // New upload, open file
    // Check if there's enough space before starting the upload
    if (!checkAvailableSpace(len)) {
      Serial.println("Not enough space for the file.");
      request->send(500, "text/plain", "Not enough space to upload the file.");
      return;
    }

    // Open the file for writing
    Serial.printf("UploadStart: %s\n", filename.c_str());
    if (SPIFFS.exists("/" + filename)) {
      SPIFFS.remove("/" + filename);  // Overwrite existing file
    }
    file = SPIFFS.open("/" + filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      request->send(500, "text/plain", "Failed to open file for writing");
      return;
    }
  }

  // Write the received data
  if (file) {
    if (file.write(data, len) != len) {
      Serial.println("Failed to write file");
      request->send(500, "text/plain", "Failed to write file");
      return;
    } else {
      Serial.printf("Wrote %u bytes\n", len);
    }
  } else {
    Serial.println("File not open");
    request->send(500, "text/plain", "File not open");
    return;
  }

  // Finalize the file write
  if (final) {
    file.close();
    Serial.printf("UploadEnd: %s (%u bytes)\n", filename.c_str(), index + len);
    request->send(200, "text/plain", "File uploaded successfully");
  }
}


// void handle_Upload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
//   if (!index) {
//     // Check if there's enough space before starting the upload
//     if (!checkAvailableSpace(len)) {
//       Serial.println("Not enough space for the file.");
//       request->send(500, "text/plain", "Not enough space to upload the file.");
//       return;
//     }

//     // Open the file for writing
//     Serial.printf("UploadStart: %s\n", filename.c_str());
//     if (!SPIFFS.exists("/" + filename)) {
//       SPIFFS.remove("/" + filename);  // Overwrite existing file
//     }
//     File file = SPIFFS.open("/" + filename, FILE_WRITE);
//     if (!file) {
//       Serial.println("Failed to open file for writing");
//       return;
//     }
//   }
//   // Write the received data
//   File file = SPIFFS.open("/" + filename, FILE_APPEND);
//   if (file) {
//     if (file.write(data, len) != len) {
//       Serial.println("Failed to write file");
//     } else {
//       Serial.printf("Wrote %u bytes\n", len);
//     }
//     file.close();
//   } else {
//     Serial.println("Failed to open file for appending");
//   }

//   // Finalize the file write
//   if (final) {
//     Serial.printf("UploadEnd: %s (%u bytes)\n", filename.c_str(), index + len);
//   }
// }

void handle_Admin(AsyncWebServerRequest *request) {
    String html = F(
        "<!DOCTYPE html><html><head><title>Admin Panel</title></head><body>"
        "<h1>Admin Panel - File Management</h1>"
        "<h2>Upload a New File</h2>"
        "<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
        "<input type=\"file\" name=\"file\">"
        "<input type=\"submit\" value=\"Upload\">"
        "</form>"
        "<h2>Available Files</h2>"
        "<ul>"
    );

    // List files in SPIFFS
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        html += "<li>" + fileName;

        // Provide link to view the file
        html += " <a href=\"/files?file=" + String(fileName) + "\" target=\"_blank\">View</a>";

        // Provide link to delete the file
        html += " <a href=\"/delete?file=" + String(fileName) + "\" onclick=\"return confirm('Are you sure you want to delete this file?');\">Delete</a>";

        html += "</li>";
        file = root.openNextFile();
    }
    html += "</ul>";
    html += "</body></html>";

    request->send(200, "text/html", html);
}

void handle_FileDownload(AsyncWebServerRequest *request) {
  String fileName = request->getParam("file")->value();
  if (SPIFFS.exists("/" + fileName)) {
    File file = SPIFFS.open("/" + fileName, "r");
    if (file) {
      request->send(file, "text/plain");
      file.close();
    } else {
      request->send(500, "text/plain", "Failed to open file");
    }
  } else {
    request->send(404, "text/plain", "File not found");
  }
}

void handle_Delete(AsyncWebServerRequest *request) {
    Serial.println("Delete handler called");
    if (request->hasParam("file")) {
        String fileName = request->getParam("file")->value();
        fileName.replace("%20", " ");  // Decode spaces if necessary
        String filePath = "/" + fileName;  // Ensure leading "/"
        Serial.println("File name: " + fileName);
        Serial.println("Filepath: " + filePath);

        Serial.println("Attempting to delete file: " + fileName);

        // // Prevent deletion of critical files
        // if (fileName == "/index.html" || fileName == "/chart.js") {
        //     request->send(403, "text/plain", "Deletion of critical files is not allowed");
        //     return;
        // }

        if (SPIFFS.exists(filePath)) {
            Serial.println("File exists: " + filePath);
            if (SPIFFS.remove(filePath)) {
                Serial.println("File successfully deleted: " + filePath);
                request->send(200, "text/plain", "File deleted: " + filePath);
            } else {
                Serial.println("Failed to delete file: " + filePath);
                request->send(500, "text/plain", "Failed to delete file");
            }
        } else {
            Serial.println("File not found: " + filePath);
            request->send(404, "text/plain", "File not found");
        }
    } else {
        request->send(400, "text/plain", "File parameter missing");
    }
    listFiles();
}

void handle_SimpleFile(AsyncWebServerRequest *request) {
  File file = SPIFFS.open("/test.txt", "r");
  if (!file) {
    Serial.println("Failed to open test.txt file");
    request->send(404, "text/plain", "File not found");
    return;
  }
  Serial.println("Serving test.txt file");

  request->send(file, "text/plain");
  file.close();
}

// Handle 404 errors
void handle_NotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not Found");
}
