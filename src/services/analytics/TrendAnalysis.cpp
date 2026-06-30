// =============================================================
//  Project SIGMA
//  Arquivo    : TrendAnalysis.cpp
//  Descricao  : Implementacao do Metodo dos Minimos Quadrados.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.5
//  Codename   : Calibracao Nao-Bloqueante Fix
//  Data       : 2026-06-28
// =============================================================

#include "TrendAnalysis.h"

// =============================================================
//  FUNCAO: calculateTemperatureTrend
//  Calcula o coeficiente angular (slope) da temperatura.
// -------------------------------------------------------------
//  Retorno  : > 0 (aquecimento), < 0 (resfriamento), 0 (estavel)
// =============================================================
float TrendAnalysis::calculateTemperatureTrend(const AnalyticsBuffer& buffer) {
  int n = buffer.size();
  if (n < 2) return 0.0f; // Impossivel extrair tendencia com < 2 pontos

  float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
  AnalyticsSample sample;

  // Iteracao linear O(N) para calculo da regressao
  for (int i = 0; i < n; i++) {
    if (buffer.get(i, sample)) {
      sumX  += i;
      sumY  += sample.temperature;
      sumXY += (i * sample.temperature);
      sumX2 += (i * i);
    }
  }

  float denominator = (n * sumX2) - (sumX * sumX);
  if (denominator == 0.0f) return 0.0f;

  return ((n * sumXY) - (sumX * sumY)) / denominator;
}

// =============================================================
//  FUNCAO: calculateVibrationTrend
//  Calcula o coeficiente angular (slope) da vibracao RMS.
// -------------------------------------------------------------
//  Retorno  : > 0 (piora mecânica), < 0 (melhora), 0 (estavel)
// =============================================================
float TrendAnalysis::calculateVibrationTrend(const AnalyticsBuffer& buffer) {
  int n = buffer.size();
  if (n < 2) return 0.0f;

  float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
  AnalyticsSample sample;

  for (int i = 0; i < n; i++) {
    if (buffer.get(i, sample)) {
      sumX  += i;
      sumY  += sample.vibration;
      sumXY += (i * sample.vibration);
      sumX2 += (i * i);
    }
  }

  float denominator = (n * sumX2) - (sumX * sumX);
  if (denominator == 0.0f) return 0.0f;

  return ((n * sumXY) - (sumX * sumY)) / denominator;
}