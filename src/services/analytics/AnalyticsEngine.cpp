// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsEngine.cpp
//  Descricao  : Implementacao do motor analitico de borda.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.13.0
//  Codename   : Multitarefa EVENT
//  Data       : 2026-06-28
// =============================================================

#include "AnalyticsEngine.h"
#include "TrendAnalysis.h"

// =============================================================
//  CONSTRUTOR
// =============================================================
AnalyticsEngine::AnalyticsEngine() {
  _lastResult = { 0.0f, 0.0f, 0.0f, 0.0f, false };
}

// =============================================================
//  FUNCAO: atualizarEstatisticas
//  Recalcula medias e anomalia percorrendo todo o buffer.
//  Executado a cada nova amostra — O(N) sobre ~300 itens e
//  aceitavel dado o intervalo de 500ms entre ciclos.
// =============================================================
void AnalyticsEngine::atualizarEstatisticas() {
  int n = _buffer.size();
  if (n == 0) return;

  float somaTemp = 0.0f;
  float somaVib  = 0.0f;
  int   alarmes  = 0;
  AnalyticsSample amostra;

  for (int i = 0; i < n; i++) {
    if (_buffer.get(i, amostra)) {
      somaTemp += amostra.temperature;
      somaVib  += amostra.vibration;
      if (amostra.alarm != AlarmLevel::NORMAL) alarmes++;
    }
  }

  _lastResult.meanTemperature = somaTemp / n;
  _lastResult.meanVibration   = somaVib / n;
  _lastResult.anomalyDetected = (alarmes > 0);
}

// =============================================================
//  FUNCAO: processSample
//  Recebe amostra, guarda no buffer e atualiza estado interno.
// =============================================================
void AnalyticsEngine::processSample(const AnalyticsSample& sample) {
  _buffer.addSample(sample);
  atualizarEstatisticas();
}

// =============================================================
//  FUNCAO: clear
//  Limpa o buffer interno e reseta o ultimo resultado.
// =============================================================
void AnalyticsEngine::clear() {
  _buffer.clear();
  _lastResult = { 0.0f, 0.0f, false };
}

// =============================================================
//  FUNCAO: getLatestResult
//  Retorna os calculos estaticos e processa as tendencias
//  sob demanda (Lazy Evaluation) para poupar CPU.
// =============================================================
AnalyticsResult AnalyticsEngine::getLatestResult() const {
  AnalyticsResult result = _lastResult;
  
  // O calculo de tendencia O(N) ocorre apenas no momento da requisicao
  result.temperatureTrend = TrendAnalysis::calculateTemperatureTrend(_buffer);
  result.vibrationTrend   = TrendAnalysis::calculateVibrationTrend(_buffer);
  
  return result;
}