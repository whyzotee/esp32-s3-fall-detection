
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

const char *ssid = "ESP32-OTA";
const char *password = "12345678";

WebServer server(80);

const char *uploadPage = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
<h2>ESP32 OTA Update</h2>
<form method='POST' action='/update' enctype='multipart/form-data'>
  <input type='file' name='firmware'>
  <input type='submit' value='Update'>
</form>
</body>
</html>
)rawliteral";

void handleRoot()
{
    server.send(200, "text/html", uploadPage);
}

void handleUpdate()
{
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.println("OTA Start");

        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (Update.end(true))
        {
            Serial.println("OTA Success");
        }
        else
        {
            Update.printError(Serial);
        }
    }
}

void setup_ota()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    Serial.println("AP Started");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);

    server.on("/update", HTTP_POST, []()
              {
                server.send(200, "text/plain", "Update Success! Rebooting...");
                delay(1000);
                ESP.restart(); }, handleUpdate);

    server.begin();
}

void handle_ota(void)
{
    server.handleClient();
}
