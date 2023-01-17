#include <WiFiManager.h>
#include <FS.h>
#include <LittleFS.h>

#define MOSFET_PIN  15
#define MOSFET_ON   1
#define MOSFET_OFF  0
#define LED_ON      0
#define LED_OFF     1

WiFiManager wm;
WiFiManagerParameter par_baudrate("baudrate", "Baudrate", "9600", 40);
WiFiManagerParameter par_parity("parity", "Parity", "E", 40);

WiFiServer server(23);

void STC_Auto_ISP(char ch)
{
  static uint8_t STC_7F_Count=0;
  if (0x7F==ch)
  {
    STC_7F_Count++;
    if (STC_7F_Count>=50)
    {
      STC_7F_Count=0;
      digitalWrite(MOSFET_PIN, !digitalRead(MOSFET_PIN));
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }
  else
  {
    STC_7F_Count=0;
  }
}

void set_serial(void)
{
  unsigned long baud=115200;
  SerialConfig config=SERIAL_8N1;
  if (LittleFS.begin())
  {
    File file = LittleFS.open("/serial.txt", "r");
    if (file)
    {
      String line=file.readStringUntil('\n');
      par_baudrate.setValue(line.c_str(), 40);
      baud=line.toInt();
      line=file.readStringUntil('\n');
      par_parity.setValue(line.c_str(), 40);
      file.close();
      line.trim();
      if (0==line.compareTo("E"))
      {
        config=SERIAL_8E1;
      }
      else if (0==line.compareTo("O"))
      {
        config=SERIAL_8O1;
      }
      else
      {
        config=SERIAL_8N1;      
      }
    }
    LittleFS.end(); 
  }
  Serial.begin(baud,config);
  Serial.setTimeout(1);
}
void saveParamsCallback()
{
  if (LittleFS.begin())
  {
    File file = LittleFS.open("/serial.txt", "w");
    if (file)
    {
      file.println(par_baudrate.getValue());
      file.println(par_parity.getValue());
      file.close();
    }
    LittleFS.end(); 
  }
  set_serial();
}

void setup()
{
  WiFi.mode(WIFI_STA);

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, MOSFET_ON);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  set_serial();

  //reset settings - wipe credentials for testing
  // wm.resetSettings();

  wm.addParameter(&par_baudrate);
  wm.addParameter(&par_parity);
  wm.setConfigPortalBlocking(false);
  wm.setSaveParamsCallback(saveParamsCallback);
  wm.autoConnect();
  wm.startConfigPortal();

  server.begin();
}

void loop() 
{
  wm.process();
  WiFiClient client = server.available();
  if (client)
  {
    digitalWrite(MOSFET_PIN, MOSFET_ON);
    digitalWrite(LED_BUILTIN, LED_ON);
    client.setNoDelay(true);
    while(1)
    {
      if (client.available())
      {
        char ch = client.read();
        Serial.print(ch);
        STC_Auto_ISP(ch);
      }
      if (Serial.available())
      {
        String s = Serial.readString();
        client.print(s);
      }
      if(0==client.status())
      {
        digitalWrite(LED_BUILTIN, LED_OFF);
        break;
      }
    }
  }
}
