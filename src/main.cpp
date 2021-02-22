#include <M5EPD.h>
#include <WiFi.h>
#include <WebServer.h>

M5EPD_Canvas canvas(&M5.EPD);
WiFiServer server(80);
String receivedFileName;

void displayForm(WiFiClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  File htmlFile = SD.open("/form.html");
  if (htmlFile)
  {
    while (htmlFile.position() < htmlFile.size())
    {
      String htmlLine = htmlFile.readStringUntil('\n');
      client.println(htmlLine);
    }
    htmlFile.close();
  }
  else
  {
    client.println("<!DOCTYPE html><head><meta charset=\"UTF-8\"></head><body><p>No html file!</p></body>");
  }
  client.flush();
}
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
void receiveFormFile(WiFiClient client)
{
  String fileName = "/received.jpg";
  String boundary = "";
  size_t boundarySize = 0;
  while (client.available())
  {
    String line = client.readStringUntil('\n');
    if (line.length() < 2) {
      break;
    }
    else if (line.startsWith("-")) {
      boundary = line;
    } else if (line.startsWith("Content-Type:")) {
      if (line.indexOf("png") >= 0) {
        fileName = "/received.png";
      }
    }
  }
  boundarySize = boundary.length();

  byte boundaryBytes[boundarySize + 1];
  boundary.getBytes(boundaryBytes, boundarySize + 1);

  SD.remove(fileName);
  File receivedFile = SD.open(fileName, FILE_WRITE);
  while (client.available())
  {
    byte buffer[256];
    size_t bufferLength = client.readBytes(buffer, 256);

    if (bufferLength > 0)
    {
      /*
      if (bufferLength == boundarySize) {
        if (memcmp(buffer, boundaryBytes, boundarySize) == 0) {
          break;
        }
      }
      */
      receivedFile.write(buffer, bufferLength);
    }
    else
    {
      break;
    }
  }
  receivedFile.close();
  receivedFileName = fileName;
}

void displayAndShutdown()
{
  canvas.drawString("Going to shutdown", 20, 20);
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  M5.shutdown();
}


void setup()
{
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(true);

  receivedFileName = "";

  canvas.createCanvas(960, 540);
  canvas.setTextSize(3);

  // Load WiFi SSID and PASS
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

  char wifiID[wifiIDString.length() + 1];
  char wifiPW[wifiPWString.length() + 1];
  wifiIDString.toCharArray(wifiID, wifiIDString.length() + 1);
  wifiPWString.toCharArray(wifiPW, wifiPWString.length() + 1);

  WiFi.begin(wifiID, wifiPW);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  IPAddress address = WiFi.localIP();
  String addressString = address.toString();
  String urlString = "http://" + addressString + "/";

  server.begin();

  canvas.drawString(wifiIDString, 540, 20);
  canvas.drawString(urlString, 540, 40);
  canvas.qrcode(urlString, 20, 20, 500, 2);

  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

void loop()
{
  WiFiClient client = server.available();
  if (client)
  {
    boolean isPOST = false;
    boolean isFile = false;
    String result = "";
    String line = "";

    File logFile = SD.open("/log.txt", FILE_WRITE);

    while (client.connected())
    {
      if (client.available())
      {
        line = client.readStringUntil('\n');
        if (line.startsWith("POST"))
        {
          isPOST = true;
        }
        else if (line.startsWith("Content-Type:"))
        {
          if (line.indexOf("multipart") > 0) 
          {
            isFile = true;
          }
        }
        if (logFile)
        {
          logFile.println(line);
        }
        result += line + "\n";
        if (line.length() <= 2) // Empty line. header finished
        {
          if (!isPOST)
          {
            displayForm(client);
            
          }
          else if (!isFile)
          {
            receiveFormText(client);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE html><head><meta charset=\"UTF-8\"></head><body><p>Text received</p></body>");

            result = "Text received";
          }
          else
          {
            receiveFormFile(client);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE html><head><meta charset=\"UTF-8\"></head><body><p>File received</p></body>");
            result = "File received";
          }
          delay(1);
          client.stop();
        }
      }
    }
    if (logFile)
    {
      logFile.close();
    }

    canvas.drawString(result, 520, 60);

    if (receivedFileName.length() > 0) {
      if (receivedFileName.endsWith("png")) {
        canvas.drawPngFile(SD, "/received.png");
      }
    }

    canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  }
}
