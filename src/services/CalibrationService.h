// =============================================================
//  Project SIGMA
//  Arquivo    : CalibrationService.h
//  Descricao  : Servico de calibracao de offset do acelerometro.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.1
//  Codename   : Remocao de Gravidade
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include "../drivers/Mpu6050Driver.h"

// Callback para atualizacao de progresso sem acoplar a UI
typedef void (*ProgressCallback)(void* ctx, int atual, int total);

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
//  Isola a regra de negocio da coleta de ponto zero.
// =============================================================
class CalibrationService {
public:
  CalibrationResult executar(Mpu6050Driver& driver, 
                             ProgressCallback cb, 
                             void* context);
};