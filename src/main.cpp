#include <M5EPD.h>
#include <WiFi.h>
#include <WebServer.h>

#define buttonX 540
#define buttonY 400
#define buttonWidth 400
#define buttonHeight 100
#define statusX 540
#define statusY 60
#define statusWidth 400
#define statusHeight 30

M5EPD_Canvas canvas(&M5.EPD);
M5EPD_Canvas buttonCanvas(&M5.EPD);
M5EPD_Canvas statusCanvas(&M5.EPD);
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
void displayImageOfFileName(String fileName);

void setup()
{
  // Initialize M5Paper
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(true);

  receivedFileName = "";

  // Create fullscreen canvas
  canvas.createCanvas(960, 540);
  canvas.setTextSize(3);

  // Create shutdown button canvas
  buttonCanvas.createCanvas(buttonWidth, buttonHeight);
  buttonCanvas.setTextSize(3);

  // Create status text canvas
  statusCanvas.createCanvas(statusWidth, statusHeight);
  statusCanvas.setTextSize(3);

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

  // Draw shutdown button
  buttonCanvas.drawString("Shutdown", 80, 40);
  buttonCanvas.ReverseColor();
  canvas.drawRect(buttonX, buttonY, buttonWidth, buttonHeight, WHITE);
  canvas.drawString("Shutdown", buttonX + 80, buttonY + 40);

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
    String line = "";

    while (client.connected())
    {
      if (client.available())
      {
        line = client.readStringUntil('\n');
        Serial.println(line);

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

        if (line.length() <= 2) 
        { // Empty line. header finished
          switch (connectionType)
          {
          case GET_CONNECTION:
          { // Send form.html
            sendResponse(client, "HTTP/1.1 200 OK");
            sendFormHTML(client);
            Serial.println("GET_CONNECTION");

            statusCanvas.drawString("form.html sent.", 0, 0);
            statusCanvas.pushCanvas(statusX, statusY, UPDATE_MODE_DU4);
            break;
          }
          case POST_CONNECTION:
          { // Receive form text or file
            Serial.println("POST_CONNECTION");
            switch (fileType)
            {
            case PNG_FILE:
            { // Receive PNG file
              Serial.println("PNG_FILE");
              statusCanvas.drawString("Start receving PNG file.", 0, 0);
              statusCanvas.pushCanvas(statusX, statusY, UPDATE_MODE_DU4);
              receivePostFile(client, "/received.png");
              break;
            }
            case JPG_FILE:
            { // Receive JPG file (not used)
              Serial.println("JPG_FILE");
              statusCanvas.drawString("Start receving JPG file.", 0, 0);
              statusCanvas.pushCanvas(statusX, statusY, UPDATE_MODE_DU4);
              receivePostFile(client, "/received.jpg");
              break;
            }
            default:
            { // Receive TXT file (not used)
              Serial.println("Text file");
              statusCanvas.drawString("Start receving TXT file.", 0, 0);
              statusCanvas.pushCanvas(statusX, statusY, UPDATE_MODE_DU4);
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

    if (receivedFileName.length() > 0)
    { // File received!
      Serial.println(receivedFileName);

      // Display received file
      displayImageOfFileName(receivedFileName);

      // Shutdown M5Paper
      delay(500);
      M5.shutdown();
      return;
    }
  }

  // Touch detection for shutdown button
  if (M5.TP.avaliable())
  {
    if (!M5.TP.isFingerUp())
    {
      M5.TP.update();
      tp_finger_t FingerItem = M5.TP.readFinger(0);
      if (FingerItem.x > buttonX && FingerItem.x < (buttonX + buttonWidth) && FingerItem.y > buttonY && FingerItem.y < (buttonY + buttonHeight))
      { // Touch up inside shutdown button

        // Invert button image
        buttonCanvas.pushCanvas(buttonX, buttonY, UPDATE_MODE_DU4);

        // display last image
        displayImageOfFileName("/received.png");

        // Shutdown M5Paper
        delay(500);
        M5.shutdown();
        return;
      }
    }
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

void displayImageOfFileName(String fileName)
{
  char fileNameChar[fileName.length() + 1];
  fileName.toCharArray(fileNameChar, fileName.length() + 1);

  // Fill screen with white color first to prevent ghost
  canvas.fillCanvas(BLACK);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
  if (fileName.endsWith("png"))
  {
    // PNG file
    canvas.drawPngFile(SD, fileNameChar);
  }
  else if (fileName.endsWith("jpg"))
  {
    // JPG file
    canvas.drawJpgFile(SD, fileNameChar);
  }
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}