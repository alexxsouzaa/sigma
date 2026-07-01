// =============================================================
//  Project SIGMA
//  Arquivo    : DiagnosticReport.h
//  Descricao  : Relatorio de diagnostico de inicializacao.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.17.0
//  Codename   : Boot Diagnostics
//  Data       : 2026-07-01
// =============================================================

#pragma once

#include <stdint.h>

// -------------------------
//  Dados do Diagnostico
// -------------------------
struct DiagData {
  uint32_t bootCount;
  uint32_t freeHeap;
  bool     mpuOk;
  bool     ds18b20Ok;
  bool     calOk;
  bool     horOk;
  bool     cfgOk;
  bool     basOk;
  bool     tasksOk;
};

// =============================================================
//  CLASSE: DiagnosticReport
//  Formata e exibe o relatorio de diagnostico do boot.
// =============================================================
class DiagnosticReport {
public:
  void imprimir(const DiagData& dados) const;
};
