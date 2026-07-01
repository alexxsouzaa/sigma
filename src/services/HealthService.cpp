// =============================================================
//  Project SIGMA
//  Arquivo    : HealthService.cpp
//  Descricao  : Implementacao da logica do Health Score.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.16.0
//  Codename   : Sistema de Alertas
//  Data       : 2026-06-27
// =============================================================

#include "HealthService.h"

// =============================================================
//  FUNCAO: calcularScore
//  Calcula pontuacao de saude ponderada de 0 a 100%.
// -------------------------------------------------------------
//  Parametros:
//    tempC : Temperatura atual do motor (oC).
//    vibG  : Vibracao atual RMS (g).
//    horas : Horas totais acumuladas (h).
//    ctx   : Contexto com todos os limiares e baselines.
//  Retorno  : Valor percentual de saude (float).
// =============================================================
float HealthService::calcularScore(float tempC, float vibG, 
                                   float horas, 
                                   const HealthContext& ctx) {
  float sT, sV, sH;

  // -- Componente Temperatura (40%) --
  if (tempC <= ctx.tempAvisoMax) {
    sT = 1.0f;
  } else if (tempC >= ctx.tempCritica) {
    sT = 0.0f;
  } else {
    sT = 1.0f - ((tempC - ctx.tempAvisoMax) / 
                 (ctx.tempCritica - ctx.tempAvisoMax));
  }

  // -- Componente Vibracao (40%) --
  if (ctx.basAtivo && ctx.basStddev > 0.0f) {
    float sigma = (vibG - ctx.basMedia) / ctx.basStddev;
    if (sigma <= 0.0f) {
      sV = 1.0f;
    } else if (sigma >= 2.0f * ctx.fatorK) {
      sV = 0.0f;
    } else if (sigma <= ctx.fatorK) {
      sV = 1.0f;
    } else {
      sV = 1.0f - ((sigma - ctx.fatorK) / ctx.fatorK);
    }
  } else {
    if (vibG <= ctx.vibAviso) {
      sV = 1.0f;
    } else if (vibG >= ctx.vibCritica) {
      sV = 0.0f;
    } else {
      sV = 1.0f - ((vibG - ctx.vibAviso) / 
                   (ctx.vibCritica - ctx.vibAviso));
    }
  }

  // -- Componente Horimetro (20%) --
  float lim = ctx.intervaloManutencaoH * 0.80f;
  if (horas <= lim) {
    sH = 1.0f;
  } else if (horas >= ctx.intervaloManutencaoH) {
    sH = 0.0f;
  } else {
    sH = 1.0f - ((horas - lim) / 
                 (ctx.intervaloManutencaoH - lim));
  }

  // Garante que o score final fique cravado entre 0 e 100
  float finalScore = (sT * 40.0f) + (sV * 40.0f) + (sH * 20.0f);
  if (finalScore < 0.0f)   finalScore = 0.0f;
  if (finalScore > 100.0f) finalScore = 100.0f;
  
  return finalScore;
}

// =============================================================
//  FUNCAO: classificar
//  Converte score numerico no texto definido no SUS sec 11.
// -------------------------------------------------------------
//  Retorno  : Ponteiro constante para string ASCII (PROGMEM).
// =============================================================
const char* HealthService::classificar(float score) {
  if (score >= 90.0f) return "Excelente";
  if (score >= 70.0f) return "Bom";
  if (score >= 50.0f) return "Manutencao Recomendada";
  return "CONDICAO CRITICA";
}