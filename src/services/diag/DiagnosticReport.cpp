// =============================================================
//  Project SIGMA
//  Arquivo    : DiagnosticReport.cpp
//  Descricao  : Impressao do relatorio de diagnostico.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.17.0
//  Codename   : Boot Diagnostics
//  Data       : 2026-07-01
// =============================================================

#include "DiagnosticReport.h"
#include <Arduino.h>

// -------------------------
//  Macros de formatacao
// -------------------------
#define SEP_LONGO \
  "============================================================"
#define SEP_CURTO \
  "------------------------------------------------------------"

void DiagnosticReport::imprimir(const DiagData& d) const {
  Serial.println(F(""));
  Serial.println(F(SEP_LONGO));
  Serial.println(F(
    "  DIAGNOSTICO DE INICIALIZACAO"));
  Serial.println(F(SEP_LONGO));
  Serial.print(F("  Boot #"));
  Serial.println(d.bootCount);
  Serial.print(F("  RAM  : "));
  Serial.print(d.freeHeap);
  Serial.println(F(" bytes livres"));
  Serial.println(F(SEP_CURTO));
  Serial.println(F(
    "  SUBSISTEMA             STATUS"));

  auto linha = [](const char* nome, bool ok) {
    Serial.print(F("  "));
    Serial.print(nome);
    // Alinha com espacos ate coluna 25
    for (uint8_t i = 0; nome[i] != '\0'; i++); // dummy
    uint8_t len = 0;
    while (nome[len] != '\0' && len < 40) len++;
    for (uint8_t i = len; i < 24; i++) Serial.print(F(" "));
    Serial.println(ok ? F("PASS") : F("FAIL"));
  };

  linha("MPU6050",          d.mpuOk);
  linha("DS18B20",          d.ds18b20Ok);
  linha("NVS (cal)",        d.calOk);
  linha("NVS (hor)",        d.horOk);
  linha("NVS (cfg)",        d.cfgOk);
  linha("NVS (bas)",        d.basOk);
  linha("FreeRTOS Tasks",   d.tasksOk);

  Serial.println(F(SEP_CURTO));

  bool tudoPass = d.mpuOk && d.ds18b20Ok && d.calOk
               && d.horOk && d.cfgOk && d.basOk
               && d.tasksOk;

  if (tudoPass) {
    Serial.println(F(
      "  RESULTADO: TODOS OS TESTES PASSARAM"));
  } else {
    Serial.println(F(
      "  RESULTADO: UM OU MAIS TESTES FALHARAM"));
    Serial.println(F(
      "  [WARN] Consulte as mensagens de erro acima."));
  }
  Serial.println(F(SEP_LONGO));
  Serial.println(F(""));
}
