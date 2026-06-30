// =============================================================
//  Project SIGMA
//  Arquivo    : NvsConfig.cpp
//  Descricao  : Implementacao do namespace sigma_cfg.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.3
//  Codename   : Calibracao Nao-Bloqueante
//  Data       : 2026-06-27
// =============================================================

#include "NvsConfig.h"
#include <Preferences.h>

static Preferences prefs;

bool NvsConfig::salvar(const NvsConfigData& dados) {
  if (!prefs.begin("sigma_cfg", false)) return false;
  prefs.putInt("escalaG", dados.escalaG);
  prefs.putFloat("vibAviso", dados.vibAviso);
  prefs.putFloat("vibCritica", dados.vibCritica);
  prefs.end();
  return true;
}

bool NvsConfig::carregar(NvsConfigData& dados) {
  if (!prefs.begin("sigma_cfg", true)) return false;
  
  // Padroes injetados se a leitura direta falhar
  dados.escalaG    = prefs.getInt("escalaG", 8);
  dados.vibAviso   = prefs.getFloat("vibAviso", 2.0f);
  dados.vibCritica = prefs.getFloat("vibCritica", 4.0f);
  
  prefs.end();
  return true;
}

void NvsConfig::apagar() {
  prefs.begin("sigma_cfg", false);
  prefs.clear();
  prefs.end();
}

bool NvsConfig::existe() {
  prefs.begin("sigma_cfg", true);
  bool ex = prefs.isKey("escalaG");
  prefs.end();
  return ex;
}