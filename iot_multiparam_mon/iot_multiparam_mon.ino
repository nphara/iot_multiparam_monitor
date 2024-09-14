#include <FS.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>


Preferences preferences;

// Server and network settings
AsyncWebServer server(80);
const char* ap_ssid = "Multiparametric Monitor";
const char* ap_password = "12345678";
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Control variables
String router_ssid = "";
String router_password = "";

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

 // Initialize Preferences
  preferences.begin("wifi-config", false);

  // Initialize Wi-Fi in AP mode
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("Wi-Fi started in AP mode");

  // Try connecting to router
  connectToRouter();

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

  server.on("/set-credentials", HTTP_POST, [](AsyncWebServerRequest *request) {
    String ssid = request->arg("ssid");
    String password = request->arg("password");

    // Save credentials and reboot
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    request->send(200, "text/plain", "Credentials saved! Rebooting...");
    delay(2000);
    ESP.restart();
  });


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
  // Keep checking Wi-Fi status (STA mode)
  if (WiFi.status() != WL_CONNECTED) {
    connectToRouter();
  }
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
  File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      request->send(404, "text/plain", "File not found");
      return;
    }
    String html = file.readString();
    file.close();
    request->send(200, "text/html; charset=UTF-8", html);
}

// Handle the Chart.js file request
void handle_ChartJS(AsyncWebServerRequest *request) {
  File chartfile = SPIFFS.open("/chart.js.gz", "r");
  if (!chartfile) {
    Serial.println("Failed to open chart.js.gz file");
    request->send(404, "text/plain; charset=UTF-8", "File not found");
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
    );

    // Display ESP32 IPs and MAC addresses
    html += F("<h2>ESP32 Network Information</h2>");

    // Access Point IP and MAC
    html += "AP IP Address: " + WiFi.softAPIP().toString() + "<br>";
    html += "AP MAC Address: " + WiFi.softAPmacAddress() + "<br>";

    // Station IP and MAC (if connected to a Wi-Fi network)
    if (WiFi.status() == WL_CONNECTED) {
        html += "STA IP Address: " + WiFi.localIP().toString() + "<br>";
        html += "STA MAC Address: " + WiFi.macAddress() + "<br>";
    } else {
        html += "STA IP Address: Not connected<br>";
        html += "STA MAC Address: " + WiFi.macAddress() + "<br>";
    }

    // Adding Wi-Fi configuration section
    html += F(
        "<h2>Wi-Fi Configuration</h2>"
        "<form method=\"POST\" action=\"/set-credentials\">"
        "<label for=\"ssid\">SSID:</label>"
        "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>"
        "<label for=\"password\">Password:</label>"
        "<input type=\"password\" id=\"password\" name=\"password\"><br><br>"
        "<input type=\"submit\" value=\"Save\">"
        "</form>"
    );

    // Adding connected clients section
    html += F("<h2>Connected Devices</h2>");
    
    int numStations = WiFi.softAPgetStationNum();  // Get the number of connected stations
    html += "Number of connected devices: " + String(numStations) + "<br>";

    if (numStations > 0) {
        wifi_sta_list_t stationList;
        tcpip_adapter_sta_list_t adapterList;

        // Get station info (from the ESP-IDF, but usable in Arduino)
        esp_wifi_ap_get_sta_list(&stationList);
        tcpip_adapter_get_sta_list(&stationList, &adapterList);

        html += "<ul>";
        for (int i = 0; i < adapterList.num; i++) {
            tcpip_adapter_sta_info_t station = adapterList.sta[i];
            String mac = String(station.mac[0], HEX) + ":" +
                         String(station.mac[1], HEX) + ":" +
                         String(station.mac[2], HEX) + ":" +
                         String(station.mac[3], HEX) + ":" +
                         String(station.mac[4], HEX) + ":" +
                         String(station.mac[5], HEX);

            html += "<li>Device " + String(i + 1) + ": " + mac + "</li>";
        }
        html += "</ul>";
    }

    // List files in SPIFFS
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    html += "<h2>Available Files</h2><ul>";
    
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
    html += F("<h2>Upload a New File</h2>"
        "<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
        "<input type=\"file\" name=\"file\">"
        "<input type=\"submit\" value=\"Upload\">"
        "</form>"
    );
    html += "</body></html>";

    request->send(200, "text/html", html);
}

// Function to get the correct content type based on file extension
String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  else if (filename.endsWith(".zip")) return "application/zip";
  return "text/plain";
}

// Secure file validation to prevent directory traversal
bool isValidFileName(String fileName) {
  if (fileName.indexOf("..") != -1 || fileName.indexOf("/") != -1) {
    return false;  // Invalid file request
  }
  return true;
}

// Function to handle file download
void handle_FileDownload(AsyncWebServerRequest *request) {
  // Retrieve file name from the request
  String fileName = request->getParam("file")->value();

  // Validate file name to prevent directory traversal
  if (!isValidFileName(fileName)) {
    request->send(400, "text/plain", "Invalid file request");
    return;
  }

  // Check if the file exists in SPIFFS
  String filePath = "/" + fileName;
  if (SPIFFS.exists(filePath)) {
    // Get content type based on the file extension
    String contentType = getContentType(fileName);

    // Send the file using the SPIFFS send method
    request->send(SPIFFS, filePath, contentType);
  } else {
    // File not found
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

void connectToRouter() {
  router_ssid = preferences.getString("ssid", "");
  router_password = preferences.getString("password", "");

  if (router_ssid != "") {
    WiFi.begin(router_ssid.c_str(), router_password.c_str());
    Serial.print("Connecting to ");
    Serial.println(router_ssid);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to router!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect to router.");
    }
  }
}