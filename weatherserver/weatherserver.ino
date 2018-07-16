
/*
  An open-source temp server for DS18B20 and BME280
  Source :   https://www.sla99.fr
  Date : 2018-07-15
*/
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include <TrueRandom.h>


//Gestion BME280
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
//double P; // for pressure conversion

//Gestion web server
byte mac[6] = { 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00 };
//byte mac[6] = { 0xBE, 0xEF, 0x00, 0xFD, 0xC7, 0x93 };

char macstr[18];



EthernetServer server(80);

const int LED_NETWORK = A0;
const int LED_CONNECTION = A1;
const int LED_ERROR_BME = A2;
const int LED_OK_BME = A3;
int pin[6]={A0,A1,A2,A3};

void ledAccueil(){
  for (int i = 0 ; i<=5 ; i++){ // boucle for pour allumer les leds une par une
      digitalWrite(pin[i], HIGH);
      delay(200);
    }
    for (int i = 5 ; i>=0 ; i--){ 
      digitalWrite(pin[i], LOW);
      delay(200);
    }
    for(int j=0;j<5;j++){
      for (int i = 0 ; i<=5 ; i++){ // boucle for pour allumer les leds une par une
        digitalWrite(pin[i], HIGH);
      }
      delay(200);
      for (int i = 0 ; i<=5 ; i++){ // boucle for pour allumer les leds une par une
        digitalWrite(pin[i], LOW);
        
      }
      delay(200);
    }
    delay(1000);
}


void setup()
{
  Serial.begin(9600);
  
  pinMode(LED_CONNECTION, OUTPUT);
  pinMode(LED_NETWORK, OUTPUT);
  pinMode(LED_ERROR_BME, OUTPUT);
  pinMode(LED_OK_BME, OUTPUT);
  ledAccueil();
  //gestion de l'adresse mac
  if (EEPROM.read(1) == '#') {
    for (int i = 3; i < 6; i++) {
    mac[i] = EEPROM.read(i);
    }
  } 
  else {
    for (int i = 3; i < 6; i++) {
    mac[i] = TrueRandom.randomByte();
    EEPROM.write(i, mac[i]);
    }
    EEPROM.write(1, '#');
  }
  snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("DHCP : ");
  Serial.println(macstr);
  
  if(!Ethernet.begin(mac)){
    Serial.println("Network failed");
  }
  else{ 
    //Initialisation        
    server.begin();    
    Serial.println(Ethernet.localIP());
    Serial.println(Ethernet.gatewayIP());
    
    digitalWrite(LED_CONNECTION, LOW);    
    digitalWrite(LED_NETWORK, HIGH);    
    digitalWrite(LED_ERROR_BME, HIGH);    
    digitalWrite(LED_OK_BME, LOW);
    delay(3000);
    if (!bme.begin()) {               
      Serial.println("Erreur lecture BME");        
    }
    else{  
      digitalWrite(LED_OK_BME, HIGH);
      digitalWrite(LED_ERROR_BME, LOW);
    }
  }
}

//fonction qui corrige la pression en fonction de l'altitude
double getP(double Pact, double temp, double altitude) {
  return Pact * pow((1 - ((0.0065 * altitude) / (temp + 0.0065 * altitude + 273.15))), -5.257);
}


void loop()
{  
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        digitalWrite(LED_CONNECTION, HIGH);                 
        char c = client.read();
       if (c == '\n' && currentLineIsBlank) {  
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Access-Control-Allow-Origin: *");
          client.println();
          client.print("[{");
            for(int i=2;i<10;i++){
              OneWire oneWire(i);
              DallasTemperature sensors(&oneWire);
              sensors.begin();
              sensors.requestTemperatures(); // Send the command to get temperatures
              client.print("\"TempOneWire");
              client.print(i);
              client.print("\":");
              if(sensors.getTempCByIndex(0) ==-127){
                client.print("\"N/A\"");
              }
              else{
                client.print(sensors.getTempCByIndex(0));
              }
              client.print(",");             
            }

            if (!bme.begin()) {               
              Serial.println("Erreur lecture BME");  
              digitalWrite(LED_OK_BME, LOW);
              digitalWrite(LED_ERROR_BME, HIGH);      
            }
            else{  
              digitalWrite(LED_OK_BME, HIGH);
              digitalWrite(LED_ERROR_BME, LOW);
              double bme280Temp = bme.readTemperature();
              bme280Temp = bme280Temp - 1;
              client.print("\"BME280_Temp\":");
              client.print(bme280Temp);
              client.print(",");
  
              double bme280Hum = bme.readHumidity();
              client.print("\"BME280_Humidity\":");
              client.print(bme280Hum);
              client.print(",");
              
              double bme280Pressure = bme.readPressure() / 100.0F;
              for(int i=0;i<=3000;i+50){
                client.print("\"BME280_Pressure_");
                client.print(i);
                client.print("m\":");
                client.print(getP(bme280Pressure, bme280Temp, i));
                client.print(",");
                i = i+50;
              }
            }
          
            client.print("\"IP\":");
            client.print("\"");
            client.print(Ethernet.localIP());
            client.print("\"");
            client.print("}]");
            delay(1000);
            digitalWrite(LED_CONNECTION, LOW); 
            break;          
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }         
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();   
  }
} 
