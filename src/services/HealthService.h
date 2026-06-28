// =============================================================
//  Project SIGMA
//  Arquivo    : HealthService.h
//  Descricao  : Servico de calculo do Health Score (Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.5.1
//  Data       : 2026-06-27
// =============================================================

#pragma once

// -------------------------
//  Contexto injetado
// -------------------------
struct HealthContext {
  float tempAvisoMax;
  float tempCritica;
  float intervaloManutencaoH;
  bool  basAtivo;
  float basMedia;
  float basStddev;
  float fatorK;
  float vibAviso;
  float vibCritica;
};

class HealthService {
public:
  float calcularScore(float tempC, float vibG, float horas, 
                      const HealthContext& ctx);
  
  const char* classificar(float score);
};