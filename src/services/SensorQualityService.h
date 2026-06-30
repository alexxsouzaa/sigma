// =============================================================
//  Project SIGMA
//  Arquivo    : SensorQualityService.h
//  Descricao  : Indice de confianca das leituras dos sensores
//               (T013 — Qualidade dos Sensores, Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.12.0
//  Codename   : Multitarefa SENSOR
//  Data       : 2026-06-30
// =============================================================

#pragma once

#include <stdint.h>

// -------------------------
//  Relatorio de Qualidade
// -------------------------
struct SensorQualityData {
  uint8_t  confianca;       // 0-100%
  uint16_t falhasConsec;    // falhas consecutivas
};

struct SensorQualityReport {
  SensorQualityData temp;
  SensorQualityData vib;
};

// =============================================================
//  CLASSE: SensorQualityService
//  Avalia a confianca de cada leitura dos sensores com base
//  em falhas consecutivas e periodo de bootstrapping.
//  Uma unica instancia atende ambos os sensores.
//  Uso: atualizarTemp/Vib() + obterRelatorio() no loop.
// =============================================================
class SensorQualityService {
public:
  static const uint16_t MAX_FALHAS = 5;
  static const uint8_t  JANELA_BOOT = 10;

  SensorQualityService();

  void atualizarTemp(bool dadoValido);
  void atualizarVib(bool dadoValido);
  SensorQualityReport obterRelatorio() const;
  const char* obterStatus(uint8_t confianca) const;
  void reset();

private:
  uint16_t _tempFalhas;
  uint8_t  _tempN;
  uint16_t _vibFalhas;
  uint8_t  _vibN;
};
