// =============================================================
//  Project SIGMA
//  Arquivo    : Ds18b20Driver.h
//  Descricao  : Driver de encapsulamento do sensor DS18B20.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.0
//  Data       : 2026-06-27
// =============================================================

#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

// -------------------------
//  Retorno de dados
// -------------------------
struct Ds18b20Data {
  float temperaturaCelsius; // (oC) Valor termico lido
  bool  valido;             // true se leitura OK
};

class Ds18b20Driver {
private:
  OneWire* _oneWire;
  DallasTemperature* _sensor;

public:
  Ds18b20Driver();
  ~Ds18b20Driver();

  bool        begin(uint8_t pino);
  Ds18b20Data lerTemperatura();
};