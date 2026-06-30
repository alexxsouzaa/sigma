// =============================================================
//  Project SIGMA
//  Arquivo    : EventHistory.cpp
//  Descricao  : Historico circular de eventos em RAM.
//               (T015 — Historico de Eventos, Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.12.0
//  Codename   : Multitarefa SENSOR
//  Data       : 2026-06-30
// =============================================================

#include "EventHistory.h"
#include <Arduino.h>

// =============================================================
//  CONSTRUTOR
// =============================================================
EventHistory::EventHistory() {
  _cabeca = 0;
  _count  = 0;
}

// =============================================================
//  FUNCAO: registrar
//  Adiciona um registro ao buffer circular.
// =============================================================
void EventHistory::registrar(uint32_t timestamp, EventType tipo,
                             const EventoDados& dados) {
  _buffer[_cabeca].timestamp = timestamp;
  _buffer[_cabeca].tipo      = tipo;
  _buffer[_cabeca].dados     = dados;
  _cabeca++;
  if (_cabeca >= MAX_REGISTROS) _cabeca = 0;
  if (_count < MAX_REGISTROS) _count++;
}

// =============================================================
//  FUNCAO: obter
//  Retorna o i-esimo registro mais antigo disponivel.
//  indice 0 = mais antigo, indice count-1 = mais recente.
// =============================================================
bool EventHistory::obter(uint8_t indice, EventHistoryRecord& record) const {
  if (indice >= _count) return false;
  uint8_t pos;
  if (_count < MAX_REGISTROS) {
    pos = indice;
  } else {
    pos = (_cabeca + indice) % MAX_REGISTROS;
  }
  record = _buffer[pos];
  return true;
}

// =============================================================
//  FUNCAO: quantidade
//  Numero de registros atualmente no buffer.
// =============================================================
uint8_t EventHistory::quantidade() const {
  return _count;
}

// =============================================================
//  FUNCAO: limpar
//  Zera todo o historico.
// =============================================================
void EventHistory::limpar() {
  _cabeca = 0;
  _count  = 0;
}

// =============================================================
//  FUNCAO: imprimir
//  Exibe todos os registros do historico via Serial.
// =============================================================
void EventHistory::imprimir() const {
  Serial.println(F("  --- Historico de Eventos ---"));
  if (_count == 0) {
    Serial.println(F("  (vazio)"));
    return;
  }
  uint8_t inicio = _count;
  uint8_t total  = _count;
  for (uint8_t i = 0; i < total; i++) {
    uint8_t pos;
    if (_count < MAX_REGISTROS) {
      pos = i;
    } else {
      pos = (_cabeca + i) % MAX_REGISTROS;
    }
    Serial.print(F("  #"));
    if (i < 10) Serial.print(F("0"));
    Serial.print(i);
    Serial.print(F(" T="));
    Serial.print(_buffer[pos].timestamp);
    Serial.print(F(" tipo="));
    Serial.print((uint8_t)_buffer[pos].tipo);
    Serial.print(F(" v1="));
    Serial.print(_buffer[pos].dados.valorPrincipal, 2);
    Serial.print(F(" v2="));
    Serial.print(_buffer[pos].dados.valorSecundario, 2);
    Serial.print(F(" org="));
    Serial.print(_buffer[pos].dados.origem);
    Serial.print(F(" cod="));
    Serial.println(_buffer[pos].dados.codigo);
  }
  Serial.println(F("  --- Fim ---"));
}
