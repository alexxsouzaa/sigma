// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsEngine.cpp
//  Descricao  : Implementacao do motor analitico de borda.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.1
//  Data       : 2026-06-28
// =============================================================

#include "AnalyticsEngine.h"

// =============================================================
//  CONSTRUTOR
// =============================================================
AnalyticsEngine::AnalyticsEngine() {
  _lastResult = { 0.0f, 0.0f, false };
}

// =============================================================
//  FUNCAO: atualizarEstatisticas
//  Executa logica matematica basica apos insercao.
// -------------------------------------------------------------
//  Observ.  : Na proxima Sprint, esta funcao chamara o modulo
//             Statistics isolado para processamento avancado.
// =============================================================
void AnalyticsEngine::atualizarEstatisticas() {
  // TODO [v0.1.6.2] : Integrar calculo real via modulo Statistics
  // Temporario: apenas copia o ultimo dado para manter a API
  AnalyticsSample ultima;
  if (_buffer.latest(ultima)) {
    _lastResult.meanTemperature = ultima.temperature;
    _lastResult.meanVibration   = ultima.vibration;
    _lastResult.anomalyDetected = (ultima.alarm != AlarmLevel::NORMAL);
  }
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
//  Retorna os calculos estaticos processados pelo motor.
// =============================================================
AnalyticsResult AnalyticsEngine::getLatestResult() const {
  return _lastResult;
}