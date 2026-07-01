// =============================================================
//  Project SIGMA
//  Arquivo    : EventHistory.h
//  Descricao  : Historico circular de eventos em RAM.
//               (T015 — Historico de Eventos, Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.17.0
//  Codename   : Boot Diagnostics
//  Data       : 2026-06-30
// =============================================================

#pragma once

#include <stdint.h>
#include "EventTypes.h"

// -------------------------
//  Registro do Historico
// -------------------------
struct EventHistoryRecord {
  uint32_t   timestamp;   // ms desde boot
  EventType  tipo;
  EventoDados dados;
};

// =============================================================
//  CLASSE: EventHistory
//  Buffer circular de eventos em RAM (sem alocacao dinamica).
//  Uso: chamar registrar() no callback do EventManager,
//       depois consultar com obter() ou imprimir().
// =============================================================
class EventHistory {
public:
  static const uint8_t MAX_REGISTROS = 40;

  EventHistory();

  void registrar(uint32_t timestamp, EventType tipo,
                 const EventoDados& dados);
  bool obter(uint8_t indice, EventHistoryRecord& record) const;
  uint8_t quantidade() const;
  void limpar();
  void imprimir() const;

private:
  EventHistoryRecord _buffer[MAX_REGISTROS];
  uint8_t _cabeca;   // indice da proxima escrita
  uint8_t _count;    // total de registros acumulados
};
