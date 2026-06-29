// =============================================================
//  Project SIGMA
//  Arquivo    : TrendAnalysis.h
//  Descricao  : Motor preditivo via Regressao Linear Simples.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.4
//  Codename   : Bugfixes e Limpeza
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include "AnalyticsBuffer.h"

// =============================================================
//  CLASSE: TrendAnalysis
//  Modulo puramente matematico. Metodos estaticos garantem
//  ausencia de estado interno e alta testabilidade.
// =============================================================
class TrendAnalysis {
public:
  static float calculateTemperatureTrend(const AnalyticsBuffer& buffer);
  static float calculateVibrationTrend(const AnalyticsBuffer& buffer);
};