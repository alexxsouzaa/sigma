// =============================================================
//  Project SIGMA
//  Arquivo    : EventTypes.h
//  Descricao  : Tipos de eventos do sistema interno.
//               (T014 — Event Manager, Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.12.0
//  Codename   : Multitarefa SENSOR
//  Data       : 2026-06-30
// =============================================================

#pragma once

#include <stdint.h>

// -------------------------
//  Tipos de Evento
// -------------------------
enum class EventType : uint8_t {
  TEMP_LEITURA      = 0,  // Nova leitura de temperatura
  VIB_LEITURA       = 1,  // Nova leitura de vibracao
  TEMP_ALARME       = 2,  // Alarme de temperatura disparado
  VIB_ALARME        = 3,  // Alarme de vibracao disparado
  SAUDE_ALTERADA    = 4,  // Health score mudou de nivel
  OUTLIER_TEMP      = 5,  // Outlier rejeitado na temperatura
  OUTLIER_VIB       = 6,  // Outlier rejeitado na vibracao
  SENSOR_FALHA      = 7,  // Falha de leitura de sensor
  CALIBRACAO_INICIO = 8,  // Calibracao iniciada
  CALIBRACAO_FIM    = 9,  // Calibracao concluida
  QTDE              = 10  // Total de tipos (nao usar como evento)
};

// -------------------------
//  Dados do Evento
// -------------------------
struct EventoDados {
  float   valorPrincipal;   // Temperatura, RMS, score, etc.
  float   valorSecundario;  // Segundo valor associado
  uint8_t origem;           // 0=temperature, 1=vibracao, 2=sistema
  uint8_t codigo;           // Subtipo ou codigo de alarme
};
