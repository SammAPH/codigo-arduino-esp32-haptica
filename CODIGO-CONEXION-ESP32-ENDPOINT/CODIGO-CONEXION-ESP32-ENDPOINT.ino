#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // Necesitas instalar esta librería desde el gestor de librerías

// Reemplaza con tu SSID y contraseña
const char* ssid = "Samm&";
const char* password = "DinoPulido2023";

// URL del endpoint expuesto por ngrok
const char* ngrokURL = "https://e06f-2801-1ca-b-114-f8ec-f27-fd06-24d2.ngrok-free.app/figures"; // Cambia esto por tu URL ngrok


unsigned long previousMillis = 0; // Almacena el último momento en que se realizó una solicitud
const long interval = 10000;      // Intervalo de 10 segundos

unsigned long ledPreviousMillis = 0;  // Almacena el último momento en que se cambió el estado del LED
int ledState = LOW;                   // Estado del LED, inicializado como apagado
int state = 0;                        // Variable para almacenar el estado
String temperature = "";              // Variable para almacenar la temperatura
int ledPin = 2;                       // Pin D2 del ESP32 (GPIO 2)

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);  // Configura el puerto Serial2 a 9600 baudios
  
  pinMode(ledPin, OUTPUT);  // Configura el pin D2 como salida
  
  // Conexión a la red Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nConectado a la red Wi-Fi");
}

void loop() {
  unsigned long currentMillis = millis(); // Obtiene el tiempo actual

  // Realiza la solicitud al endpoint cada 10 segundos
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Actualiza el tiempo anterior
    consumeEndpoint();              // Realiza la solicitud al endpoint
  }

  // Control del LED en función del estado
  controlLed(currentMillis);
  
  // Enviar datos por Serial2
  sendDataToArduino();
}

void consumeEndpoint() {
  if (WiFi.status() == WL_CONNECTED) { // Verifica si estás conectado a WiFi
    HTTPClient http;
    
    http.begin(ngrokURL); // Inicia la conexión con la URL
    int httpResponseCode = http.GET(); // Envía la solicitud GET
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Respuesta del servidor:");
      Serial.println(payload);
      
      if (payload[0] == '[' || payload[0] == '{') { // Verifica si la respuesta podría ser JSON
        extractDataFromResponse(payload);
      } else {
        Serial.println("La respuesta no es un JSON válido");
      }
    } else {
      Serial.print("Error en la solicitud GET: ");
      Serial.println(httpResponseCode);
    }
    
    http.end(); // Finaliza la conexión
  } else {
    Serial.println("Error: No conectado a WiFi");
  }
}

void extractDataFromResponse(String jsonResponse) {
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(3) + 60;
  DynamicJsonDocument doc(capacity);
  
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    Serial.print("Error al analizar JSON: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject obj = doc[0];

  temperature = obj["temperature"].as<String>();
  state = obj["state"].as<int>();
  
  Serial.print("Temperatura: ");
  Serial.println(temperature);
  
  Serial.print("Estado: ");
  Serial.println(state);
}

void controlLed(unsigned long currentMillis) {
  int ledInterval;

  switch (state) {
    case 1:
      digitalWrite(ledPin, LOW);  // Apaga el LED
      break;
    case 2:
      ledInterval = 1000;  // Baja frecuencia: 1 segundo (1000 ms)
      break;
    case 3:
      ledInterval = 500;   // Frecuencia media: 0.5 segundos (500 ms)
      break;
    default:
      digitalWrite(ledPin, LOW);  // Apaga el LED por defecto
      return;
  }

  if (state == 2 || state == 3) {
    if (currentMillis - ledPreviousMillis >= ledInterval) {
      ledPreviousMillis = currentMillis;
      ledState = !ledState;  // Cambia el estado del LED
      digitalWrite(ledPin, ledState);
    }
  }
}

void sendDataToArduino() {
  // Envía los datos de temperatura y estado por Serial2
  Serial2.print("Temperatura: ");
  Serial2.println(temperature);
  
  Serial2.print("Estado: ");
  Serial2.println(state);
}