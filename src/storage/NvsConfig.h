// =============================================================
//  Project SIGMA
//  Arquivo    : NvsConfig.h
//  Descricao  : Persistencia de limiares absolutos e escalas.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.1
//  Data       : 2026-06-27
// =============================================================

#pragma once

// -------------------------
//  Contrato de dados
// -------------------------
struct NvsConfigData {
  int   escalaG;
  float vibAviso;
  float vibCritica;
};

class NvsConfig {
public:
  bool salvar(const NvsConfigData& dados);
  bool carregar(NvsConfigData& dados);
  void apagar();
  bool existe();
};