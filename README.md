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

## Licenses Notation
"default_16MB.csv" in this project is used under following license:

MIT License

Copyright (c) 2020 m5stack

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.