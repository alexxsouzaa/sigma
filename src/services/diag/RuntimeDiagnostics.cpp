// =============================================================
//  Project SIGMA
//  Arquivo    : RuntimeDiagnostics.cpp
//  Descricao  : Monitoramento continuo de saude do sistema.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.18.0
//  Codename   : Runtime Diagnostics
//  Data       : 2026-07-01
// =============================================================

#include "RuntimeDiagnostics.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

RuntimeDiagnostics::RuntimeDiagnostics()
  : _tarefas{}, _qtdTarefas(0),
    _freeHeapMin(0xFFFFFFFF),
    _ultAtualizacao(0),
    _ultQualTemp(0.0f), _ultQualVib(0.0f),
    _ultEventosPend(0) {}

void RuntimeDiagnostics::adicionarTarefa(
    TaskHandle_t handle, const char* nome, uint16_t stackSize) {
  if (_qtdTarefas >= MAX_TAREFAS) return;
  _tarefas[_qtdTarefas].handle    = handle;
  _tarefas[_qtdTarefas].nome      = nome;
  _tarefas[_qtdTarefas].stackSize = stackSize;
  _qtdTarefas++;
}

void RuntimeDiagnostics::atualizar(
    uint32_t agora, float qualTemp, float qualVib,
    uint8_t eventosPendentes) {
  uint32_t heap = esp_get_free_heap_size();
  if (heap < _freeHeapMin) _freeHeapMin = heap;

  _ultAtualizacao   = agora;
  _ultQualTemp      = qualTemp;
  _ultQualVib       = qualVib;
  _ultEventosPend   = eventosPendentes;
}

void RuntimeDiagnostics::imprimir(
    uint32_t agora, uint32_t totalEventos) const {
  // Uptime formatado
  uint32_t t = _ultAtualizacao / 1000;
  uint32_t h = t / 3600;  t %= 3600;
  uint32_t m = t / 60;    t %= 60;

  char buf[9];
  buf[0] = '0' + (h / 10);  buf[1] = '0' + (h % 10);
  buf[2] = ':';
  buf[3] = '0' + (m / 10);  buf[4] = '0' + (m % 10);
  buf[5] = ':';
  buf[6] = '0' + (t / 10);  buf[7] = '0' + (t % 10);
  buf[8] = '\0';

  uint32_t heap = esp_get_free_heap_size();

  Serial.println(F(""));
  Serial.println(F(
    "============================================================"));
  Serial.println(F(
    "  DIAGNOSTICO DE EXECUCAO"));
  Serial.println(F(
    "============================================================"));
  Serial.print(F("  Uptime    : "));
  Serial.println(buf);
  Serial.print(F("  RAM       : "));
  Serial.print(heap);
  Serial.print(F(" B (min: "));
  Serial.print(_freeHeapMin);
  Serial.println(F(" B)"));
  Serial.println(F(
    "------------------------------------------------------------"));
  Serial.println(F(
    "  TAREFA              STACK  LIVRE   USO"));

  for (uint8_t i = 0; i < _qtdTarefas; i++) {
    if (_tarefas[i].handle == NULL) continue;
    UBaseType_t hwm = uxTaskGetStackHighWaterMark(
                        _tarefas[i].handle);
    uint16_t usado  = _tarefas[i].stackSize - hwm;
    uint8_t  pct    = (usado * 100) / _tarefas[i].stackSize;

    Serial.print(F("  "));
    Serial.print(_tarefas[i].nome);
    uint8_t len = 0;
    while (_tarefas[i].nome[len] != '\0' && len < 40) len++;
    for (uint8_t j = len; j < 20; j++) Serial.print(F(" "));
    Serial.print(F(" "));
    Serial.print(_tarefas[i].stackSize);
    Serial.print(F("  "));
    if (hwm < 100) Serial.print(F(" "));
    if (hwm < 10)  Serial.print(F(" "));
    Serial.print(hwm);
    Serial.print(F("  "));
    if (pct < 100) Serial.print(F(" "));
    if (pct < 10)  Serial.print(F(" "));
    Serial.print(pct);
    Serial.println(F("%"));
  }

  Serial.println(F(
    "------------------------------------------------------------"));
  Serial.println(F(
    "  SENSORES"));
  Serial.print(F("  Temp  : "));
  Serial.print(_ultQualTemp, 1);
  Serial.println(F("%"));
  Serial.print(F("  Vib   : "));
  Serial.print(_ultQualVib, 1);
  Serial.println(F("%"));
  Serial.println(F(
    "------------------------------------------------------------"));
  Serial.println(F(
    "  EVENTOS"));
  Serial.print(F("  Fila  : "));
  Serial.print(_ultEventosPend);
  Serial.println(F(" / 8"));
  Serial.print(F("  Total : "));
  Serial.println(totalEventos);
  Serial.println(F(
    "============================================================"));
  Serial.println(F(""));
}
