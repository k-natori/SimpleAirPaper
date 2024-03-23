# SimpleAirPaper
This is simple PlatformIO project to transfer an image to M5Paper via Wi-Fi connection.

## Usage
1. Copy form.html and wifi.txt to microSD card. SSID and password should be replaced with actual ones.
2. Power on M5Paper. M5Paper will connect to designated Wi-Fi and display QR code.
3. Read the QR code with your smartphone. Web browser will open form.html.
4. Select image to transfer and push upload button.
5. The image should be displayed on M5Paper, and M5Paper will shutdown automatically (e-ink display can keep image)

## Dependencies
This PlatformIO project depends on following libraries:
- M5Stack https://github.com/m5stack/M5Stack
- M5EPD https://github.com/m5stack/M5EPD
