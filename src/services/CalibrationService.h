// =============================================================
//  Project SIGMA
//  Arquivo    : CalibrationService.h
//  Descricao  : Servico de calibracao de offset do acelerometro.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.6
//  Codename   : Pausa Monitoramento Calibracao
//  Data       : 2026-06-29
// =============================================================

#pragma once

#include <stdint.h>
#include "../drivers/Mpu6050Driver.h"

// -------------------------
//  Contrato de Saida
// -------------------------
struct CalibrationResult {
  float offsetAX;
  float offsetAY;
  float offsetAZ;
  bool  sucesso;
};

// =============================================================
//  CLASSE: CalibrationService
//  Estado interno para calibracao nao-bloqueante.
//  Uso: chamar iniciar(), depois passo() a cada ciclo ate
//       que retorne true. Entao obterResultado().
// =============================================================
class CalibrationService {
public:
  static const int TOTAL_AMOSTRAS = 200;
  static const int INTERVALO_MS   = 25;

  CalibrationService();

  void iniciar();
  bool passo(Mpu6050Driver& driver);
  bool estaAtivo() const;
  int  obterProgresso() const;
  CalibrationResult obterResultado() const;

private:
  int     _indice;
  float   _somaX;
  float   _somaY;
  float   _somaZ;
  int     _amostrasValidas;
  uint32_t _ultimoMs;
  bool    _ativo;
};
