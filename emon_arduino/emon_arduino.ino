#include "EmonLib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Timer.h"
#include <SoftwareSerial.h>

#define LED_PIN 13
#define PHASE_SEQ_PIN 0
#define READVCC_CALIBRATION_CONST 1180000L
#define CALC_ENERGY_INTERVAL 5 // in secs

Timer timer;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature DS18B20(&oneWire);

EnergyMonitor emon1;             // Create an instance
EnergyMonitor emon2;             // Create an instance
EnergyMonitor emon3;             // Create an instance

SoftwareSerial ESPserial(11, 12); // RX, TX

int P1_realPower;
float P1_powerFactor;
int P1_supplyVoltage;
float P1_Irms;

int P2_realPower;
float P2_powerFactor;
int P2_supplyVoltage;
float P2_Irms;

int P3_realPower;
float P3_powerFactor;
int P3_supplyVoltage;
float P3_Irms;

float temp;

float P1_energy;
float P2_energy;
float P3_energy;
int error;

void emonget() {
  DS18B20.requestTemperatures(); // Send the command to get temperatures
  temp = DS18B20.getTempCByIndex(0);
  DS18B20.setWaitForConversion(false);
  DS18B20.requestTemperatures();

  emon1.calcVI(20, 2000);        // Calculate all. No.of half wavelengths (crossings), time-out
  emon2.calcVI(20, 2000);        // Calculate all. No.of half wavelengths (crossings), time-out
  emon3.calcVI(20, 2000);        // Calculate all. No.of half wavelengths (crossings), time-out

  P1_realPower       = emon1.realPower;        //extract Real Power in W into variable
  P1_powerFactor     = emon1.powerFactor;      //extract Power Factor into Variable
  P1_supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  P1_Irms            = emon1.Irms;             //extract Irms into Variable

  P2_realPower       = emon2.realPower;        //extract Real Power in W into variable
  P2_powerFactor     = emon2.powerFactor;      //extract Power Factor into Variable
  P2_supplyVoltage   = emon2.Vrms;             //extract Vrms into Variable
  P2_Irms            = emon2.Irms;             //extract Irms into Variable

  P3_realPower       = emon3.realPower;        //extract Real Power in W into variable
  P3_powerFactor     = emon3.powerFactor;      //extract Power Factor into Variable
  P3_supplyVoltage   = emon3.Vrms;             //extract Vrms into Variable
  P3_Irms            = emon3.Irms;             //extract Irms into Variable
}

void emonsendserial() {
  Serial.print(F("Ln"));
  Serial.print('\t');
  Serial.print(F("V"));
  Serial.print('\t');
  Serial.print(F("C"));
  Serial.print('\t');
  Serial.print(F("PF"));
  Serial.print('\t');
  Serial.print(F("P"));
  Serial.print('\t');
  Serial.println(F("E"));

  Serial.print(F("L1:     "));
  Serial.print(P1_supplyVoltage);
  Serial.print('\t');
  Serial.print(P1_Irms);
  Serial.print('\t');
  Serial.print(P1_powerFactor);
  Serial.print('\t');
  Serial.print(P1_realPower);
  Serial.print('\t');
  Serial.println(P1_energy, 2);

  Serial.print(F("L2:     "));
  Serial.print(P2_supplyVoltage);
  Serial.print('\t');
  Serial.print(P2_Irms);
  Serial.print('\t');
  Serial.print(P2_powerFactor);
  Serial.print('\t');
  Serial.print(P2_realPower);
  Serial.print('\t');
  Serial.println(P2_energy, 2);

  Serial.print(F("L3:     "));
  Serial.print(P3_supplyVoltage);
  Serial.print('\t');
  Serial.print(P3_Irms);
  Serial.print('\t');
  Serial.print(P3_powerFactor);
  Serial.print('\t');
  Serial.print(P3_realPower);
  Serial.print('\t');
  Serial.println(P3_energy, 2);
  Serial.println();

  Serial.print(F("TEMP: "));
  Serial.print(temp);
  Serial.print('\t');
  Serial.print(F("PHASE SEQ. ERROR: "));
  Serial.println(error);

  Serial.println();
}

void emonsendtoesp() {
  digitalWrite(LED_PIN, HIGH);
  char dataarray[32];
  sprintf(dataarray, ">1,%d,%d,%d,%d,%ld", P1_supplyVoltage, (int)(P1_Irms * 10), (int)(P1_powerFactor * 100), P1_realPower, (long)(P1_energy * 100));
  ESPserial.println(dataarray);
  delay(150);
  sprintf(dataarray, ">2,%d,%d,%d,%d,%ld", P2_supplyVoltage, (int)(P2_Irms * 10), (int)(P2_powerFactor * 100), P2_realPower, (long)(P2_energy * 100));
  ESPserial.println(dataarray);
  delay(150);
  sprintf(dataarray, ">3,%d,%d,%d,%d,%ld", P3_supplyVoltage, (int)(P3_Irms * 10), (int)(P3_powerFactor * 100), P3_realPower, (long)(P3_energy * 100));
  ESPserial.println(dataarray);
  delay(150);
  sprintf(dataarray, ">4,%d,%d", (int)(temp * 10), error);
  ESPserial.println(dataarray);
  delay(150);
  digitalWrite(LED_PIN, LOW);
}

void calcenergy() {
  P1_energy += ((float)P1_realPower * (float)CALC_ENERGY_INTERVAL / 3600) / 1000; //in kWh
  P2_energy += ((float)P2_realPower * (float)CALC_ENERGY_INTERVAL / 3600) / 1000; //in kWh
  P3_energy += ((float)P3_realPower * (float)CALC_ENERGY_INTERVAL / 3600) / 1000; //in kWh
}

void setup() {
  Serial.begin(115200);
  ESPserial.begin(9600);
  pinMode(PHASE_SEQ_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  emon1.voltage(5, 256.2, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon1.current(1, 62.59);       // Current: input pin, calibration.
  emon2.voltage(2, 250.9, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon2.current(3, 60.25);       // Current: input pin, calibration.
  emon3.voltage(0, 255.9, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon3.current(4, 61.66);       // Current: input pin, calibration.
  DS18B20.begin();
  DS18B20.setResolution(12);
  DS18B20.setWaitForConversion(false);
  DS18B20.requestTemperatures();
  timer.every(CALC_ENERGY_INTERVAL * 1000, calcenergy);
  delay(5000);
}

void loop() {
  timer.update();
  emonget();
  emonsendserial();
  emonsendtoesp();
  if (!digitalRead(PHASE_SEQ_PIN)) {
    error = 1;
  }
  else {
    error = 0;
  }
}
