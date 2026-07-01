// =============================================================
//  Project SIGMA
//  Arquivo    : AlarmService.cpp
//  Descricao  : Implementacao da classificacao de alarmes.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.16.0
//  Codename   : Sistema de Alertas
//  Data       : 2026-06-27
// =============================================================

#include "AlarmService.h"

// =============================================================
//  FUNCAO: classificar
//  Determina o nivel de seguranca operacional do motor baseando
//  seja nos limites fixos ou no baseline estatistico relativo.
// -------------------------------------------------------------
//  Parametros:
//    tempC : Temperatura atual em oC.
//    vibG  : Vibracao atual em g.
//    ctx   : Contexto contendo todos os limiares definidos.
//  Retorno  : "NORMAL", "AVISO" ou "CRITICO".
// =============================================================
const char* AlarmService::classificar(float tempC, float vibG, 
                                      const AlarmContext& ctx) {
  float limAviso, limCritico;

  if (ctx.basAtivo) {
    limAviso   = ctx.basMedia + (ctx.fatorK * ctx.basStddev);
    limCritico = ctx.basMedia + (2.0f * ctx.fatorK * ctx.basStddev);
  } else {
    limAviso   = ctx.vibAviso;
    limCritico = ctx.vibCritica;
  }

  // Falha critica tem prioridade sobre aviso (Fail-Safe)
  if (tempC >= ctx.tempCritica || vibG >= limCritico) {
    return "CRITICO";
  }
  
  if (tempC >= ctx.tempAvisoMax || vibG >= limAviso) {
    return "AVISO";
  }
  
  return "NORMAL";
}