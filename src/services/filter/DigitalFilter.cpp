// =============================================================
//  Project SIGMA
//  Arquivo    : DigitalFilter.cpp
//  Descricao  : Implementacao dos algoritmos de filtragem
//               digital (media movel e EMA).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.4
//  Codename   : Calibracao Passo Fix
//  Data       : 2026-06-29
// =============================================================

#include "DigitalFilter.h"

// =============================================================
//  CONSTRUTOR
//  Inicia com filtro desligado e estado zerado.
// =============================================================
DigitalFilter::DigitalFilter() {
  _cfg.tipo   = FilterType::NENHUM;
  _cfg.janela = 1;
  _cfg.alfa   = 0.5f;
  _indice = 0;
  _count  = 0;
  _prevEma = 0.0f;
  _soma    = 0.0f;
  for (int i = 0; i < MAX_JANELA; i++) {
    _buffer[i] = 0.0f;
  }
}

// =============================================================
//  FUNCAO: configurar
//  Aplica nova configuracao e reinicia o estado interno.
// =============================================================
void DigitalFilter::configurar(const FilterConfig& cfg) {
  _cfg = cfg;

  // Valida limites da janela
  if (_cfg.janela < 2)  _cfg.janela = 2;
  if (_cfg.janela > MAX_JANELA) _cfg.janela = MAX_JANELA;

  // Valida limites do alfa EMA
  if (_cfg.alfa < 0.01f) _cfg.alfa = 0.01f;
  if (_cfg.alfa > 0.99f) _cfg.alfa = 0.99f;

  reset();
}

// =============================================================
//  FUNCAO: reset
//  Zera buffer e medias, mantendo a configuracao.
// =============================================================
void DigitalFilter::reset() {
  _indice  = 0;
  _count   = 0;
  _prevEma = 0.0f;
  _soma    = 0.0f;
  for (int i = 0; i < MAX_JANELA; i++) {
    _buffer[i] = 0.0f;
  }
}

// =============================================================
//  FUNCAO: filtrar
//  Aplica o filtro configurado ao valor bruto.
//  Retorna o valor filtrado (ou o proprio raw se NENHUM).
// =============================================================
float DigitalFilter::filtrar(float valorRaw) {
  switch (_cfg.tipo) {
    case FilterType::MEDIA_MOVEL:
      return filtrarMediaMovel(valorRaw);
    case FilterType::EMA:
      return filtrarEma(valorRaw);
    default:
      return valorRaw;
  }
}

// =============================================================
//  FUNCAO: filtrarMediaMovel
//  Media aritmetica dos ultimos N valores.
//  Implementacao com soma corrente (O(1) por amostra).
// =============================================================
float DigitalFilter::filtrarMediaMovel(float valor) {
  // Remove o valor mais antigo da soma se a janela estiver cheia
  if (_count >= _cfg.janela) {
    _soma -= _buffer[_indice];
  }

  // Insere o novo valor
  _buffer[_indice] = valor;
  _soma += valor;
  _indice = (_indice + 1) % _cfg.janela;

  if (_count < _cfg.janela) {
    _count++;
  }

  return _soma / _count;
}

// =============================================================
//  FUNCAO: filtrarEma
//  Media exponencial: saida = alfa * raw + (1-alfa) * saida_ant
//  Para a primeira amostra usa o proprio valor raw.
// =============================================================
float DigitalFilter::filtrarEma(float valor) {
  if (_count == 0) {
    _prevEma = valor;
    _count = 1;
    return _prevEma;
  }
  _prevEma = (_cfg.alfa * valor) + ((1.0f - _cfg.alfa) * _prevEma);
  return _prevEma;
}

// =============================================================
//  FUNCAO: obterConfig
//  Retorna a configuracao ativa no momento.
// =============================================================
FilterConfig DigitalFilter::obterConfig() const {
  return _cfg;
}
