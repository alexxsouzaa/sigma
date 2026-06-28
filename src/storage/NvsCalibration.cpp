// =============================================================
//  Project SIGMA
//  Arquivo    : NvsCalibration.cpp
//  Descricao  : Implementacao do namespace sigma_cal.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.0
//  Data       : 2026-06-27
// =============================================================

#include "NvsCalibration.h"
#include <Preferences.h>

static Preferences prefs;

// =============================================================
//  FUNCAO: salvar
//  Abre namespace sigma_cal e grava offsets em memoria nao
//  volatil. Retorna false se falhar.
// =============================================================
bool NvsCalibration::salvar(const NvsCalibrationData& dados) {
  if (!prefs.begin("sigma_cal", false)) return false;
  prefs.putFloat("offsetAX", dados.offsetAX);
  prefs.putFloat("offsetAY", dados.offsetAY);
  prefs.putFloat("offsetAZ", dados.offsetAZ);
  prefs.putBool("calibrado", dados.calibrado);
  prefs.end();
  return true;
}

// =============================================================
//  FUNCAO: carregar
//  Le offsets. Retorna false se nao houver calibracao ativa.
// =============================================================
bool NvsCalibration::carregar(NvsCalibrationData& dados) {
  if (!prefs.begin("sigma_cal", true)) return false;
  
  dados.calibrado = prefs.getBool("calibrado", false);
  if (dados.calibrado) {
    dados.offsetAX = prefs.getFloat("offsetAX", 0.0f);
    dados.offsetAY = prefs.getFloat("offsetAY", 0.0f);
    dados.offsetAZ = prefs.getFloat("offsetAZ", 0.0f);
  }
  prefs.end();
  return dados.calibrado;
}

// =============================================================
//  FUNCAO: apagar
//  Deleta todas as chaves do namespace.
// =============================================================
void NvsCalibration::apagar() {
  prefs.begin("sigma_cal", false);
  prefs.clear();
  prefs.end();
}

// =============================================================
//  FUNCAO: existe
//  Verifica a existencia da flag base do namespace.
// =============================================================
bool NvsCalibration::existe() {
  prefs.begin("sigma_cal", true);
  bool ex = prefs.isKey("calibrado");
  prefs.end();
  return ex;
}