// =============================================================
//  Project SIGMA
//  Arquivo    : NvsHorimeter.h
//  Descricao  : Persistencia do horimetro e logs de reset.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.16.0
//  Codename   : Sistema de Alertas
//  Data       : 2026-06-27
// =============================================================

#pragma once

#include <stdint.h>

// -------------------------
//  Contrato de dados
// -------------------------
struct NvsHorimeterData {
  float horimetro;
  int   resetCount;
  char  resetMotivo[32]; // Evita alocacao dinamica (String)
};

class NvsHorimeter {
public:
  bool salvar(const NvsHorimeterData& dados);
  bool carregar(NvsHorimeterData& dados);
  void apagar();
  bool existe();
};