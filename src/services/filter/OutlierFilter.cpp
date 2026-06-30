// =============================================================
//  Project SIGMA
//  Arquivo    : OutlierFilter.cpp
//  Descricao  : Implementacao do filtro de outliers com
//               algoritmo online de Welford + Z-Score.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.12.0
//  Codename   : Multitarefa SENSOR
//  Data       : 2026-06-30
// =============================================================

#include "OutlierFilter.h"

const float   OutlierFilter::LIMIAR_ZSCORE_PADRAO = 3.5f;
const uint8_t OutlierFilter::MIN_AMOSTRAS_PADRAO  = 10;

// =============================================================
//  CONSTRUTOR
//  Inicia com estado zerado e limites desligados.
// =============================================================
OutlierFilter::OutlierFilter() {
  _limMin  = -INFINITY;
  _limMax  = INFINITY;
  _zLimiar = LIMIAR_ZSCORE_PADRAO;
  _minN    = MIN_AMOSTRAS_PADRAO;
  _count   = 0;
  _mean    = 0.0f;
  _m2      = 0.0f;
}

// =============================================================
//  FUNCAO: configurar
//  Aplica limites fisicos e parametros do Z-Score.
// =============================================================
void OutlierFilter::configurar(float limiteMin, float limiteMax,
                               float zScoreLimiar, uint8_t minAmostras) {
  _limMin  = limiteMin;
  _limMax  = limiteMax;
  _zLimiar = zScoreLimiar;
  _minN    = (minAmostras < 3) ? 3 : minAmostras;
  reset();
}

// =============================================================
//  FUNCAO: filtrar
//  1. Verifica limite fisico → rejeita se fora
//  2. Bootstrap: aceita as primeiras N amostras
//  3. Z-Score: rejeita se |z| > limiar
//  Retorna o valor se aceito, NAN se rejeitado.
// =============================================================
float OutlierFilter::filtrar(float valor) {
  if (!dentroDosLimites(valor)) {
    return NAN;
  }

  // Bootstrap: aceita as primeiras amostras
  if (_count < _minN) {
    _count++;
    float delta = valor - _mean;
    _mean += delta / (float)_count;
    float delta2 = valor - _mean;
    _m2 += delta * delta2;
    return valor;
  }

  // Z-Score
  float var = _m2 / (float)_count;
  if (var >= 1e-12f) {
    float stddev = sqrt(var);
    float z = (valor - _mean) / stddev;
    if (z < 0.0f) z = -z;
    if (z > _zLimiar) {
      return NAN;
    }
  }

  // Aceito — atualiza estatisticas
  _count++;
  float delta = valor - _mean;
  _mean += delta / (float)_count;
  float delta2 = valor - _mean;
  _m2 += delta * delta2;
  return valor;
}

// =============================================================
//  FUNCAO: reset
//  Zera estatisticas, mantendo a configuracao.
// =============================================================
void OutlierFilter::reset() {
  _count = 0;
  _mean  = 0.0f;
  _m2    = 0.0f;
}

// =============================================================
//  FUNCAO: isCalibrado
//  Retorna true se ja coletou amostras suficientes para
//  o Z-Score ter significancia estatistica.
// =============================================================
bool OutlierFilter::isCalibrado() const {
  return _count >= _minN;
}

// =============================================================
//  FUNCAO: obterMedia
//==============================================================
float OutlierFilter::obterMedia() const {
  return _mean;
}

// =============================================================
//  FUNCAO: obterDesvio
//  Desvio padrao amostral via raiz de M2 / count.
//  Retorna 0 se count < 2 (indefinido).
// =============================================================
float OutlierFilter::obterDesvio() const {
  if (_count < 2) return 0.0f;
  return sqrt(_m2 / (float)_count);
}

// =============================================================
//  FUNCAO: obterContagem
//==============================================================
uint16_t OutlierFilter::obterContagem() const {
  return _count;
}

// =============================================================
//  FUNCAO: dentroDosLimites
//  Verifica se o valor esta dentro dos limites fisicos.
// =============================================================
bool OutlierFilter::dentroDosLimites(float v) const {
  return (v >= _limMin && v <= _limMax);
}
