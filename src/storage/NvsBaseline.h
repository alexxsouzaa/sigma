// =============================================================
//  Project SIGMA
//  Arquivo    : NvsBaseline.h
//  Descricao  : Persistencia do modelo de vibracao normal.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.11.0
//  Codename   : Historico de Eventos
//  Data       : 2026-06-27
// =============================================================

#pragma once

// -------------------------
//  Contrato de dados
// -------------------------
struct NvsBaselineData {
  float basMedia;
  float basStddev;
  float fatorK;
  bool  basAtivo;
  int   basN;
};

class NvsBaseline {
public:
  bool salvar(const NvsBaselineData& dados);
  bool carregar(NvsBaselineData& dados);
  void apagar();
  bool existe();
};