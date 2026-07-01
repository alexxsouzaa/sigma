// =============================================================
//  Project SIGMA
//  Arquivo    : EventManager.h
//  Descricao  : Sistema de eventos internos pub/sub.
//               (T014 — Event Manager, Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.14.0
//  Codename   : Multitarefa UI
//  Data       : 2026-06-30
// =============================================================

#pragma once

#include <stdint.h>
#include "EventTypes.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// -------------------------
//  Callback de Evento
// -------------------------
typedef void (*EventCallback)(EventType tipo, const EventoDados& dados);

// =============================================================
//  CLASSE: EventManager
//  Implementa um barramento pub/sup estatico sem alocacao
//  dinamica. Suporta disparo sincrono (disparar) e
//  assincrono (enfileirar + processarFila).
//  Uso: inscrever() no setup(), disparar() no loop().
// =============================================================
class EventManager {
public:
  static const uint8_t MAX_ASSINANTES   = 16;
  static const uint8_t MAX_EVENTOS_FILA = 8;

  EventManager();

  // Inscreve um callback para um tipo de evento.
  // Retorna true se conseguiu, false se limite atingido.
  bool inscrever(EventType tipo, EventCallback callback);

  // Dispara evento sincrono — chama todos inscritos
  // imediatamente.
  void disparar(EventType tipo, const EventoDados& dados);

  // Enfileira evento para processamento posterior
  // via processarFila(). Retorna true se enfileirou.
  bool enfileirar(EventType tipo, const EventoDados& dados);

  // Processa todos os eventos enfileirados.
  void processarFila();

  // Remove todos inscritos e limpa fila.
  void reset();

private:
  struct Assinante {
    EventType      tipo;
    EventCallback  callback;
    bool           ativo;
  };

  struct EventoFila {
    EventType  tipo;
    EventoDados dados;
    bool       ocupado;
  };

  Assinante   _assinantes[MAX_ASSINANTES];
  EventoFila  _fila[MAX_EVENTOS_FILA];
  uint8_t     _numAssinantes;
  SemaphoreHandle_t _filaMutex;
};
