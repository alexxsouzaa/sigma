// =============================================================
//  Project SIGMA
//  Arquivo    : NvsHorimeter.cpp
//  Descricao  : Implementacao do namespace sigma_sys.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.2
//  Codename   : Calibracao Robusta I2C
//  Data       : 2026-06-27
// =============================================================

#include "NvsHorimeter.h"
#include <Preferences.h>
#include <string.h>

static Preferences prefs;

bool NvsHorimeter::salvar(const NvsHorimeterData& dados) {
  if (!prefs.begin("sigma_sys", false)) return false;
  prefs.putFloat("horimetro", dados.horimetro);
  prefs.putInt("resetCount", dados.resetCount);
  prefs.putString("resetMotivo", dados.resetMotivo);
  prefs.end();
  return true;
}

bool NvsHorimeter::carregar(NvsHorimeterData& dados) {
  if (!prefs.begin("sigma_sys", true)) return false;
  
  dados.horimetro  = prefs.getFloat("horimetro", 0.0f);
  dados.resetCount = prefs.getInt("resetCount", 0);
  
  // Le string direto para o buffer estatico sem String local
  prefs.getString("resetMotivo", "Nenhum").toCharArray(
    dados.resetMotivo, 32);
  
  prefs.end();
  return true;
}

void NvsHorimeter::apagar() {
  prefs.begin("sigma_sys", false);
  prefs.clear();
  prefs.end();
}

bool NvsHorimeter::existe() {
  prefs.begin("sigma_sys", true);
  bool ex = prefs.isKey("horimetro");
  prefs.end();
  return ex;
}