// =============================================================
//  Project SIGMA
//  Arquivo    : NvsAlarm.h
//  Descricao  : Persistencia do historico de alarmes (T016).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.16.0
//  Codename   : Sistema de Alertas
//  Data       : 2026-07-01
// =============================================================

#pragma once

#include <stdint.h>
#include "../services/alarm/AlarmManager.h"

class NvsAlarm {
public:
  bool salvar(const AlarmaHistorico* dados, uint8_t qtd);
  uint8_t carregar(AlarmaHistorico* buffer, uint8_t capacidade);
  void apagar();
};
