// =============================================================
//  Project SIGMA
//  Arquivo    : EventManager.cpp
//  Descricao  : Sistema de eventos internos pub/sub.
//               (T014 — Event Manager, Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.15.0
//  Codename   : ProcessingTask
//  Data       : 2026-06-30
// =============================================================

#include "EventManager.h"

// =============================================================
//  CONSTRUTOR
// =============================================================
EventManager::EventManager() {
  _numAssinantes = 0;
  for (uint8_t i = 0; i < MAX_ASSINANTES; i++) {
    _assinantes[i].ativo = false;
    _assinantes[i].callback = nullptr;
  }
  for (uint8_t i = 0; i < MAX_EVENTOS_FILA; i++) {
    _fila[i].ocupado = false;
  }
  _filaMutex = xSemaphoreCreateMutex();
}

// =============================================================
//  FUNCAO: inscrever
//  Registra um callback para um tipo de evento.
// =============================================================
bool EventManager::inscrever(EventType tipo, EventCallback callback) {
  if (_numAssinantes >= MAX_ASSINANTES) return false;
  if (callback == nullptr) return false;
  _assinantes[_numAssinantes].tipo     = tipo;
  _assinantes[_numAssinantes].callback = callback;
  _assinantes[_numAssinantes].ativo    = true;
  _numAssinantes++;
  return true;
}

// =============================================================
//  FUNCAO: disparar
//  Dispara evento sincrono — percorre todos assinantes
//  e chama callbacks com tipo correspondente.
// =============================================================
void EventManager::disparar(EventType tipo, const EventoDados& dados) {
  for (uint8_t i = 0; i < MAX_ASSINANTES; i++) {
    if (!_assinantes[i].ativo) continue;
    if (_assinantes[i].tipo != tipo) continue;
    _assinantes[i].callback(tipo, dados);
  }
}

// =============================================================
//  FUNCAO: enfileirar
//  Enfileira evento para processamento posterior.
// =============================================================
bool EventManager::enfileirar(EventType tipo, const EventoDados& dados) {
  bool ok = false;
  if (xSemaphoreTake(_filaMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (uint8_t i = 0; i < MAX_EVENTOS_FILA; i++) {
      if (_fila[i].ocupado) continue;
      _fila[i].tipo   = tipo;
      _fila[i].dados  = dados;
      _fila[i].ocupado = true;
      ok = true;
      break;
    }
    xSemaphoreGive(_filaMutex);
  }
  return ok;
}

// =============================================================
//  FUNCAO: processarFila
//  Processa todos os eventos enfileirados em ordem FIFO.
// =============================================================
void EventManager::processarFila() {
  // Snapshot da fila sob mutex para minimizar o tempo
  // de bloqueio — a iteracao chama callbacks que podem
  // levar mais tempo que o toleravel para o produtor.
  EventoFila snap[MAX_EVENTOS_FILA];
  uint8_t n = 0;

  if (xSemaphoreTake(_filaMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    for (uint8_t i = 0; i < MAX_EVENTOS_FILA; i++) {
      if (!_fila[i].ocupado) continue;
      snap[n] = _fila[i];
      _fila[i].ocupado = false;
      n++;
    }
    xSemaphoreGive(_filaMutex);
  }

  for (uint8_t i = 0; i < n; i++) {
    disparar(snap[i].tipo, snap[i].dados);
  }
}

// =============================================================
//  FUNCAO: reset
//  Remove todos inscritos e limpa fila.
// =============================================================
void EventManager::reset() {
  _numAssinantes = 0;
  for (uint8_t i = 0; i < MAX_ASSINANTES; i++) {
    _assinantes[i].ativo = false;
    _assinantes[i].callback = nullptr;
  }
  for (uint8_t i = 0; i < MAX_EVENTOS_FILA; i++) {
    _fila[i].ocupado = false;
  }
}
