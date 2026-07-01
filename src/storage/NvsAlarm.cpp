// =============================================================
//  Project SIGMA
//  Arquivo    : NvsAlarm.cpp
//  Descricao  : Implementacao do namespace sigma_alm.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.18.0
//  Codename   : Runtime Diagnostics
//  Data       : 2026-07-01
// =============================================================

#include "NvsAlarm.h"
#include <Preferences.h>

static Preferences prefs;

bool NvsAlarm::salvar(const AlarmaHistorico* dados, uint8_t qtd) {
  if (!prefs.begin("sigma_alm", false)) return false;

  prefs.putUChar("qtd", qtd);
  if (qtd > 0) {
    prefs.putBytes("hist", dados,
                   sizeof(AlarmaHistorico) * qtd);
  }
  prefs.end();
  return true;
}

uint8_t NvsAlarm::carregar(AlarmaHistorico* buffer,
                            uint8_t capacidade) {
  if (!prefs.begin("sigma_alm", true)) return 0;

  uint8_t qtd = prefs.getUChar("qtd", 0);
  if (qtd > 0) {
    if (qtd > capacidade) qtd = capacidade;
    prefs.getBytes("hist", buffer,
                   sizeof(AlarmaHistorico) * qtd);
  }
  prefs.end();
  return qtd;
}

void NvsAlarm::apagar() {
  prefs.begin("sigma_alm", false);
  prefs.clear();
  prefs.end();
}
