// =============================================================
//  Project SIGMA
//  Arquivo    : SerialUI.h
//  Descricao  : Gerenciador da Interface Serial (SUS v2.1.0).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.11.0
//  Codename   : Historico de Eventos
//  Data       : 2026-06-27
// =============================================================

#pragma once

#include <Arduino.h>
#include "hal/Version.h"
#include "../services/analytics/AnalyticsTypes.h" // Importa o contrato da API
#include "../services/SensorQualityService.h"

// Macros de layout SUS
#define LINHA_SEP "============================================================"
#define LINHA_SUB "------------------------------------------------------------"
#define CAIXA_TOP "+----------------------------------------------------------+"
#define CAIXA_BOT "+----------------------------------------------------------+"

class SerialUI {
public:
  void begin(uint32_t baudRate);
  void imprimirBarraProgresso(int atual, int total);
  bool aguardarConfirmacao(const char* msgS, const char* msgN, uint32_t timeoutMs);
  void imprimirMensagem(const char* prefixo, const char* mensagem);
  
  // Telas e Menus Especificos
  void imprimirCabecalhoBoot();
  void imprimirErroFatal(const char* erro, const char* acao);
  void exibirMenuSensibilidade(int escalaAtual, float aviso, float critico);
  void imprimirRelatorio(uint32_t tempoMs, float temp, float vib, float horas, 
                         float score, const char* clHealth, const char* clAlarme,
                         const AnalyticsResult& analytics,
                         const SensorQualityReport& qual);
};