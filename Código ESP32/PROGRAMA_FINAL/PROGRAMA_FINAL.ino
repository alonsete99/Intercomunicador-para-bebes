#include <WiFi.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>

#define I2S_WS 14
#define I2S_SCK 27
#define I2S_SD 26
#define I2S_PORT I2S_NUM_0
#define I2S_PORT_2 I2S_NUM_1
#define I2S_SAMPLE_RATE           (16000)
#define I2S_SAMPLE_BITS           (16)
#define I2S_READ_LEN              (1024) 
#define I2S_CHANNEL_NUM           (1)

/*********LEDS*********/
int ledPin=19;
int ledPin2=23;
int conectionled=0;


TaskHandle_t taskHandler=NULL;
const char* ssid = "WIFI_INTERCOM";
const char* password = NULL;


int contconexion = 0;
int i2s_read_len=I2S_READ_LEN;
char* i2s_read_buff;
uint8_t* flash_write_buff;
size_t bytes_read;
WiFiUDP Udp;
int localPort = 8080;

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin,OUTPUT);
  pinMode(conectionled,OUTPUT);
  pinMode(ledPin2,OUTPUT);
  wifi_init();
  open_udp_port();

}

void loop()
{
 GetUDP_Packet(); 
}

void GetUDP_Packet()
{
   int packetSize = Udp.parsePacket();
   char packetBuffer[1024];
   if ((packetSize>0)&&(packetSize<512))
   {
      // read the packet into packetBufffer
      Udp.read(packetBuffer, 1024);

      Serial.println();
      Serial.print("Received packet of size ");
      Serial.print(packetSize);
      Serial.print(" from ");
      Serial.print(Udp.remoteIP());
      Serial.print(":");
      Serial.println(Udp.remotePort());
      int dataLen=(size_t)packetSize;
      char message[dataLen+1];
      strncpy(message,(const char *)packetBuffer,dataLen);
      message[dataLen]=0;
      String textString=message;
      Serial.printf("MENSAJE RECIBIDO: %s\n",textString);
      if(textString.equals("ENVIA")){
        digitalWrite(ledPin,HIGH);
        Serial.println("ENVIANDO...");
        startSending(); //Función que inicia la grabación
        
      }
      else if(textString.equals("RECIBE")){
        digitalWrite(ledPin2,HIGH);
        Serial.println("RECIBIENDO...");
        startReceiving(); //Función que inicia la grabación
        
      }
      else if(textString.equals("FINALIZA")){
        Serial.println("CERRANDO TAREAS...");
        digitalWrite(ledPin2,LOW);
        digitalWrite(ledPin,LOW);
        finish(); //Función que inicia la grabación

      }
    }
   delay(10);
}


void startReceiving(){
  Serial.println("INICIO DE RECEPCION..."); 
  xTaskCreate(ReceivingTask,"ReceivingTask",1024*2,NULL,1,&taskHandler);
}

void startSending(){
  Serial.println("INICIO DE ENVIO..."); 
  i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  xTaskCreate(SendingTask,"SendingTask",1024*2,NULL,1,&taskHandler);
}

void finish(){
  Serial.println("GRABACION FINALIZADA"); 
  xTaskCreate(finishTask,"finishTask",configMINIMAL_STACK_SIZE,NULL,1,NULL);
}

void SendingTask(void* arg){
  i2s_init_MIC();
  while(true){
     Udp.beginPacket(Udp.remoteIP(), 8081); 
     i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
     i2s_adc_data_scale(flash_write_buff,(uint8_t*)i2s_read_buff,I2S_READ_LEN);
     Udp.write(flash_write_buff,I2S_READ_LEN);
     Udp.endPacket();
  }
  vTaskDelete(NULL);
}

void finishTask(void* arg){
  Serial.println("La grabación ha finalizado");
  if(flash_write_buff!=NULL){
    free(flash_write_buff);
    flash_write_buff=NULL;
  }
  
  if(i2s_read_buff!=NULL){
    free(i2s_read_buff);
    i2s_read_buff=NULL;
  }
  
  vTaskDelete(taskHandler);
  i2s_driver_uninstall(I2S_PORT);
  vTaskDelete(NULL);
}

void ReceivingTask(void* arg){
  char buff_reciv[1024];
  size_t bytes_write=0;
  while(true){
      bytes_write=Udp.read(buff_reciv,sizeof(buff_reciv));      
      if(bytes_write){
        for(int i=0;i<bytes_write;i++){
          dacWrite(25,buff_reciv[i]);
          delayMicroseconds(56);
        }
      }      
  }
  vTaskDelete(NULL);
}



void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 2048;
    }
}


void i2s_init_MIC(){
   i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate =  I2S_SAMPLE_RATE,              
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS), 
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S|I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
    .use_apll = 1,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
   };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

   const i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = -1,
      .data_in_num = I2S_SD
  };
  
  i2s_set_pin(I2S_PORT, &pin_config);
  Serial.println("I2S MICROFONO CONFIGURADO");
}


void WiFiAPEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  if (event == SYSTEM_EVENT_AP_START) {
    Serial.println("AP Started");
  }
  else if (event == SYSTEM_EVENT_AP_STACONNECTED) {
    Serial.println("Client connected");
    digitalWrite(conectionled,HIGH);
  }
  else if (event == SYSTEM_EVENT_AP_STADISCONNECTED) {
    Serial.println("Client disconnected");
    digitalWrite(conectionled,LOW);
  }
}

void open_udp_port(){
  Serial.println();
  Serial.println("Starting UDP");
   if (Udp.begin(localPort) != 1) 
   {
      Serial.println("Connection failed");
      while (true) { delay(1000); } 
   }
   Serial.println("UDP successful");
}

void wifi_init(){
  //MODO PUNTO DE ACCESO  
  Serial.println();
  WiFi.onEvent(WiFiAPEvent, SYSTEM_EVENT_AP_START);
  WiFi.onEvent(WiFiAPEvent, SYSTEM_EVENT_AP_STACONNECTED);
  WiFi.onEvent(WiFiAPEvent, SYSTEM_EVENT_AP_STADISCONNECTED);
  WiFi.mode(WIFI_AP);
  IPAddress Ip(192,168,1,180); 
  IPAddress Gateway(192,168,1,0); 
  IPAddress Subnet(255,255,255,0); 
  //WiFi.softAPConfig(Ip, Gateway, Subnet); 
  while(!WiFi.softAP(ssid,NULL)){
    Serial.print(".");
    delay(100);
  }

  Serial.print("Nombre: ");
  Serial.print(ssid);
  Serial.println();
  Serial.print("IP address: ");
  Serial.print(WiFi.softAPIP());
  
}
