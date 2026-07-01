// =============================================================
//  Project SIGMA
//  Arquivo    : CalibrationService.cpp
//  Descricao  : Implementacao da calibracao nao-bloqueante.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.15.0
//  Codename   : ProcessingTask
//  Data       : 2026-06-29
// =============================================================

#include "CalibrationService.h"
#include <Arduino.h>

// =============================================================
//  CONSTRUTOR
// =============================================================
CalibrationService::CalibrationService() {
  _indice = 0;
  _somaX = 0.0f;
  _somaY = 0.0f;
  _somaZ = 0.0f;
  _amostrasValidas = 0;
  _ultimoMs = 0;
  _ativo = false;
}

// =============================================================
//  FUNCAO: iniciar
//  Reseta acumuladores e marca calibracao como ativa.
// =============================================================
void CalibrationService::iniciar() {
  _indice = 0;
  _somaX = 0.0f;
  _somaY = 0.0f;
  _somaZ = 0.0f;
  _amostrasValidas = 0;
  _ultimoMs = 0;
  _ativo = true;
}

// =============================================================
//  FUNCAO: passo
//  Le uma amostra do sensor se o intervalo minimo tiver
//  passado. Avanca apenas em leituras validas.
//  Retorna true quando as 200 amostras forem coletadas.
//  Nunca bloqueia.
// =============================================================
bool CalibrationService::passo(Mpu6050Driver& driver) {
  if (!_ativo) return true;

  uint32_t agora = millis();
  if (agora - _ultimoMs < INTERVALO_MS) return false;
  _ultimoMs = agora;

  // Leitura bruta (sem offsets). Se falhar, tenta de novo
  // no proximo ciclo (retorno false sem incrementar).
  Mpu6050Data bruto = driver.lerAceleracao(0.0f, 0.0f, 0.0f);
  if (!bruto.valido) return false;

  _somaX += bruto.ax;
  _somaY += bruto.ay;
  _somaZ += bruto.az;
  _indice++;

  if (_indice >= TOTAL_AMOSTRAS) {
    _ativo = false;
    _amostrasValidas = _indice;
    return true;
  }

  return false;
}

// =============================================================
//  FUNCAO: estaAtivo
// =============================================================
bool CalibrationService::estaAtivo() const {
  return _ativo;
}

// =============================================================
//  FUNCAO: obterProgresso
//  Retorna o numero de amostras coletadas (0..200).
// =============================================================
int CalibrationService::obterProgresso() const {
  return _indice;
}

// =============================================================
//  FUNCAO: obterResultado
//  Calcula as medias das amostras validas coletadas.
//  Chame apenas depois que passo() retornar true.
// =============================================================
CalibrationResult CalibrationService::obterResultado() const {
  CalibrationResult res = { 0.0f, 0.0f, 0.0f, false };

  if (_amostrasValidas > 0) {
    res.offsetAX = _somaX / _amostrasValidas;
    res.offsetAY = _somaY / _amostrasValidas;
    res.offsetAZ = _somaZ / _amostrasValidas;
    res.sucesso  = true;
  }

  return res;
}
