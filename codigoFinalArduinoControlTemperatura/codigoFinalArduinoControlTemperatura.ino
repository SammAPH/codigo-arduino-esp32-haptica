/* 
RESUMEN
Este código contiene las instrucciones para enviar y recibir datos en tiempo real a travez de un arduino mega adk y un esp32
la conexión entre el arduino y el esp32 se establece por los pines tx1 y rx1 siendo esta una comunicación tipo serie.
El esp32 se conecta a una base de datos para realizar la petición de datos. Acto seguido envía los datos al arduino, datos tipo string, el arduino los recibe 
y en función de los datos recibidos evalúa las condiciones e instrucciones a ejecutar. 
El arduino esta programado usando switch case, se tienen 3 estados posibles. 1. Todo apagado. 2. Generación de calor y 3. Generación de frio. Se tienen las siguientes entradasy salidas:
INPUT:
Sensor de temperatura LM35 --> PIN A0
Sensor de temperatura LM35 --> PIN A1

OUTPUT:
IRF440N(Celda peltier calor) --> PIN DIGITAL 5
IRF440N(Celda peltier frio) --> PIN DIGITAL 6
IRF440N(Cartucho impresión) --> PIN DIGITAL 7 
Ventilador aire caliente --> PIN PWM 8
Ventilador aire frío --> PIN PWM 9 
Ventilador discipador celda frío --> PIN DIGITAL 10

COMUNICACION:
RX2 --> PIN 17
TX2 --> PIN 16

 **LA EXPLICACION DE LAS FUNCIONES SE ENCUENTRA EN EL APARTADO "FUNCIONES"**

*/

/*

DEFINICION DE VARIABLES
1- todas las variables bajo constint refieren los pines de entrada / salida de datos y señales de control
2- String: _tempValue: variable que recibe el dato por com serial del esp32 referente a la temperatura que se desea alcanzar mediante el sistema
3- Unsigned long y const long: para la función millis que servirá para contabilizar tiempo y así evitar el uso de delays
4- float: _temperaturaSetpoint: es la variable que almacena el dato de temperatura recibido del esp32 y ya convertido a float que será el setpoint al que llegará el sistema 
          _sensorSignal1: variable que almacena la lectura del sensor de temperatura 1 (calor) señal que llega al pinSensor1(A0)
          _sensorSignal2: variable que almacena la lectura del sensor de temperatura 2 (frio) señal que llega al pinSensor2(A1)
          _voltaje: variable que almacena la conversion de la señal de entrada a un valor de voltaje proporcional 
          _temperaturaMedida1: variable que almacena el valor de temperatura ya convertida de voltaje a °C del sensor 1 (calor)
          _temperaturaMedida2: variable que almacena el valor de temperatura ya convertida de voltaje a °C del sensor 2 (frio)
5- const float: _voltajeReferencia: Es una constante que establece el valor de referencia del sensor de temperatura y operación
                _gradoPorVoltio: Se refiere a la equivalencia entre la señal del sensor y la temperatura medida
6- int: _state: Variable que almacena el estado en el que debe estar el sistema según la funcion switch case, se recibe como int desde el esp32

*/

//DECLARACION DE VARIABLES / pinSensor1 mide conducto caliente y pinSensor2 mide conducto frio. 

const int pinSensor1 = A0, pinSensor2 = A1, peltierHeat = 5, peltierCold = 6, cartucho = 7, hotAirFan = 8, coldAirFan = 9, heatsinkFan = 10;
String tempValue="";
unsigned long tiempoAnterior = 0; // Variable para almacenar el tiempo anterior
const long intervalo = 2000; // Intervalo de tiempo para el parpadeo (en milisegundos)
float temperaturaSetpoint, sensorSignal1, sensorSignal2, voltaje, temperaturaMedida1, temperaturaMedida2;
// Define la constante de conversión (temperatura en grados Celsius por voltio)
const float voltajeReferencia = 5.0;  // Voltaje de referencia del Arduino (5V o 3.3V)
const float gradoPorVoltio = 105.0;    // El LM35 proporciona 10 mV por grado Celsius
int state;

//-------------------------------------------------------------Configuracion de pines y comunicacion---------------------------------------------------------------------------------

void setup() {
  
  //Configuracion de pines de entrada y salida
    pinMode(peltierHeat, OUTPUT);
    pinMode(peltierCold, OUTPUT);
    pinMode(cartucho, OUTPUT);
    pinMode(hotAirFan, OUTPUT);
    pinMode(coldAirFan, OUTPUT);
    pinMode(heatsinkFan, OUTPUT);

  //Puerto serial depuracion
    Serial.begin(9600);
  //Puerto serial comunicacion con ESP32, pines --> RX2=PIN17  TX2=PIN16
    Serial2.begin(9600);
    
    digitalWrite(peltierHeat, HIGH);
    digitalWrite(peltierCold, HIGH);
    digitalWrite(cartucho, HIGH);
    digitalWrite(hotAirFan, LOW);
    digitalWrite(coldAirFan, LOW);
    digitalWrite(heatsinkFan, LOW);
}

//----------------------------------------------------------------------------Codigo de proceso------------------------------------------------------------------------------------

void loop() {
  
    comunicacion();
    stateMachine();

}

//--------------------------------------------------------------------------------Funciones----------------------------------------------------------------------------------------

/*
EXPLICACION DE FUNCIONES

  Funciones principales
    1- comunicacion(); --> Funcion que se encarga de recibir y enviar datos al esp32, se encarga de la conexión serial 
    2- stateMachine(); --> Función que alberga la maquina de estados
  Funciones auxiliares
    1- setPoint(); --> Función que se encarga de recibir el dato string del esp32 y convertirlo en un dato de tipo float para su posterior procesamiento 
    2- caso1(); --> Función para definir el caso o estado #1 donde todas las salidas estan en modo off o apagadas
    3- caso2(); --> Función para definir el caso o estado #2 donde se generará la sensación de calor, encendiendo así la celda peltier de calor, a su vez esta contiene una función auxiliar que es: 
      controlTemperatura1(); -> Función que se encarga de medir la temperatura con base en la señal del sensor 1 y activa la celda de calor (y el cartucho) hasta alcanzar el setpoint además de activar el ventilador que impulsará el aire caliente por el conducto  
    4- caso3(); --> Función para definir el caso o estado #3 donde se generará la sensación de frio, encendiendo así la celda peltier de frio, a su vez esta contiene una función auxiliar que es: 
      controlTemperatura2(); -> Función que se encarga de medir la temperatura con base en la señal del sensor 2 y activa la celda de frio  hasta alcanzar el setpoint además de activar el ventilador que impulsará el aire frio por el conducto y el ventilador de refrigeración del disipador de la celda de frio

*/

//COMUNICACION SERIAL (ENVÍO Y RECEPCIÓN DE DATOS CON ESP32)
void comunicacion() {
    // Verificar si hay datos disponibles en Serial2
    if (Serial2.available() > 0) {
        // Leer el entero desde Serial2
        state = Serial2.parseInt();

        // Limpiar cualquier espacio en blanco restante
        while (Serial2.peek() == ' ') {
            Serial2.read();
        }

        // Leer la cadena de texto desde Serial2
        tempValue = Serial2.readStringUntil('\n');
        //llama a la funcion que transforma el string a float 
        temperaturaSetpoint = setpoint(tempValue);

        // Imprimir los datos recibidos
        Serial.print("Estado recibido: ");
        Serial.println(state);
        Serial.print("Valor temperatura recibido: ");
        Serial.println(tempValue);

    }
}

//MAQUINA DE ESTADOS
void stateMachine() {
  
    switch(state){

        case 1:
          caso1();
        break;  

        case 2:
          caso2();
        break;

        case 3:
          caso3();
        break;
    }
}

//FUNCIONES AUXILIARES

//FUNCION PARA TRANSFORMAR EL STRING A FLOAT 
float setpoint (String texto) {
    // Convertir el String a float usando toFloat()
    return texto.toFloat();
}

//FUNCION CASO 1
void caso1() {

    digitalWrite(peltierHeat, HIGH);//LAS CELDAS TRABAJAN CON LOGICA NEGADA
    digitalWrite(peltierCold, HIGH);
    digitalWrite(cartucho, HIGH);
    digitalWrite(hotAirFan, LOW);
    digitalWrite(coldAirFan, LOW);
    digitalWrite(heatsinkFan, LOW);
    Serial.println("Caso 1, all off");
}

//FUNCION CASO 2
void caso2() {

    controlTemperatura1();
}

//FUNCION CASO 3
void caso3() {

    controlTemperatura2();
}

//FUNCIONES PARA LEER Y CONTROLAR TEMPERATURA
void controlTemperatura1() {
    //for sensor 1
    sensorSignal1 = analogRead(pinSensor1);
    
    voltaje = sensorSignal1 * (voltajeReferencia / 1023);
    temperaturaMedida1 = voltaje * gradoPorVoltio;

    if (temperaturaMedida1 < temperaturaSetpoint) {
        digitalWrite(peltierHeat, LOW);
        digitalWrite(peltierCold, HIGH);
        digitalWrite(cartucho, LOW);
        digitalWrite(hotAirFan, HIGH);
        digitalWrite(coldAirFan, LOW);
        digitalWrite(heatsinkFan, LOW);
    }
    else {
        digitalWrite(peltierHeat, HIGH);
        digitalWrite(peltierCold, HIGH);
        digitalWrite(cartucho, LOW);
        digitalWrite(hotAirFan, HIGH);
        digitalWrite(coldAirFan, LOW);
        digitalWrite(heatsinkFan, LOW);
    }

    //imprimir la temperatura por el serial2 para enviarla al esp32
    // Obtener el tiempo actual
    unsigned long tiempoActual = millis();

    if (tiempoActual - tiempoAnterior >= intervalo) {
        // enviar temperatura al esp32
        Serial2.println(temperaturaMedida1);
        Serial.print("Temperatura sensor 1: ");
        Serial.print(temperaturaMedida1);
        Serial.println(" °C");
        Serial.println("Caso 2, hot air");
        // Actualizar el tiempo anterior al tiempo actual
        tiempoAnterior = tiempoActual;
    }

}

void controlTemperatura2(){
    //for sensor 2
    sensorSignal2 = analogRead(pinSensor2);
    
    voltaje = sensorSignal2 * (voltajeReferencia / 1023);
    temperaturaMedida2 = voltaje * gradoPorVoltio;

    if (temperaturaMedida2 > temperaturaSetpoint) {
        digitalWrite(peltierHeat, HIGH);
        digitalWrite(peltierCold, LOW);
        digitalWrite(cartucho, HIGH);
        digitalWrite(hotAirFan, LOW);
        digitalWrite(coldAirFan, HIGH);
        digitalWrite(heatsinkFan, HIGH);
    }
    else {
        digitalWrite(peltierHeat, HIGH);
        digitalWrite(peltierCold, HIGH);
        digitalWrite(cartucho, HIGH);
        digitalWrite(hotAirFan, LOW);
        digitalWrite(coldAirFan, HIGH);
        digitalWrite(heatsinkFan, LOW);
    }

    //imprimir la temperatura por el serial2 para enviarla al esp32
    // Obtener el tiempo actual
    unsigned long tiempoActual = millis();

     if (tiempoActual - tiempoAnterior >= intervalo) {
        // enviar temperatura al esp32
        Serial2.println(temperaturaMedida2);
        Serial.print("Temperatura sensor 2: ");
        Serial.print(temperaturaMedida2);
        Serial.println(" °C");
        Serial.println("Caso 3, cold air");
        // Actualizar el tiempo anterior al tiempo actual
        tiempoAnterior = tiempoActual;
    }

}

