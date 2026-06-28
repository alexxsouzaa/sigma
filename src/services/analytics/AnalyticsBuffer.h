// =============================================================
//  Project SIGMA
//  Arquivo    : AnalyticsBuffer.h
//  Descricao  : Servico de armazenamento ciclico de amostras.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.0
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include <stdint.h>
#include "../../hal/Version.h"

// -------------------------
//  Estrutura de dados base
// -------------------------
struct AnalyticsSample {
  uint32_t timestamp;   // (ms) Momento da coleta
  float    temperature; // (oC) Temperatura lida
  float    vibration;   // (g)  Vibracao RMS
  float    health;      // (%)  Health Score calculado
  float    runtime;     // (h)  Horimetro acumulado
  uint8_t  alarm;       // 0=NORMAL, 1=AVISO, 2=CRITICO
};

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