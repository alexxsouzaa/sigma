// =============================================================
//  Project SIGMA
//  Arquivo    : AlarmManager.cpp
//  Descricao  : Gerenciador de alarmes persistentes (T016).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.17.0
//  Codename   : Boot Diagnostics
//  Data       : 2026-07-01
// =============================================================

#include <cstddef>
#include "AlarmManager.h"

AlarmManager::AlarmManager()
  : _qtdHistorico(0), _idxHistorico(0) {
  for (uint8_t i = 0; i < 2; i++) {
    _ativos[i].ativo = false;
    _ativos[i].nivel = AlarmLevel::NORMAL;
    _ativos[i].reconhecido = false;
    _ativos[i].timestampInicio = 0;
  }
}

TransicaoAlarme AlarmManager::atualizar(AlarmLevel nivelTemp,
                                         AlarmLevel nivelVib,
                                         uint32_t agora) {
  TransicaoAlarme result = {false, false, false, false};

  // Sensor de temperatura (indice 0)
  {
    const uint8_t idx = 0;
    if (nivelTemp != AlarmLevel::NORMAL) {
      if (!_ativos[idx].ativo) {
        _ativos[idx].ativo = true;
        _ativos[idx].tipo = AlarmType::TEMPERATURA;
        _ativos[idx].nivel = nivelTemp;
        _ativos[idx].timestampInicio = agora;
        _ativos[idx].reconhecido = false;
        result.tempMudou = true;
      } else if (nivelTemp > _ativos[idx].nivel) {
        _ativos[idx].nivel = nivelTemp;
        result.tempMudou = true;
      }
      result.tempAtivo = true;
    } else {
      if (_ativos[idx].ativo) {
        adicionarHistorico(AlarmType::TEMPERATURA,
                           _ativos[idx].nivel,
                           _ativos[idx].timestampInicio, agora,
                           _ativos[idx].reconhecido);
        _ativos[idx].ativo = false;
        _ativos[idx].nivel = AlarmLevel::NORMAL;
        result.tempMudou = true;
      }
      result.tempAtivo = false;
    }
  }

  // Sensor de vibracao (indice 1)
  {
    const uint8_t idx = 1;
    if (nivelVib != AlarmLevel::NORMAL) {
      if (!_ativos[idx].ativo) {
        _ativos[idx].ativo = true;
        _ativos[idx].tipo = AlarmType::VIBRACAO;
        _ativos[idx].nivel = nivelVib;
        _ativos[idx].timestampInicio = agora;
        _ativos[idx].reconhecido = false;
        result.vibMudou = true;
      } else if (nivelVib > _ativos[idx].nivel) {
        _ativos[idx].nivel = nivelVib;
        result.vibMudou = true;
      }
      result.vibAtivo = true;
    } else {
      if (_ativos[idx].ativo) {
        adicionarHistorico(AlarmType::VIBRACAO,
                           _ativos[idx].nivel,
                           _ativos[idx].timestampInicio, agora,
                           _ativos[idx].reconhecido);
        _ativos[idx].ativo = false;
        _ativos[idx].nivel = AlarmLevel::NORMAL;
        result.vibMudou = true;
      }
      result.vibAtivo = false;
    }
  }

  return result;
}

bool AlarmManager::reconhecer(AlarmType tipo) {
  const uint8_t idx = (uint8_t)tipo;
  if (idx > 1) return false;
  if (!_ativos[idx].ativo) return false;
  if (_ativos[idx].reconhecido) return false;
  _ativos[idx].reconhecido = true;
  return true;
}

uint8_t AlarmManager::quantidadeAtivos() const {
  uint8_t qtd = 0;
  for (uint8_t i = 0; i < 2; i++) {
    if (_ativos[i].ativo) qtd++;
  }
  return qtd;
}

const AlarmaAtivo* AlarmManager::obterAtivo(uint8_t indice) const {
  uint8_t encontrados = 0;
  for (uint8_t i = 0; i < 2; i++) {
    if (_ativos[i].ativo) {
      if (encontrados == indice) return &_ativos[i];
      encontrados++;
    }
  }
  return NULL;
}

uint8_t AlarmManager::quantidadeHistorico() const {
  return _qtdHistorico;
}

const AlarmaHistorico* AlarmManager::obterHistorico(
    uint8_t indice) const {
  if (indice >= _qtdHistorico) return NULL;
  return &_historico[indice];
}

void AlarmManager::carregarHistorico(
    const AlarmaHistorico* dados, uint8_t qtd) {
  _qtdHistorico = (qtd > MAX_HISTORICO) ? MAX_HISTORICO : qtd;
  for (uint8_t i = 0; i < _qtdHistorico; i++) {
    _historico[i] = dados[i];
  }
}

uint8_t AlarmManager::salvarHistorico(
    AlarmaHistorico* buffer) const {
  for (uint8_t i = 0; i < _qtdHistorico; i++) {
    buffer[i] = _historico[i];
  }
  return _qtdHistorico;
}

void AlarmManager::limparHistorico() {
  _qtdHistorico = 0;
  _idxHistorico = 0;
}

void AlarmManager::adicionarHistorico(
    AlarmType tipo, AlarmLevel nivel,
    uint32_t inicio, uint32_t fim, bool reconhecido) {
  _historico[_idxHistorico].timestampInicio = inicio;
  _historico[_idxHistorico].timestampFim = fim;
  _historico[_idxHistorico].tipo = tipo;
  _historico[_idxHistorico].nivel = nivel;
  _historico[_idxHistorico].foiReconhecido = reconhecido;

  _idxHistorico++;
  if (_idxHistorico >= MAX_HISTORICO) _idxHistorico = 0;

  if (_qtdHistorico < MAX_HISTORICO) _qtdHistorico++;
}
