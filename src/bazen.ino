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
  // --- DHT senzor -------------------------------------------------------------------------------------------
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

  // --- Dallas senzor -----------------------------------------------------------------------------------------
  // načtení informací ze všech připojených čidel na daném pinu
  senzoryDS.requestTemperatures();
  Print("Teplota bazenu", senzoryDS.getTempCByIndex(0), "°C");

  // --- PH senzor ---------------------------------------------------------------------------------------------
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
  // Print("PH bazenu", vyslednePH, "pH");
  Serial.print("Namerene pH: ");
  Serial.println(vyslednePH);

  delay(900);
}

void Print(String source, int value, String unit)
{
  Serial.print(source + ": ");
  Serial.print(value, DEC);
  Serial.println(unit);
}