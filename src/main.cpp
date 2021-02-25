#include <M5EPD.h>
#include <WiFi.h>
#include <WebServer.h>

M5EPD_Canvas canvas(&M5.EPD);
WiFiServer server(80);
String receivedFileName;

enum ConnectionType
{
  UNDEFINED_CONNECTION,
  GET_CONNECTION,
  POST_CONNECTION
};

enum FileType
{
  UNDEFINED_FILE,
  PNG_FILE,
  JPG_FILE
};

void sendResponse(WiFiClient client, String firstLine);
void sendFormHTML(WiFiClient client);
void receiveFormText(WiFiClient client);
void receiveFormFile(WiFiClient client);
void receivePostFile(WiFiClient client, String fileName);

void setup()
{
  // Initialize M5Paper
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(true);

  

  receivedFileName = "";

  canvas.createCanvas(960, 540);
  canvas.setTextSize(3);

  // Load WiFi SSID and PASS from "wifi.txt" in SD card
  String wifiIDString = "wifiID";
  String wifiPWString = "wifiPW";
  File wifiSettingFile = SD.open("/wifi.txt");
  if (wifiSettingFile)
  {
    String line = wifiSettingFile.readStringUntil('\n');
    int location = line.indexOf("SSID:");
    if (location >= 0 && line.length() > 5)
    {
      wifiIDString = line.substring(5);
      wifiIDString.trim();
    }

    line = wifiSettingFile.readStringUntil('\n');
    location = line.indexOf("PASS:");
    if (location >= 0 && line.length() > 5)
    {
      wifiPWString = line.substring(5);
      wifiPWString.trim();
    }

    wifiSettingFile.close();
  }

  // Convert String to char[]
  char wifiID[wifiIDString.length() + 1];
  char wifiPW[wifiPWString.length() + 1];
  wifiIDString.toCharArray(wifiID, wifiIDString.length() + 1);
  wifiPWString.toCharArray(wifiPW, wifiPWString.length() + 1);
  WiFi.begin(wifiID, wifiPW);

  Serial.println(wifiIDString);
  // Wait until wifi connected
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Get IP Address and create URL
  IPAddress address = WiFi.localIP();
  String addressString = address.toString();
  String urlString = "http://" + addressString + "/";

  // Start web server
  server.begin();

  // Display SSID, IP Address and QR code for this M5Paper
  canvas.drawString(wifiIDString, 540, 20);
  canvas.drawString(urlString, 540, 40);
  canvas.qrcode(urlString, 20, 20, 500, 2);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void loop()
{
  // Wait client in main loop
  WiFiClient client = server.available();

  if (client)
  {
    enum ConnectionType connectionType = UNDEFINED_CONNECTION;
    enum FileType fileType = UNDEFINED_FILE;
    String result = "";
    String line = "";

    // Write HTTP header into log file
    File logFile = SD.open("/log.txt", FILE_WRITE);

    while (client.connected())
    {
      if (client.available())
      {
        line = client.readStringUntil('\n');
        Serial.println(line);
        if (logFile)
          logFile.println(line);

        // Read http header
        if (line.startsWith("GET /"))
          connectionType = GET_CONNECTION; // In this code, only "/" used. otherwise return 404
        else if (line.startsWith("POST /"))
          connectionType = POST_CONNECTION;
        else if (line.startsWith("Content-Type:"))
        {
          if (line.indexOf("image/png") > 0)
            fileType = PNG_FILE;
          else if (line.indexOf("image/jpeg") > 0)
            fileType = JPG_FILE;
        }

        if (line.length() <= 2) // Empty line. header finished
        {
          switch (connectionType)
          {
          case GET_CONNECTION:
          { // Send form.html
            sendResponse(client, "HTTP/1.1 200 OK");
            sendFormHTML(client);
            result = "form.html sent.";
            Serial.println("GET_CONNECTION");
            break;
          }
          case POST_CONNECTION:
          { // Receive form text or file
            Serial.println("POST_CONNECTION");
            switch (fileType)
            {
            case PNG_FILE:
            {
              Serial.println("PNG_FILE");
              receivePostFile(client, "/received.png");
              break;
            }
            case JPG_FILE:
            {
              Serial.println("JPG_FILE");
              receivePostFile(client, "/received.jpg");
              break;
            }
            default:
            {
              Serial.println("Text file");
              receiveFormText(client);
            }
            }
            sendResponse(client, "HTTP/1.1 200 OK");
            client.println("<!DOCTYPE html><head><meta charset=\"UTF-8\"></head><body><p>Received successfully.</p></body>");
          }
          default:
          { // Other request
            sendResponse(client, "HTTP/1.1 404 Not Found");
            client.println("<!DOCTYPE html><head><meta charset=\"UTF-8\"></head><body><p>404 Not Found</p></body>");
          }
          }
          delay(1);
          client.stop();
        }
      }
    }
    if (logFile)
      logFile.close();

    if (receivedFileName.length() > 0)
    {
      Serial.println(receivedFileName);
      // File received!
      // Convert file name String to char[]
      char receivedFileNameChar[receivedFileName.length() + 1];
      receivedFileName.toCharArray(receivedFileNameChar, receivedFileName.length() + 1);

      if (receivedFileName.endsWith("png"))
      {
        // PNG file
        // Fill screen with white color first to prevent ghost
        canvas.fillCanvas(BLACK); 
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        canvas.drawPngFile(SD, receivedFileNameChar);
        canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
      }
      else if (receivedFileName.endsWith("jpg"))
      {
        // JPG file
        canvas.fillCanvas(BLACK);
        canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
        canvas.drawJpgFile(SD, receivedFileNameChar);
        canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
      }

      // Shutdown M5Paper
      delay(500);
      M5.shutdown();
      return;
    }
    canvas.drawString(result, 540, 60);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
  }
}

// Return http response header
void sendResponse(WiFiClient client, String firstLine)
{
  // First line should be like "HTTP/1.1 200 OK"
  client.println(firstLine);
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
}

// Return content of form.html in SD card
void sendFormHTML(WiFiClient client)
{
  File htmlFile = SD.open("/form.html");
  if (htmlFile)
  { // send form.html line by line
    while (htmlFile.position() < htmlFile.size())
    {
      String htmlLine = htmlFile.readStringUntil('\n');
      client.println(htmlLine);
    }
    htmlFile.close();
  }
  else
  { // when form.html not found
    client.println("<!DOCTYPE html><head><meta charset=\"UTF-8\"></head><body><p>No html file!</p></body>");
  }
  client.flush();
}

// Handle POST body from text form. Content will be written in /received.txt on SD card
void receiveFormText(WiFiClient client)
{
  SD.remove("/received.txt");
  File receivedFile = SD.open("/received.txt", FILE_WRITE);
  while (client.available())
  {
    String line = client.readStringUntil('\n');
    if (line.length() <= 0)
    {
      break;
    }
    receivedFile.println(line);
  }
  receivedFile.close();
}

// Handle POST body from <input type="file">. No longer used in form.html
void receiveFormFile(WiFiClient client)
{
  String fileName = "/received.jpg";
  String boundary = "";
  // size_t boundarySize = 0;
  while (client.available())
  {
    // Read multipart header
    String line = client.readStringUntil('\n');
    if (line.length() < 2)
    {
      break;
    }
    else if (line.startsWith("-"))
    {
      boundary = line;
    }
    else if (line.startsWith("Content-Type:"))
    {
      if (line.indexOf("png") >= 0)
      {
        fileName = "/received.png";
      }
    }
  }
  // boundarySize = boundary.length();
  // In this code, multipart boundary string will be written after binary...
  receivePostFile(client, fileName);
}

// Handle POST body from XMLHttpRequest. More simple because of no boundary
void receivePostFile(WiFiClient client, String fileName)
{
  SD.remove(fileName);
  File receivedFile = SD.open(fileName, FILE_WRITE);
  while (client.available())
  {
    byte buffer[256];
    size_t bufferLength = client.readBytes(buffer, 256);

    if (bufferLength > 0)
    {
      receivedFile.write(buffer, bufferLength);
    }
    else
      break;
  }
  receivedFile.close();
  receivedFileName = fileName;
}