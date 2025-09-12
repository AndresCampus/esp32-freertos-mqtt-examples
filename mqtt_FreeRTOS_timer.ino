#include <WiFi.h>
#include <PubSubClient.h>

// definimos macro para indicar tarea, función y línea de código en los mensajes
#define DEBUG_STRING "["+String(pcTaskGetName(NULL))+" - "+String(__FUNCTION__)+"():"+String(__LINE__)+"]   "

// --- WiFi/MQTT ---
WiFiClient wClient;
PubSubClient mqtt_client(wClient);

const String ssid = "infind";
const String password = "1518wifi";
const String mqtt_server = "iot.ac.uma.es";
const String mqtt_user = "infind";
const String mqtt_pass = "***REMOVED***";

String ID_PLACA;
String topic_PUBLICACION;
String topic_SUSCRIPCION;

#define PERIODO_PUBlICACION 30000

// --- FreeRTOS ---
QueueHandle_t colaLed;
SemaphoreHandle_t semMqttReady;
TimerHandle_t timer_publisher;


//-----------------------------------------------------
// muestra información de la tarea
//-----------------------------------------------------
inline void info_tarea_actual() { 
 Serial.println(DEBUG_STRING+"Prioridad de tarea "+ String(pcTaskGetName(NULL))+": "+String(uxTaskPriorityGet(NULL)));
}

//-----------------------------------------------------
// Callback MQTT → recibe mensajes y los procesa
//-----------------------------------------------------
void procesa_mensaje(char* topic, byte* payload, unsigned int length) { 
  String mensaje="";
  for(int i=0; i<length; i++) mensaje += (char)payload[i];
  Serial.println(DEBUG_STRING+"Mensaje recibido ["+ String(topic) +"] \"" + mensaje + "\"");

  if(String(topic)==topic_SUSCRIPCION) {
    if (mensaje[0]=='1') 
        neopixelWrite(RGB_BUILTIN, 255, 0, 0); // LED rojo
      else 
        digitalWrite(RGB_BUILTIN, LOW);        // LED apagado
      }  
}

//-----------------------------------------------------
// Conexión con WiFi
//-----------------------------------------------------
void conecta_wifi() {
  Serial.println(DEBUG_STRING+"Connecting to " + ssid);
 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.print(".");
  }
  Serial.println();
  Serial.println(DEBUG_STRING+"WiFi connected, IP address: " + WiFi.localIP().toString());
}

//-----------------------------------------------------
// Conexión con MQTT
//-----------------------------------------------------
void conecta_mqtt() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.println(DEBUG_STRING+"Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect(ID_PLACA.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
      Serial.println(DEBUG_STRING+" conectado a broker: " + mqtt_server);
      mqtt_client.subscribe(topic_SUSCRIPCION.c_str());
    } else {
      Serial.println(DEBUG_STRING+"ERROR:"+ String(mqtt_client.state()) +" reintento en 5s" );
      // Wait 5 seconds before retrying
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

//-----------------------------------------------------
//   TAREAS FreeRTOS
//-----------------------------------------------------

//-----------------------------------------------------
// Mantener conexión y ejecutar loop MQTT
void taskMQTTService(void *pvParameters) {
  info_tarea_actual();
  // Inicialización de WiFi
  conecta_wifi();
  // Preparar identificadores
  ID_PLACA = String(WiFi.getHostname());
  topic_PUBLICACION = "infind/"+ ID_PLACA +"/publicacion";
  topic_SUSCRIPCION = "infind/"+ ID_PLACA +"/recepcion";

  Serial.println(DEBUG_STRING+"Identificador placa : "+ ID_PLACA);
  Serial.println(DEBUG_STRING+"Topic publicacion : "+ topic_PUBLICACION);
  Serial.println(DEBUG_STRING+"Topic suscripcion : "+ topic_SUSCRIPCION);

  // Inicializar cliente MQTT
  mqtt_client.setServer(mqtt_server.c_str(), 1883);
  mqtt_client.setBufferSize(512);
  mqtt_client.setCallback(procesa_mensaje);

  // Conectar a MQTT
  conecta_mqtt();

  Serial.println(DEBUG_STRING+"Semaforo abierto...");

  // Señalizar que MQTT ya está listo
  xSemaphoreGive(semMqttReady);

  // Bucle principal de servicio MQTT
  while(true) {
    if (!mqtt_client.connected()) conecta_mqtt();
    mqtt_client.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

//-----------------------------------------------------
// función de publicación que lanza el termporizador (timer)
void callback_publica( TimerHandle_t xTimer )
{
  String mensaje="Mensaje enviado desde "+ ID_PLACA +" en "+ String(millis()) +" ms";
  Serial.println(DEBUG_STRING+"Publicando: " + mensaje);
  mqtt_client.publish(topic_PUBLICACION.c_str(), mensaje.c_str());
}

//-----------------------------------------------------
// Configura y lanza la publicación cada cierto tiempo
void taskPublisher(void *pvParameters) {
  info_tarea_actual();
  timer_publisher= xTimerCreate("Temporizador publicador", pdMS_TO_TICKS(PERIODO_PUBlICACION), pdTRUE,  ( void * ) 0, callback_publica);
  Serial.println(DEBUG_STRING+"Tarea publicadora esperando en semáforo...");
  // Esperar a que MQTT esté listo
  xSemaphoreTake(semMqttReady, portMAX_DELAY);
  Serial.println(DEBUG_STRING+"Tarea publicadora supera el semáforo");
  // vTimerCallback(timer_publisher);  // si quieres una invocación inmediata podemos hacerlo nosotros manualmente
  xTimerStart( timer_publisher, 0 );
  Serial.println(DEBUG_STRING+"Timer en marcha...");
  vTaskDelete(NULL); // no hay nada más que hacer, el timer sigue su curso
  // una tarea o se termina con vTaskDelete o se queda en un bucle infinito, no puede hacer return!
}

//-----------------------------------------------------
//   SETUP
//-----------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();
  // Crear cola y semáforo
  colaLed = xQueueCreate(5, sizeof(int));
  semMqttReady = xSemaphoreCreateBinary();
  info_tarea_actual();

  // Arrancar primero la tarea MQTT (que inicializa conexión)
  xTaskCreate(taskMQTTService, "MQTT Service", 4096, NULL, 2, NULL);

  // Arrancar la otra tarea (esperará al semáforo de MQTT para publicar)
  xTaskCreate(taskPublisher,   "Publisher",   4096, NULL, 1, NULL);

  Serial.println(DEBUG_STRING+"Setup terminado, esperando conexión MQTT...");

  // --- Terminar la tarea loopTask ---
  vTaskDelete(NULL);
}

//-----------------------------------------------------
void loop() {
  // vacío: todo lo hacen las tareas FreeRTOS
}