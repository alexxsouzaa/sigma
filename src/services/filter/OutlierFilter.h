// =============================================================
//  Project SIGMA
//  Arquivo    : OutlierFilter.h
//  Descricao  : Filtro de rejeicao de outliers por
//               limite fisico e Z-Score (Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.8.0
//  Codename   : Deteccao de Outliers
//  Data       : 2026-06-30
// =============================================================

#pragma once

#include <stdint.h>
#include <math.h>

// =============================================================
//  CLASSE: OutlierFilter
//  Filtro rejeicao por Z-Score online (Welford) + limites
//  fisicos. Uma instancia por sinal.
//  Uso: configurar() + filtrar() a cada nova leitura.
// =============================================================
class OutlierFilter {
public:
  static const float LIMIAR_ZSCORE_PADRAO;
  static const uint8_t MIN_AMOSTRAS_PADRAO;

  OutlierFilter();

  void configurar(float limiteMin, float limiteMax,
                  float zScoreLimiar = LIMIAR_ZSCORE_PADRAO,
                  uint8_t minAmostras = MIN_AMOSTRAS_PADRAO);
  float filtrar(float valor);
  void  reset();

  bool     isCalibrado() const;
  float    obterMedia() const;
  float    obterDesvio() const;
  uint16_t obterContagem() const;

private:
  float    _limMin;
  float    _limMax;
  float    _zLimiar;
  uint8_t  _minN;
  uint16_t _count;
  float    _mean;
  float    _m2;

  bool dentroDosLimites(float v) const;
};
