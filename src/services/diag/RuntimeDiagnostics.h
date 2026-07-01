// =============================================================
//  Project SIGMA
//  Arquivo    : RuntimeDiagnostics.h
//  Descricao  : Monitoramento continuo de saude do sistema.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.18.0
//  Codename   : Runtime Diagnostics
//  Data       : 2026-07-01
// =============================================================

#pragma once

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// =============================================================
//  CLASSE: RuntimeDiagnostics
//  Amostra metricas do sistema a cada ciclo e gera relatorio.
// =============================================================
class RuntimeDiagnostics {
public:
  static constexpr uint8_t MAX_TAREFAS = 6;

  RuntimeDiagnostics();
  void adicionarTarefa(TaskHandle_t handle, const char* nome,
                       uint16_t stackSize);
  void atualizar(uint32_t agora, float qualTemp, float qualVib,
                 uint8_t eventosPendentes);
  void imprimir(uint32_t agora, uint32_t totalEventos) const;
  uint32_t getFreeHeapMin() const { return _freeHeapMin; }

private:
  struct TaskEntry {
    TaskHandle_t handle;
    const char*  nome;
    uint16_t     stackSize;
  };

  TaskEntry _tarefas[MAX_TAREFAS];
  uint8_t   _qtdTarefas;
  uint32_t  _freeHeapMin;
  uint32_t  _ultAtualizacao;
  float     _ultQualTemp;
  float     _ultQualVib;
  uint8_t   _ultEventosPend;
};
