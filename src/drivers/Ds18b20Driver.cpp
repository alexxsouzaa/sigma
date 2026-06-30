// =============================================================
//  Project SIGMA
//  Arquivo    : Ds18b20Driver.cpp
//  Descricao  : Implementacao do driver DS18B20.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.6
//  Codename   : Pausa Monitoramento Calibracao
//  Data       : 2026-06-27
// =============================================================

#include "Ds18b20Driver.h"

// =============================================================
//  CONSTRUTOR / DESTRUTOR
// =============================================================
Ds18b20Driver::Ds18b20Driver() {
  _oneWire = nullptr;
  _sensor  = nullptr;
}

Ds18b20Driver::~Ds18b20Driver() {
  if (_sensor)  delete _sensor;
  if (_oneWire) delete _oneWire;
}

// =============================================================
//  FUNCAO: begin
//  Inicializa barramento 1-Wire e objeto DallasTemperature.
// -------------------------------------------------------------
//  Parametros:
//    pino : GPIO onde o sensor esta conectado.
//  Retorno  : true se ao menos 1 sensor for detectado.
// =============================================================
bool Ds18b20Driver::begin(uint8_t pino) {
  // Alocacao em boot-time e aceitavel. Nunca em runtime.
  _oneWire = new OneWire(pino);
  if (!_oneWire) return false;

  _sensor  = new DallasTemperature(_oneWire);
  if (!_sensor) { delete _oneWire; _oneWire = nullptr; return false; }

  _sensor->begin();
  int qtd = _sensor->getDeviceCount();
  
  if (qtd > 0) {
    _sensor->setResolution(12);
    return true;
  }
  return false;
}

// =============================================================
//  FUNCAO: lerTemperatura
//  Solicita conversao termica e verifica integridade do dado.
// -------------------------------------------------------------
//  Retorno  : Struct Ds18b20Data com flag de validade.
//  Observ.  : Retorna valido = false se desconectado (-127).
// =============================================================
Ds18b20Data Ds18b20Driver::lerTemperatura() {
  Ds18b20Data dado = { 0.0f, false };
  
  if (!_sensor) return dado;

  _sensor->requestTemperatures();
  float leitura = _sensor->getTempCByIndex(0);
  
  // DEVICE_DISCONNECTED_C indica falha fisica na comunicacao
  if (leitura != DEVICE_DISCONNECTED_C) {
    dado.temperaturaCelsius = leitura;
    dado.valido = true;
  }
  
  return dado;
}