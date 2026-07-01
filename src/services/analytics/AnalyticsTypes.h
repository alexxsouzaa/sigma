// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsTypes.h
//  Descricao  : Estruturas e tipos comuns da Camada Analytics.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.14.0
//  Codename   : Multitarefa UI
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include <stdint.h>

// -------------------------
//  Niveis de Alarme Seguros
// -------------------------
enum class AlarmLevel : uint8_t {
  NORMAL   = 0,
  WARNING  = 1,
  CRITICAL = 2
};

// -------------------------
//  Amostra Base
// -------------------------
struct AnalyticsSample {
  uint32_t   timestamp;   // (ms) Momento da coleta
  float      temperature; // (oC) Temperatura lida
  float      vibration;   // (g)  Vibracao RMS
  float      health;      // (%)  Health Score calculado
  float      runtime;     // (h)  Horimetro acumulado
  AlarmLevel alarm;       // Classificacao tipada
};

// -------------------------
//  Contrato de Saida (API)
// -------------------------
struct AnalyticsResult {
  float meanTemperature;  // (oC) Media da janela
  float meanVibration;    // (g)  Media da janela
  float temperatureTrend; // (Slope) > 0 aquecendo, < 0 esfriando
  float vibrationTrend;   // (Slope) > 0 piorando, < 0 melhorando
  bool  anomalyDetected;  // true se anomalia identificada
};