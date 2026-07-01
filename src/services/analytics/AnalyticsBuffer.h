// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsBuffer.h
//  Descricao  : Servico de armazenamento ciclico de amostras.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.16.0
//  Codename   : Sistema de Alertas
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include <stdint.h>
#include "../../hal/Version.h"
#include "AnalyticsTypes.h" // Importa os contratos (Structs/Enums)

// =============================================================
//  CLASSE: AnalyticsBuffer
//  Buffer circular FIFO restrito a memoria estatica.
// =============================================================
class AnalyticsBuffer {
private:
  AnalyticsSample _buffer[ANALYTICS_BUFFER_SIZE];
  int _head;  // Indice de insercao
  int _tail;  // Indice do dado mais antigo
  int _count; // Total de itens atualmente armazenados

public:
  AnalyticsBuffer();

  void addSample(const AnalyticsSample& sample);
  void clear();
  
  int  size() const;
  int  capacity() const;
  bool isFull() const;
  bool isEmpty() const;
  
  bool get(int index, AnalyticsSample& outSample) const;
  bool latest(AnalyticsSample& outSample) const;
};