// =============================================================
//  Project SIGMA
//  Arquivo    : AlarmService.h
//  Descricao  : Servico de avaliacao de alarmes (Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.0
//  Data       : 2026-06-27
// =============================================================

#pragma once

// -------------------------
//  Contexto injetado
// -------------------------
struct AlarmContext {
  float tempAvisoMax;
  float tempCritica;
  bool  basAtivo;
  float basMedia;
  float basStddev;
  float fatorK;
  float vibAviso;
  float vibCritica;
};

class AlarmService {
public:
  const char* classificar(float tempC, float vibG, 
                          const AlarmContext& ctx);
};