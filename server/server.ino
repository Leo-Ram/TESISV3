#include <WiFi.h>  //red wifi
#include <esp_wifi.h>
#include <WebServer.h>    // web comunicacion
#include <ArduinoJson.h>  // para archivos json
#include <ESPmDNS.h>      // no escribir ip
#include <Preferences.h>  //guardad coniguracion
#include "config.h"       // credenciales wifi
#include "pagina.h"       // partes de la pagina
#include <Arduino.h>      // tareas paraleo

#include <Wire.h>    //leer corriente
#include <Adafruit_GFX.h> // screen 
#include <Adafruit_SSD1306.h> // screen
// Definir las dimensiones de la pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

#include "INA3221.h"          // usar INA

#include <SPI.h>  //usar puerto SPI para SD
#include <SD.h>   //usar SD

#include <DHT.h>  //sensor de temperatura

#include <time.h>  //contar tiempo

#define CS_PIN 5
#define DHT_PIN 13  // s
#define ANG_PIN 36
#define ANGT_PIN 34
#define MXA_PIN 27
#define MXB_PIN 14
#define MXC_PIN 12
#define LED_PIN 2
#define OVP_PIN 25  //contro overvoltage
#define UVP_PIN 26  //control undervoltage
#define OFF_PIN 32  // encendido

#define BAL_PIN 33 // encendido balanceo 

const int pin[] = { MXA_PIN, MXB_PIN, MXC_PIN, LED_PIN, BAL_PIN, OFF_PIN, OVP_PIN, UVP_PIN };
INA3221 INA(0x40);
DHT dht(DHT_PIN, DHT11);

WebServer server(80);
Preferences pre;

TaskHandle_t Task1;
TaskHandle_t Task2;
SemaphoreHandle_t mutex;

float bat[6] = { 3.7, 3.8, 3.9, 4.0, 4.1, 4.2 };
float lec[9] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 599, 28, 24 };
float g[10] = {};
float conf[10] = {};
float absolut[6] = {};
bool boton[4] = {};
int cc = 255;
int dd = 255;

const int ch = 3;
String  ipS = "";
const int margen = 300;
const int margenM = 150;


unsigned long ti;     // tiempo inicial
time_t ta;            // tiempo actual
struct tm* timeinfo;  // timepo para guardar
unsigned long tp;     // tiempo pasado
void tiempoin() {
  struct tm tiempo = { 0 };
  tiempo.tm_year = 2024 - 1900;  // Años desde 1900
  tiempo.tm_mon = 0;             // 0-11 (0 = Enero)
  tiempo.tm_mday = 1;            // 1-31
  tiempo.tm_hour = 0;
  tiempo.tm_min = 0;
  tiempo.tm_sec = 0;
  tp = mktime(&tiempo);

  ti = millis();
  ta = 0;
}

const char* CONFIG_FILE = "/config.json";

void estado() {
  DynamicJsonDocument doc(1024);
  doc["GS"] = 0;
  
  File configFile = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  
  serializeJson(doc, configFile);
  configFile.close();
}

void actualVariable() {
  File configFile = SD.open(CONFIG_FILE);
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }
  
  // Valores g[]
  g[0] = doc["GB1"] | 1.5f;
  g[1] = doc["GB2"] | 1.5f;
  g[2] = doc["GB3"] | 1.5f;
  g[3] = doc["GB4"] | 1.5f;
  g[4] = doc["GB5"] | 1.5f;
  g[5] = doc["GB6"] | 1.5f;
  g[6] = doc["GBI"] | 1.0f;
  g[7] = doc["GT"] | 1.0f;
  g[8] = doc["GANG"] | 0.0f;
  g[9] = doc["GS"] | 0.0f;
  
  // Valores conf[]
  conf[0] = doc["OVP"] | 4.2f;  // over voltage protection
  conf[1] = doc["OVPR"] | 4.0f;   // over voltage protection recovery
  conf[2] = doc["UVP"] | 3.0f;    //under voltage protection
  conf[3] = doc["UVPR"] | 3.4f;   // under voltage protection recovery
  conf[4] = doc["VBal"] | 0.1f;  //voltage balancing
  conf[5] = doc["CCP"] | 1500.0f; //maximum charging current
  conf[6] = doc["DCP"] | 1500.0f; //maximum discharging current
  conf[7] = doc["TMin"] | 4.0f;   //minimum temperature
  conf[8] = doc["TMax"] | 60.0f; //maximum temperature
  conf[9] = doc["Cap"] | 1800.0f; // capacity
  
  // Valores absolut[]
  absolut[0] = doc["VMax"] | 4.2f;
  absolut[1] = doc["VMin"] | 2.8f;
  absolut[2] = doc["IMax"] | 1500.0f;
  absolut[3] = doc["ACap"] | 1800.0f;
  absolut[4] = doc["MBal"] | 1.0f;
  absolut[5] = doc["MRec"] | 2.0f;
  
  // Valores boton[]
  boton[0] = doc["carga"] | false;
  boton[1] = doc["descarga"] | false;
  boton[2] = doc["balance"] | false;
  boton[3] = doc["emergencia"] | false;
}

void handleGetConfig() {
  File configFile = SD.open(CONFIG_FILE);
  if (!configFile) {
    server.send(500, "text/plain", "Failed to open config file");
    return;
  }
  
  String json = configFile.readString();
  configFile.close();
  
  server.send(200, "application/json", json);
}
//----------------------------------------


// ... ...

void configuracion() {
  digitalWrite(LED_PIN, HIGH);
  String newJson = server.arg("plain");
  Serial.println("Nuevos datos recibidos: " + newJson);

  // Documento JSON para los nuevos datos
  DynamicJsonDocument newDoc(1024);
  DeserializationError error = deserializeJson(newDoc, newJson);
  
  if (error) {
    Serial.println("Error al parsear JSON nuevo");
    server.send(400, "text/plain", "Error en el formato JSON");
    digitalWrite(LED_PIN, LOW);
    return;
  }

  // Cargar configuración existente
  DynamicJsonDocument existingDoc(2048);
  File configFile = SD.open(CONFIG_FILE);
  if (configFile) {
    DeserializationError existingError = deserializeJson(existingDoc, configFile);
    configFile.close();
    
    if (existingError) {
      Serial.println("Error al parsear JSON existente");
    }
  }

  // Actualizar solo los campos recibidos
  JsonObject newObj = newDoc.as<JsonObject>();
  for (JsonPair p : newObj) {
    const char* key = p.key().c_str();
    JsonVariant value = p.value();

    // Verificar si es un booleano
    if (strcmp(key, "carga") == 0 || 
        strcmp(key, "descarga") == 0 || 
        strcmp(key, "balance") == 0 || 
        strcmp(key, "emergencia") == 0) {
      bool boolValue = strcmp(value.as<const char*>(), "false");
      existingDoc[key] = boolValue;
      Serial.print("Actualizando booleano - Key: ");
      Serial.print(key);
      Serial.print(" Valor: ");
      Serial.println(boolValue);
    } else {
      // Es un valor flotante
      float floatValue = value.as<float>();
      existingDoc[key] = floatValue;
      Serial.print("Actualizando flotante - Key: ");
      Serial.print(key);
      Serial.print(" Valor: ");
      Serial.println(floatValue);
    }
  }

  // Guardar la configuración actualizada
  configFile = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!configFile) {
    server.send(500, "text/plain", "Error al abrir archivo para escritura");
    digitalWrite(LED_PIN, LOW);
    return;
  }

  serializeJson(existingDoc, configFile);
  configFile.close();

  server.send(200, "text/plain", "Configuración actualizada correctamente");
  actualVariable();
  digitalWrite(LED_PIN, LOW);
}

// Función auxiliar para imprimir el contenido actual del archivo (útil para depuración)
void printCurrentConfig() {
  File configFile = SD.open(CONFIG_FILE);
  if (configFile) {
    Serial.println("Configuración actual:");
    while (configFile.available()) {
      Serial.write(configFile.read());
    }
    Serial.println();
    configFile.close();
  }
}
//----------------------------------------------------------------

void lecturas() {
  StaticJsonDocument<200> docLec;                         //crear documento json
  JsonArray data = docLec.createNestedArray("lecturas");  //añadir a documento
  for (int i = 0; i < 6; i++) {
    String key = "bat" + String(i + 1);
    if (g[8] > 1){
    docLec[key] = lec[i];
    } else{
    docLec[key] = bat[i];}
  }
  docLec["Total"] = lec[8];
  docLec["Current"] = lec[6];
  docLec["Temperature"] = lec[7];

  // Serializa el objeto JSON a una cadena
  String json;
  serializeJson(docLec, json);
  server.send(200, "application/json", json);
}

void handleNotFound() {
  digitalWrite(LED_PIN, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(LED_PIN, 0);
}

void canal(uint8_t valor) {
  digitalWrite(MXA_PIN, valor & 0x01);
  digitalWrite(MXB_PIN, (valor >> 1) & 0x01);
  digitalWrite(MXC_PIN, (valor >> 2) & 0x01);
}

void leer() {
  static const uint8_t canales[] = { 3, 0, 5, 7, 6, 4 };
  for (uint8_t i = 0; i < 6; i++) {
    canal(canales[i]);
    vTaskDelay(pdMS_TO_TICKS(100));
    lec[i] = analogRead(ANG_PIN);
  }
  lec[6] = INA.getCurrent_mA(1);
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    lec[7] = temp;
  }
  for (int i = 0; i < 6; i++) {
    lec[i] = lec[i] / g[i];
  }
  for (int i = 0; i < 6; i++) {
    bat[i] = lec[i] + g[6];
  }
  lec[8] = analogRead(ANGT_PIN);
  lec[8] = lec[8]/g[7];
  escribirsd();
  dispPrint();
}

void dispPrint(){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(ipS);
    for (int i = 0; i < 3; i++) {
      display.setCursor(i * 40, 16); // Colocar el texto (división de 128px en 3 columnas)
      display.print(bat[i]);
    }

    for (int i = 0; i < 3; i++) {
      display.setCursor(i * 40, 32); // Fila inferior (y = 16 para la segunda fila)
      display.print(bat[i+3]);
    }
    display.setCursor(0, 48);
    display.print((int)lec[6]);
    display.setCursor(60, 48);
    display.print((int)lec[8]);
    display.display();

}


//----------------------------------------------------------------contro 0

void apagarSalidas(){
  analogWrite(OVP_PIN,0);
  analogWrite(UVP_PIN,0);
  digitalWrite(BAL_PIN, LOW);
}

void control(){
  if ((lec[7] < conf[7]) || (lec[7] > conf[8])){ // temperatura
    Serial.println("temperatura inadecuada");
    apagarSalidas();
    return;
  }
  if (boton[3]) {  //modo emergencia
    analogWrite(OVP_PIN,0);
    analogWrite(UVP_PIN,255);
    digitalWrite(BAL_PIN, HIGH);
    return;
  }
  bool estado [5] = {true, true, true, true, false}; // need carga cargar descarga balance
  for (int i = 0; i < 6; i++){
    float aux = bat[i];
    estado[0] &=  aux < conf[0];
    estado[1] &= aux < conf[1];
    estado[2] &= aux > conf[2];
    estado[3] &= aux > conf[3];
    if ((abs(bat[0] - aux)) > conf[4] ) {
      estado[4] = true;
    }
    if(aux < absolut[1]){
      digitalWrite(OFF_PIN, LOW);
    }
  }
  if (boton[0] && estado[0]) { // carga
    if(estado[1]){
      if (abs(lec[6] + conf[5]) > margen){
        ajusteCarga();
      }
      //Serial.println("ajusteCarga");
    }
  }else {
    analogWrite(OVP_PIN, 0);
    //Serial.println("apagaCarga");
  }
  if(boton[1] && estado[2]){
    if(estado[3]){
      if (abs(lec[6] - conf[6]) > margen){
        ajusteDescarga();
      }
         //Serial.println("ajusteDescarga");
    }
  }else {
    analogWrite(UVP_PIN, 0);
   // Serial.println("apagaDesarga");
  }
  if (boton[2] && estado[4]){
    digitalWrite(BAL_PIN, HIGH);
  }else{
    digitalWrite(BAL_PIN, LOW);
  }
}
void ajusteCarga(){
  //Serial.println(cc);
  analogWrite(OVP_PIN, cc);
  vTaskDelay(pdMS_TO_TICKS(5));
  lec[6] = INA.getCurrent_mA(1);

  if (-lec[6] > (conf[5] + margen) ){
    cc = 0;
    analogWrite(OVP_PIN, cc);
    //Serial.println("Apagon_Carga");
  }
  while ((-lec[6] < (conf[5]-margenM)) && (cc < 255)){
    cc = min(255, cc + 1);
    //Serial.println(cc);
    analogWrite(OVP_PIN, cc);
    vTaskDelay(pdMS_TO_TICKS(5));
    lec[6] = INA.getCurrent_mA(1);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0); 
    display.print(lec[6]);
    display.setCursor(0, 20);
    display.print(cc);
    display.setCursor(60, 0);
    display.print(conf[5]);
    display.display();
  }
}
void ajusteDescarga(){
  //Serial.println(dd);
  analogWrite(UVP_PIN, dd);
  vTaskDelay(pdMS_TO_TICKS(5));
  lec[6] = INA.getCurrent_mA(1);

  if (lec[6] > (conf[6] + margen) ){
    dd = 0;
    analogWrite(OVP_PIN, dd);
    //Serial.println("Apagon_Descarga");
  }
  while ((lec[6] < (conf[6]-margenM)) && (dd < 255)){
    dd = min(255, dd + 1);
    //Serial.println(dd);
    analogWrite(UVP_PIN, dd);
    vTaskDelay(pdMS_TO_TICKS(5));
    lec[6] = INA.getCurrent_mA(1);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0); 
    display.print(lec[6]);
    display.setCursor(0, 20);
    display.print(dd);
    display.setCursor(60, 0);
    display.print(conf[6]);
    display.display();
  }
}

//---------------------------------------------------------------- contro fin 0



void escribirsd() {
  ta = tp + ((millis() - ti) / 1000);
  timeinfo = gmtime(&ta);  // tiempo para guardar
  File dataFile = SD.open("/datos.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.seek(dataFile.size());
    for (int i = 0; i < 9; i++) {
      if (i < 6) {
        dataFile.print(bat[i]);
        dataFile.print(",");
      } else {
        dataFile.print(lec[i]);
        dataFile.print(",");
      }
    }
    dataFile.print(timeinfo);
    dataFile.println();
    dataFile.close();
    //Serial.println("Data saved");
  } else {
    Serial.println("Error opening data file");
  }
}

void setupServer() {
  server.on("/", []() {
    server.send(200, "text/html", paginaHTML);
  });
  server.on("/style.css", []() {
    server.send(200, "text/css", paginaCSS);
  });
  server.on("/script.javascript", []() {
    server.send(200, "text/javascript", paginaJS);
  });
  //no base
  server.on("/conf", HTTP_POST, []() {
    configuracion();
  });
  server.on("/lec", HTTP_GET, []() {
    lecturas();
  });
  server.on("/datasaved", HTTP_GET, []() {
    handleGetConfig();
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void Task1code(void* pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1 segundo de ciclo total

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    // Ejecutar leer()
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      leer();
      xSemaphoreGive(mutex);
    }
    // Esperar 500ms
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    // Ejecutar control()
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      control();
      //Serial.println("control");
      xSemaphoreGive(mutex);
    }
    // Esperar hasta completar 1 segundo desde el inicio del ciclo
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void Task2code(void* pvParameters) {
  setupServer();  // Función para configurar el servidor

  for (;;) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      server.handleClient();
      xSemaphoreGive(mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1));  // Pequeña pausa para no saturar el CPU
  }
}

void wifi1() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int c = 0;
  while ((WiFi.status() != WL_CONNECTED) && (c < 20)) {
    delay(1000);
    Serial.print(".");
    c++;
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ipS = WiFi.localIP().toString();
  dns1();
}

void dns1() {
  int c = 0;
  while ((!MDNS.begin(name)) && (c < 20)) {
    Serial.println("Error setting up MDNS responder!");
    delay(1000);
    c++;
  }
}

void initina() {
  Wire.begin();
  int c = 0;
  while((!INA.begin()) && (c < 10)){
    Serial.println("Failed to find INA3221 chip, retrying...");
    Serial.println(c);
    delay(1000);
    c++;
  }
  INA.setShuntR(0, 0.100);
  INA.setShuntR(1, 0.015);
  INA.setShuntR(2, 0.100);
  Serial.println("INA3221 iniciado");
}

void initsd() {
  int c = 0;
  while ((!SD.begin(CS_PIN)) && (c < 10)) {
    Serial.println("Failed to find SD, retrying...");
    delay(1000);
    c++;
  }
  Serial.println("Tarjeta SD inicializada correctamente");
}

void initpin() {
  for (int i = 0; i < 5; i++) {
    pinMode(pin[i], OUTPUT);
    digitalWrite(pin[i], 0);
  }
  pinMode(OFF_PIN, OUTPUT);
  digitalWrite(OFF_PIN,1);
  pinMode(OVP_PIN, OUTPUT);
  pinMode(UVP_PIN, OUTPUT);
  analogWrite(OVP_PIN, 255);
  analogWrite(UVP_PIN, 255);
  pinMode(ANG_PIN, INPUT);
  pinMode(ANGT_PIN, INPUT);
}

void initoled () {
  // Iniciar la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("No se pudo iniciar la pantalla OLED"));
  //  while (true); // Detener el programa si falla la inicialización
  }

  // Limpiar el buffer de la pantalla
  display.clearDisplay();
  
  // Mostrar un texto de prueba en la pantalla
  display.setTextSize(1.2);      // Tamaño del texto
  display.setTextColor(SSD1306_WHITE); // Color del texto
  display.setCursor(0, 0);     // Posición del texto
  display.println(F("Hola Mundo!"));
  
  // Actualizar la pantalla con lo que hemos escrito
  display.display();
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  Serial.println("Hello!");
  initpin();
  estado();  // para que wifi no se apague
  wifi1();
  initina();
  initsd();
  initoled();
  actualVariable();
  tiempoin();

  mutex = xSemaphoreCreateMutex();
  // Crea las tareas en diferentes núcleos
  xTaskCreatePinnedToCore(
    Task1code,  // Función de la tarea
    "Task1",    // Nombre de la tarea
    10000,      // Tamaño del stack
    NULL,       // Parámetro de entrada
    1,          // Prioridad de la tarea
    &Task1,     // Manejador de la tarea
    0);         // Núcleo al que se asigna la tarea

  xTaskCreatePinnedToCore(
    Task2code,  // Función de la tarea
    "Task2",    // Nombre de la tarea
    10000,      // Tamaño del stack
    NULL,       // Parámetro de entrada
    1,          // Prioridad de la tarea
    &Task2,     // Manejador de la tarea
    1);         // Núcleo al que se asigna la tarea
}

void loop(void) {}