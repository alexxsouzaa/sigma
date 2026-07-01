// =============================================================
//  Project SIGMA
//  Arquivo    : SensorQualityService.cpp
//  Descricao  : Indice de confianca dos sensores baseado em
//               falhas consecutivas e bootstrapping.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.14.0
//  Codename   : Multitarefa UI
//  Data       : 2026-06-30
// =============================================================

#include "SensorQualityService.h"

// =============================================================
//  CONSTRUTOR
// =============================================================
SensorQualityService::SensorQualityService() {
  _tempFalhas = 0;
  _tempN      = 0;
  _vibFalhas  = 0;
  _vibN       = 0;
}

// =============================================================
//  FUNCAO: atualizarTemp
//  Atualiza estado do sensor de temperatura.
// =============================================================
void SensorQualityService::atualizarTemp(bool dadoValido) {
  if (!dadoValido) {
    _tempFalhas++;
    _tempN = 0;
    return;
  }
  _tempFalhas = 0;
  if (_tempN < JANELA_BOOT) _tempN++;
}

// =============================================================
//  FUNCAO: atualizarVib
//  Atualiza estado do sensor de vibracao.
// =============================================================
void SensorQualityService::atualizarVib(bool dadoValido) {
  if (!dadoValido) {
    _vibFalhas++;
    _vibN = 0;
    return;
  }
  _vibFalhas = 0;
  if (_vibN < JANELA_BOOT) _vibN++;
}

// =============================================================
//  FUNCAO: obterRelatorio
//  Computa confianca atual de cada sensor.
//  Regra:
//    falhas >= MAX_FALHAS → 0% (FALHA)
//    falhas > 0           → 80%..20% (degradado)
//    bootstrapping        → 50%..99%
//    normal               → 100%
// =============================================================
SensorQualityReport SensorQualityService::obterRelatorio() const {
  SensorQualityReport rel;

  // --- Temperatura ---
  rel.temp.falhasConsec = _tempFalhas;
  if (_tempFalhas >= MAX_FALHAS) {
    rel.temp.confianca = 0;
  } else if (_tempFalhas > 0) {
    rel.temp.confianca = (uint8_t)(100 - (_tempFalhas * 20));
  } else if (_tempN < JANELA_BOOT) {
    rel.temp.confianca = (uint8_t)(50 + ((float)_tempN / JANELA_BOOT) * 49.0f);
  } else {
    rel.temp.confianca = 100;
  }

  // --- Vibracao ---
  rel.vib.falhasConsec = _vibFalhas;
  if (_vibFalhas >= MAX_FALHAS) {
    rel.vib.confianca = 0;
  } else if (_vibFalhas > 0) {
    rel.vib.confianca = (uint8_t)(100 - (_vibFalhas * 20));
  } else if (_vibN < JANELA_BOOT) {
    rel.vib.confianca = (uint8_t)(50 + ((float)_vibN / JANELA_BOOT) * 49.0f);
  } else {
    rel.vib.confianca = 100;
  }

  return rel;
}

// =============================================================
//  FUNCAO: obterStatus
//  Retorna string de status baseada na confianca.
// =============================================================
const char* SensorQualityService::obterStatus(uint8_t confianca) const {
  if (confianca == 0)   return "FALHA";
  if (confianca < 50)   return "DEGRADADO";
  return "NORMAL";
}

// =============================================================
//  FUNCAO: reset
//  Zera todo o estado interno.
// =============================================================
void SensorQualityService::reset() {
  _tempFalhas = 0;
  _tempN      = 0;
  _vibFalhas  = 0;
  _vibN       = 0;
}
