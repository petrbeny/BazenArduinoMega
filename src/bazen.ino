#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHT_PIN 2
#define DLS_PIN 3
#define PH_PIN A0

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
  // zapnutí komunikace s teploměrem DHT...
  mojeDHT.begin();
  // zapnutí komunikace knihovny s teplotním čidlem
  senzoryDS.begin();
}

void loop()
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

  // načtení informací ze všech připojených čidel na daném pinu
  senzoryDS.requestTemperatures();
  Print("Teplota bazenu", senzoryDS.getTempCByIndex(0), "°C");

  delay(20000);
}

void Print(String source, int value, String unit)
{
  Serial.print(source + ": ");
  Serial.print(value, DEC);
  Serial.println(unit);
}