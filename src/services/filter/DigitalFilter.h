// =============================================================
//  Project SIGMA
//  Arquivo    : DigitalFilter.h
//  Descricao  : Filtro digital para reducao de ruido
//               em leituras de sensores (Camada 4).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.9.0
//  Codename   : Qualidade dos Sensores
//  Data       : 2026-06-29
// =============================================================

#pragma once

#include <stdint.h>

// -------------------------
//  Tipo de Filtro
// -------------------------
enum class FilterType : uint8_t {
  NENHUM       = 0,  // Passa direto (sem filtro)
  MEDIA_MOVEL  = 1,  // Media movel simples
  EMA          = 2   // Media exponencial
};

// -------------------------
//  Configuracao do Filtro
// -------------------------
struct FilterConfig {
  FilterType tipo;     // Tipo de filtro ativo
  uint8_t    janela;   // Tamanho da janela (2 a 32) — usado
                       // apenas em MEDIA_MOVEL
  float      alfa;     // Coeficiente de suavizacao (0.01 a 0.99)
                       // — usado apenas em EMA
};

// =============================================================
//  CLASSE: DigitalFilter
//  Filtro digital reentrante (uma instancia por sinal).
//  Uso: configurar() + filtrar() a cada nova leitura.
// =============================================================
class DigitalFilter {
public:
  static const int MAX_JANELA = 32;

  DigitalFilter();

  void         configurar(const FilterConfig& cfg);
  float        filtrar(float valorRaw);
  void         reset();

  FilterConfig obterConfig() const;

private:
  FilterConfig _cfg;
  float        _buffer[MAX_JANELA];
  int          _indice;
  int          _count;
  float        _prevEma;
  float        _soma;

  float filtrarNenhum(float valor);
  float filtrarMediaMovel(float valor);
  float filtrarEma(float valor);
};
