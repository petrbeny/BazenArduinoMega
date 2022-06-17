#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// piny 4 a 10 nepouzivat, pac je pouziva ethernet schiled kvuli komunikaci s arduino deskou!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define DHT_PIN 6
#define DLS_PIN 3
#define PIR_PIN 2 // PIR je na 2 pinu kvuli podpore ineruptu
#define PH_PIN A8

// Arduino MAC address must be unique for every node in same network
// To make a new unique address change last letter
// Arduino 0
byte mac[] = {0xCC, 0xFA, 0x06, 0xCB, 0x19, 0x02};

// Unique static IP address of this Arduino
IPAddress ip(192, 168, 200, 51);

// IP Address of your MQTT broker (OpenHAB server)
byte server[] = {192, 168, 200, 123};

EthernetClient arduinoBazen;
void callback(char *topic, byte *payload, unsigned int length);
PubSubClient client(server, 1883, callback, arduinoBazen);

#define DHT22_TYPE DHT22

// inicializace DHT senzoru s nastaveným pinem a typem senzoru
DHT mojeDHT(DHT_PIN, DHT22_TYPE);

// vytvoření instance oneWireDS z knihovny OneWire
OneWire oneWireDS(DLS_PIN);
// vytvoření instance senzoryDS z knihovny DallasTemperature
DallasTemperature senzoryDS(&oneWireDS);

void setup()
{
  // Nastaveni logovani
  Serial.begin(9600);

  // Setup ethernet connection to MQTT broker
  Ethernet.begin(mac, ip);
  // change as desired - clientname must be unique for MQTT broker
  if (client.connect("arduinoBazen", "openhabian", "4Mamf6we"))
  {
    client.publish("/outside/arduinoBazen/connect", "hello world - here arduinoBazen ip 192.168.200.51");
    Serial.println("connected");
    // subscribe to topics
    client.subscribe("/house/downstairs/loznice/co2sensor/calibration");
  }
  else
  {
    Serial.println("MQTT client not connected!");
  }

  // výpis informace o nastavené IP adrese
  Serial.print("Server je na IP adrese: ");
  Serial.println(Ethernet.localIP());

  // zapnutí komunikace s teploměrem DHT...
  mojeDHT.begin();

  // zapnutí komunikace knihovny s teplotním čidlem
  senzoryDS.begin();

  // PIR
  pinMode(PIR_PIN, INPUT);

  // nastavení přerušení na pin 6 (int0)
  // při rostoucí hraně (logO->log1) se vykoná program prerus
  // attachInterrupt(digitalPinToInterrupt(PIR_PIN), detection, RISING);
}

long lastReconnectAttempt = 0;
boolean reconnect()
{
  if (client.connect("arduinoMegaOriginal1", "openhabian", "4Mamf6we"))
  {    
    // Once connected, publish an announcement...
    client.publish("/outside/gate/lock", "reconnected");
    // ... and resubscribe
    client.subscribe("/outside/gate/lock");
    client.subscribe("/house/downstairs/loznice/co2sensor/calibration");
  }

  return client.connected();
}

void loop()
{
  // --- MQTT client handling -------------------------------------------
  if (!client.connected())
  {
    Serial.println("MQTT arduino client disconnected");
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect())
      {
        lastReconnectAttempt = 0;
        Serial. println("reconnected");
      }
      else
      {
        Serial.println("reconnection failed");
      }
    }
  }
  else
  {
    Serial.println("MQTT arduino client connected");
    // Client connected
    client.loop();
  }

  // --- Venkovni teplota a vlhkost ---
  // ReadDht();

  // --- Teplota bazenu ---------------
  // ReadDallas();

  // --- pH bazenu --------------------
  ReadPh();

  //--- Sepnout rele ---------------------------------------------------
  // Sepne filtraci
  // closeRelay(RELE_FILTRACE_PIN);
  // Sepne cerpadlo
  // closeRelay(RELE_CERPADLO_PIN);

  delay(10000);
}

void Print(String source, float value, String unit)
{
  Serial.print(source + ": ");
  Serial.print(value, 2);
  Serial.println(unit);
}

void ReadDht()
{
  // nacteni teploty a vlhkosti s DHT senzoru
  float tep = mojeDHT.readTemperature();
  float vlh = mojeDHT.readHumidity();
  // kontrola, jestli jsou načtené hodnoty čísla pomocí funkce isnan
  if (isnan(tep) || isnan(vlh))
  {
    // při chybném čtení vypiš hlášku
    Serial.println("Chyba při čtení z DHT senzoru!");
  }
  else
  {
    // pokud jsou hodnoty v pořádku,
    // vypiš je po sériové lince
    Print("Venkovni teplota", tep, "°C");
    Print("Venkovni vlhkost", vlh, "%");
    // client.publish("/house/downstairs/komora/thermometer/temperature", String(tep, 3).c_str());
    // client.publish("/house/downstairs/komora/thermometer/humidity", String(vlh, 3).c_str());
  }
}

void ReadDallas()
{
  // načtení informací ze všech připojených čidel na daném pinu
  senzoryDS.requestTemperatures();
  Print("Teplota bazenu", senzoryDS.getTempCByIndex(0), "°C");
}

void ReadPh()
{
  // vytvoření pomocných proměnných
  int pole[10];
  int zaloha;
  unsigned long int prumerVysl = 0;
  // načtení deseti vzorků po 10 ms do pole
  for (int i = 0; i < 10; i++)
  {
    pole[i] = analogRead(PH_PIN);
    delay(10);
  }
  // seřazení členů pole naměřených výsledků podle velikosti
  for (int i = 0; i < 9; i++)
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (pole[i] > pole[j])
      {
        zaloha = pole[i];
        pole[i] = pole[j];
        pole[j] = zaloha;
      }
    }
  }
  // uložení 2. až 8. výsledku do
  // proměnné, z které se vypočte průměr
  // (vynechání dvou členů pole na začátku
  // a konci pro lepší přesnost)
  for (int i = 2; i < 8; i++)
  {
    prumerVysl += pole[i];
  }
  // výpočet hodnoty pH z průměru
  // měření a přepočet na rozsah 0-14 pH
  float prumerPH = (float)prumerVysl * 5.0 / 1024 / 6;
  float vyslednePH = -5.70 * prumerPH + 21.34;
  // vytištění výsledků po sériové lince
  Print("PH bazenu", vyslednePH, "pH");
}

void closeRelay(int relePin)
{
  // Serial.println("close relay");
  //  sepne rele
  digitalWrite(relePin, HIGH);
  delay(2000);
  // Serial.println("open relay");
  digitalWrite(relePin, LOW);
  // Serial.println(client.connected());
  // client.publish("/outside/gate/lock", "0");
}

void detection()
{
  // pokud je aktivován digitální vstup,
  // vypiš informaci po sériové lince
  Serial.println("Detekce pohybu pomoci HC-SR501!");
}

// Handle and convert incoming MQTT messages ----------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
  // Print the topic
  Serial.print("Topic: ");
  Serial.println(topic);

  // handle message arrived
  String content = "";
  char character;
  for (int num = 0; num < length; num++)
  {
    character = payload[num];
    content.concat(character);
  }

  String t = topic;

  Serial.println(t);

  /*
  if (t == "/outside/gate/lock" && content == "1")
  {
    closeRelay();
  }

  if (t == "/house/downstairs/loznice/co2sensor/calibration" && content == "1")
  {
    CalibrateCo2Senzors();
  }
  */
}
