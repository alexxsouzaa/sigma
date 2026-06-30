// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsEngine.h
//  Descricao  : Motor analitico. Ponto unico de acesso.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.9.0
//  Codename   : Qualidade dos Sensores
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include "AnalyticsTypes.h"
#include "AnalyticsBuffer.h"

// =============================================================
//  CLASSE: AnalyticsEngine
//  Orquestra armazenamento e calculos (estado proprio).
// =============================================================
class AnalyticsEngine {
private:
  AnalyticsBuffer _buffer;
  AnalyticsResult _lastResult;

  void atualizarEstatisticas();

public:
  AnalyticsEngine();

  void processSample(const AnalyticsSample& sample);
  void clear();

  AnalyticsResult getLatestResult() const;
};