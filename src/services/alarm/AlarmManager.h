// =============================================================
//  Project SIGMA
//  Arquivo    : AlarmManager.h
//  Descricao  : Gerenciador de alarmes persistentes (T016).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.18.0
//  Codename   : Runtime Diagnostics
//  Data       : 2026-07-01
// =============================================================

#pragma once

#include <stdint.h>
#include "../analytics/AnalyticsTypes.h"

// -------------------------
//  Tipos de Alarme
// -------------------------
enum class AlarmType : uint8_t {
  TEMPERATURA = 0,
  VIBRACAO    = 1
};

// -------------------------
//  Alarme Ativo (RAM)
// -------------------------
struct AlarmaAtivo {
  AlarmType  tipo;
  AlarmLevel nivel;
  uint32_t   timestampInicio;
  bool       reconhecido;
  bool       ativo;
};

// -------------------------
//  Alarme Resolvido (RAM+NVS)
// -------------------------
struct AlarmaHistorico {
  uint32_t   timestampInicio;
  uint32_t   timestampFim;
  AlarmType  tipo;
  AlarmLevel nivel;
  bool       foiReconhecido;
};

// -------------------------
//  Transicao no Ciclo
// -------------------------
struct TransicaoAlarme {
  bool tempMudou;
  bool vibMudou;
  bool tempAtivo;
  bool vibAtivo;
};

// =============================================================
//  CLASSE: AlarmManager
//  Gerencia alarmes ativos e historico de alarmes.
// =============================================================
class AlarmManager {
public:
  static constexpr uint8_t MAX_HISTORICO = 20;

  AlarmManager();

  // Atualiza estado com base nos niveis atuais dos sensores
  TransicaoAlarme atualizar(AlarmLevel nivelTemp, AlarmLevel nivelVib,
                            uint32_t agora);

  // Reconhece alarme ativo por tipo
  bool reconhecer(AlarmType tipo);

  // Getters para alarmes ativos
  uint8_t quantidadeAtivos() const;
  const AlarmaAtivo* obterAtivo(uint8_t indice) const;

  // Getters para historico
  uint8_t quantidadeHistorico() const;
  const AlarmaHistorico* obterHistorico(uint8_t indice) const;

  // Persistencia
  void carregarHistorico(const AlarmaHistorico* dados, uint8_t qtd);
  uint8_t salvarHistorico(AlarmaHistorico* buffer) const;
  void limparHistorico();

private:
  AlarmaAtivo     _ativos[2];
  AlarmaHistorico _historico[20];
  uint8_t         _qtdHistorico;
  uint8_t         _idxHistorico;

  void adicionarHistorico(AlarmType tipo, AlarmLevel nivel,
                          uint32_t inicio, uint32_t fim,
                          bool reconhecido);
};
