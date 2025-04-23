#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


//Creacion de instancia UDP
WiFiUDP udp;
SoftwareSerial Pic;

/*
SERIAL: RX:13, TX:15
CONTROL A PIC: 4
*/

#define RX_PIC 13
#define TX_PIC 15
#define MCLR_PIC 4

#define D0  16
#define D1  5
#define D2  4
#define D3  0

/*FUNCIONES*/
void Wifi_Scanner(void);

/*SERIAL*/
unsigned long Serial_Tempo = 0;
char Serial_Recep = 0;
String Serial_str = "";

/*SERIAL PIC*/
unsigned long SerialP_Tempo = 0;
char SerialP_Recep = 0;
String SerialP_str = "";
int cnt_SerialP = 0;
char ArraySerial[1000]; 

/*PARAMETROS WIFI*/
String URL = "";
String SEND = "";
unsigned long PORT = 0;
String SSID = "";
String PASS = "";
uint8_t buffer[1000];
int cnt_buffer = 0;

unsigned long tempo_init = 0;
char tempo_flag = 0;

char wait_server = 0;
unsigned long tempo_server = 0;

void Read_Comando(String CMD);
void Send_UDP(String DATA);

void setup() {
  Serial.begin(115200);
  pinMode(2,OUTPUT);
  digitalWrite(2,LOW);
  Pic.begin(9600, SWSERIAL_8N1, 13, 15);
  pinMode(MCLR_PIC, INPUT_PULLUP);
  pinMode(D0,INPUT_PULLUP);
  pinMode(D1,INPUT_PULLUP);
  pinMode(D2,INPUT_PULLUP);
  pinMode(D3,INPUT_PULLUP);
  pinMode(14,INPUT_PULLUP);
  EEPROM.begin(255);
  tempo_init = millis();
}
unsigned long tempo_pic = 0;
unsigned long tempo_conect = 0;
unsigned long tempo_status = 0;
char flag_connect = 0;
char flag_status = 1;
char flag_correcto = 0;

char cambio = 0;

void loop() {
  //
  if(!flag_connect){
    if(millis() - tempo_conect >= 3000){
      Read_Comando("WIFI_CONNECT");
      flag_connect = 1;
      flag_status = 1;
      tempo_status = millis();
    }
  }
  if(flag_status == 1){
    if(millis() - tempo_status >= 10000){
      Read_Comando("WIFI_STATUS");
      flag_status = 0;
    }
  }

  //MENSAJE INICIAL
  if (tempo_flag == 0) {
    if (millis() - tempo_init >= 2000) {
      Serial.println("CONFIGURACIONES INICIALES");
      Read_Comando("URL?");
      Read_Comando("PORT?");
      Read_Comando("SSID?");
      Read_Comando("PASS?");
      tempo_flag = 1;
    }
  }

  //PUERTO SERIAL
  if (Serial.available() > 0) {
    char c = Serial.read();
    Serial_str += c;
    Serial_Tempo = millis();
    Serial_Recep = 1;
  }
  if (Serial_Recep) {
    if (millis() - Serial_Tempo > 20) {
      Pic.println("Comandos para PIC");
      Serial.println("Comandos para PIC");
      Pic.println(Serial_str);
      Serial.println(Serial_str);
      Read_Comando(Serial_str);
      Serial_str = "";
      Serial_Recep = 0;
    }
  }

  //SERIAL PIC
  if (Pic.available() > 0) {
    char c = Pic.read();
    ArraySerial[cnt_SerialP] = c;
    cnt_SerialP ++;
    //SerialP_str += c;
    SerialP_Tempo = millis();
    SerialP_Recep = 1;
  }
  if (SerialP_Recep) {
    if (millis() - SerialP_Tempo > 50) {
      Serial.println("PIC:");
      for(int i = 0; i < cnt_SerialP;i++){
        Serial.write(ArraySerial[i]);
      }
      Serial.println();
      //Send_UDP_PIC();
      cnt_SerialP = 0;
      //Serial.println(SerialP_str);
      //Read_Comando(SerialP_str);
      //SerialP_str = "";
      SerialP_Recep = 0;
    }
  }

  if (wait_server == 1) {
    if (millis() - tempo_server >= 4000) {
      UDP_Read();
      tempo_server = 0;
      wait_server = 0;
    }
  }

  if (millis() - tempo_pic >= 10000) {
    Serial.println("ENVIO WIFI");
    if(flag_connect == 0){
      Read_Comando("WIFI_CONNECT");
    }
    Pic.println("WIFI CONECTADO");
    if(flag_correcto == 1){
      if(cambio == 0){
        digitalWrite(2,LOW);
        cambio = 1;
      }else{
        digitalWrite(2,HIGH);
        cambio = 0;
      }
    }else{
      digitalWrite(2,HIGH);
    }
    tempo_pic = millis();
  }
}

void Read_Comando(String CMD) {
  char Serial_correcto = 0;
  if (CMD.indexOf("SCANNER") >= 0) {
    Wifi_Scanner();
    Serial_correcto = 1;
  }

  //COMANDO PARA ENVIAR POR WIFI
  if (CMD.indexOf("SEND:") >= 0) {
    int Cmd_SEND_Total = CMD.length();
    int Cmd_SEND_Init = CMD.indexOf("SEND:") + 5;
    int Cmd_SEND_Check = 0;
    String SEND_aux = "";
    for (int i = Cmd_SEND_Init; i < Cmd_SEND_Total; i++) {
      char c = CMD[i];
      if (c == 42) {
        Cmd_SEND_Check = 1;
        Serial_correcto = 1;
        Serial.print("SEND: ");
        Serial.println(SEND_aux);
        Send_UDP(SEND_aux);
      }
      if (Cmd_SEND_Check == 0) {
        SEND_aux += c;
      }
    }
  }

  if(CMD.indexOf("RELE_ON") >= 0){
    digitalWrite(14,HIGH);
    Serial_correcto = 1;
  }

  if(CMD.indexOf("RELE_OFF") >= 0){
    digitalWrite(14,LOW);
    Serial_correcto = 1;
  }

  //CONFIGURACION DE DATOS:
  if (CMD.indexOf("URL:") >= 0) {
    Serial.println("Configurando URL");
    int Cmd_URL_Total = CMD.length();
    int Cmd_URL_Init = CMD.indexOf("URL:") + 4;
    char Cmd_URL_Check = 0;
    String URL_aux = "";
    for (int i = Cmd_URL_Init; i < Cmd_URL_Total; i++) {
      char c = CMD[i];
      if (c == '*') {
        Cmd_URL_Check = 1;
        Serial.print("URL: ");
        URL = URL_aux;
        Serial.println(URL);
        int longitud = URL.length();
        EEPROM.write(41, longitud);
        longitud += 42;
        for (int i = 42; i < longitud; i++) {
          EEPROM.write(i, URL[i - 42]);
        }
        EEPROM.commit();
        Serial_correcto = 1;
        i = Cmd_URL_Total;
      }
      if (!Cmd_URL_Check) {
        URL_aux += c;
      }
    }
  }

  if (CMD.indexOf("PORT:") >= 0) {
    int Cmd_PORT_Total = CMD.length();
    int Cmd_PORT_Init = CMD.indexOf("PORT:") + 5;
    char Cmd_PORT_Check = 0;
    unsigned long PORT_aux = 0;
    for (int i = Cmd_PORT_Init; i < Cmd_PORT_Total; i++) {
      char c = CMD[i];
      if (c == '*') {
        Cmd_PORT_Check = 1;
        Serial.print("PORT: ");
        PORT = PORT_aux;
        Serial.println(PORT);
        EEPROM.write(61, PORT / 10000);
        EEPROM.write(62, (PORT / 100) % 100);
        EEPROM.write(63, PORT % 100);
        EEPROM.commit();
        Serial_correcto = 1;
        i = Cmd_PORT_Total;
      }
      if (!Cmd_PORT_Check) {
        if (c >= '0' && c <= '9') {
          PORT_aux = PORT_aux * 10 + c - 48;
        } 
      }
    }
  }

  if (CMD.indexOf("SSID:") >= 0) {
    int Cmd_SSID_Total = CMD.length();
    int Cmd_SSID_Init = CMD.indexOf("SSID:") + 5;
    char Cmd_SSID_Check = 0;
    String SSID_aux = "";
    for (int i = Cmd_SSID_Init; i < Cmd_SSID_Total; i++) {
      char c = CMD[i];
      if (c == '*') {
        Cmd_SSID_Check = 1;
        Serial.print("SSID: ");
        SSID = SSID_aux;
        Serial.println(SSID);
        int longitud = SSID.length();
        EEPROM.write(1, longitud);
        longitud += 2;
        for (int i = 2; i < longitud; i++) {
          EEPROM.write(i, SSID[i - 2]);
        }
        EEPROM.commit();
        Serial_correcto = 1;
        i = Cmd_SSID_Total;
      }
      if (!Cmd_SSID_Check) {
        SSID_aux += c;
      }
    }
  }

  if (CMD.indexOf("PASS:") >= 0) {
    int Cmd_PASS_Total = CMD.length();
    int Cmd_PASS_Init = CMD.indexOf("PASS:") + 5;
    char Cmd_PASS_Check = 0;
    String PASS_aux = "";
    for (int i = Cmd_PASS_Init; i < Cmd_PASS_Total; i++) {
      char c = CMD[i];
      if (c == '*') {
        Cmd_PASS_Check = 1;
        Serial.print("PASS: ");
        PASS = PASS_aux;
        Serial.println(PASS);
        int longitud = PASS.length();
        EEPROM.write(21, longitud);
        longitud += 22;
        for (int i = 22; i < longitud; i++) {
          EEPROM.write(i, PASS[i - 22]);
        }
        EEPROM.commit();
        Serial_correcto = 1;
        i = Cmd_PASS_Total;
      }
      if (!Cmd_PASS_Check) {
        PASS_aux += c;
      }
    }
  }

  if (CMD.indexOf("RESET") >= 0) {
    ESP.restart();
  }
  //CONSULTA DE DATOS:
  if (CMD.indexOf("URL?") >= 0) {
    int longitud = EEPROM.read(41) + 42;
    Serial.print("URL: ");
    URL = "";
    for (int i = 42; i < longitud; i++) {
      char c = EEPROM.read(i);
      URL += c;
    }
    Serial.println(URL);
    Serial_correcto = 1;
  }

  if (CMD.indexOf("SSID?") >= 0) {
    int longitud = EEPROM.read(1) + 2;
    Serial.print("SSID: ");
    SSID = "";
    for (int i = 2; i < longitud; i++) {
      char c = EEPROM.read(i);
      SSID += c;
    }
    Serial.println(SSID);
    Serial_correcto = 1;
  }

  if (CMD.indexOf("PASS?") >= 0) {
    int longitud = EEPROM.read(21) + 22;
    Serial.print("PASS: ");
    for (int i = 22; i < longitud; i++) {
      char c = EEPROM.read(i);
      PASS += c;
    }
    Serial.println(PASS);
    Serial_correcto = 1;
  }

  if (CMD.indexOf("PORT?") >= 0) {
    unsigned long PORT_aux = EEPROM.read(61) * 100;
    PORT_aux += EEPROM.read(62);
    PORT_aux *= 100;
    PORT_aux += EEPROM.read(63);
    Serial.print("PORT: ");
    PORT = PORT_aux;
    Serial.println(PORT);
    Serial_correcto = 1;
  }

  if (CMD.indexOf("PING") >= 0) {
    udp.begin(PORT);
    Serial.println("ENVIO A PLATAFORMA");

    memset(buffer, '\0', 1000);

    buffer[0] = 'P';
    buffer[1] = 'I';
    buffer[2] = 'N';
    buffer[3] = 'G';
    cnt_buffer = 4;

    //send hello world to server
    char V_URL[20];
    for (int i = 0; i < 20; i++) {
      V_URL[i] = '\0';
    }
    int ln_cmd = URL.length();
    for (int i = 0; i < ln_cmd; i++) {
      V_URL[i] = URL[i];
    }
    udp.beginPacket(V_URL, PORT);
    udp.write(buffer, cnt_buffer);
    udp.endPacket();
    memset(buffer, '\0', 1000);
    //processing incoming packet, must be called before reading the buffer
    udp.parsePacket();
    UDP_Read();
    Serial_correcto = 1;
    //tempo_recep = millis();
    //wait_recep = 1;
  }

  if (CMD.indexOf("WIFI_CONNECT") >= 0) {
    WiFi.begin(SSID, PASS);
    Serial_correcto = 1;
  }

  if (CMD.indexOf("WIFI_STATUS") >= 0) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("CONECTADO");
      flag_correcto = 1;
    } else {
      Serial.println("ERROR CONNECT");
      flag_connect = 0;
      flag_status = 0;
      flag_correcto = 0;
      tempo_conect = millis();
      tempo_status = millis();
    }
    Serial_correcto = 1;
  }

  if (!Serial_correcto) {
    Serial.println("COMANDO NO ACEPTADO");
    Pic.println(CMD);
  }
}

void UDP_Read(void) {
  //receive response from server, it will be HELLO WORLD
  delay(3000);
  if (udp.read(buffer, 1000) > 0) {
    Serial.print("Server to client: ");
    Serial.println((char *)buffer);
    Pic.println((char *)buffer);
  }
}

void Wifi_Scanner(void) {
  String ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t *bssid;
  int32_t channel;
  bool hidden;
  int scanResult;
  Serial.println(F("Starting WiFi scan..."));
  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  if (scanResult == 0) {
    Serial.println(F("No networks found"));
  } else if (scanResult > 0) {
    Serial.printf(PSTR("%d networks found:\n"), scanResult);

    // Print unsorted scan results
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);

      // get extra info
      const bss_info *bssInfo = WiFi.getScanInfoByIndex(i);
      String phyMode;
      const char *wps = "";
      if (bssInfo) {
        phyMode.reserve(12);
        phyMode = F("802.11");
        String slash;
        if (bssInfo->phy_11b) {
          phyMode += 'b';
          slash = '/';
        }
        if (bssInfo->phy_11g) {
          phyMode += slash + 'g';
          slash = '/';
        }
        if (bssInfo->phy_11n) {
          phyMode += slash + 'n';
        }
        if (bssInfo->wps) {
          wps = PSTR("WPS");
        }
      }
      Serial.printf(PSTR("  %02d: %ddBm %s\n"), i, rssi, ssid.c_str());
      yield();
    }
  } else {
    Serial.printf(PSTR("WiFi scan error %d"), scanResult);
  }

  // Wait a bit before scanning again
  delay(5000);
}

void Send_UDP(String DATA) {
  udp.begin(PORT);
  Serial.println("ENVIO A PLATAFORMA");

  memset(buffer, '\0', 1000);

  unsigned int L_DATA = DATA.length() - 1;
  for (int i = 0; i < L_DATA; i++) {
    buffer[i] = DATA[i];
    cnt_buffer++;
  }

  //send hello world to server
  char V_URL[20];
  for (int i = 0; i < 20; i++) {
    V_URL[i] = '\0';
  }
  int ln_cmd = URL.length();
  for (int i = 0; i < ln_cmd; i++) {
    V_URL[i] = URL[i];
  }
  udp.beginPacket(V_URL, PORT);
  udp.write(buffer, cnt_buffer);
  udp.endPacket();
  memset(buffer, '\0', 1000);
  //processing incoming packet, must be called before reading the buffer
  udp.parsePacket();
  //UDP_Read();
  wait_server = 1;
  tempo_server = millis();
  //tempo_recep = millis();
  //wait_recep = 1;
}

void Send_UDP_PIC(void) {
  udp.begin(PORT);
  Serial.println("ENVIO A PLATAFORMA");
  //send hello world to server
  char V_URL[20];
  for (int i = 0; i < 20; i++) {
    V_URL[i] = '\0';
  }
  int ln_cmd = URL.length();
  for (int i = 0; i < ln_cmd; i++) {
    V_URL[i] = URL[i];
  }

  udp.beginPacket(V_URL, PORT);
  udp.write(ArraySerial,cnt_SerialP);
  udp.endPacket();
  memset(ArraySerial, '\0', 1000);
  //processing incoming packet, must be called before reading the buffer
  udp.parsePacket();
  //UDP_Read();
  wait_server = 1;
  tempo_server = millis();
  //tempo_recep = millis();
  //wait_recep = 1;
}
