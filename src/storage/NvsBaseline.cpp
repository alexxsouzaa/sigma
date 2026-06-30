// =============================================================
//  Project SIGMA
//  Arquivo    : NvsBaseline.cpp
//  Descricao  : Implementacao do namespace sigma_bas.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.5
//  Codename   : Calibracao Nao-Bloqueante Fix
//  Data       : 2026-06-27
// =============================================================

#include "NvsBaseline.h"
#include <Preferences.h>

static Preferences prefs;

bool NvsBaseline::salvar(const NvsBaselineData& dados) {
  if (!prefs.begin("sigma_bas", false)) return false;
  prefs.putFloat("basMedia", dados.basMedia);
  prefs.putFloat("basStddev", dados.basStddev);
  prefs.putFloat("fatorK", dados.fatorK);
  prefs.putBool("basAtivo", dados.basAtivo);
  prefs.putInt("basN", dados.basN);
  prefs.end();
  return true;
}

bool NvsBaseline::carregar(NvsBaselineData& dados) {
  if (!prefs.begin("sigma_bas", true)) return false;
  
  dados.basAtivo = prefs.getBool("basAtivo", false);
  if (dados.basAtivo) {
    dados.basMedia  = prefs.getFloat("basMedia", 0.0f);
    dados.basStddev = prefs.getFloat("basStddev", 0.0f);
    dados.fatorK    = prefs.getFloat("fatorK", 2.0f);
    dados.basN      = prefs.getInt("basN", 0);
  }
  
  prefs.end();
  return dados.basAtivo;
}

void NvsBaseline::apagar() {
  prefs.begin("sigma_bas", false);
  prefs.clear();
  prefs.end();
}

bool NvsBaseline::existe() {
  prefs.begin("sigma_bas", true);
  bool ex = prefs.isKey("basAtivo");
  prefs.end();
  return ex;
}