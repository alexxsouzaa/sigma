// =============================================================
//  Project SIGMA
//  Arquivo    : NvsBoot.cpp
//  Descricao  : Implementacao do namespace sigma_boot.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.17.0
//  Codename   : Boot Diagnostics
//  Data       : 2026-07-01
// =============================================================

#include "NvsBoot.h"
#include <Preferences.h>

static Preferences prefs;

uint32_t NvsBoot::lerContador() {
  if (!prefs.begin("sigma_boot", true)) return 0;
  uint32_t cnt = prefs.getUInt("bootCnt", 0);
  prefs.end();
  return cnt;
}

void NvsBoot::incrementar() {
  if (!prefs.begin("sigma_boot", false)) return;
  uint32_t cnt = prefs.getUInt("bootCnt", 0);
  prefs.putUInt("bootCnt", cnt + 1);
  prefs.end();
}

void NvsBoot::apagar() {
  prefs.begin("sigma_boot", false);
  prefs.clear();
  prefs.end();
}
