// =============================================================
//  Project SIGMA
//  Arquivo    : CommandHandler.h
//  Descricao  : Despacho e processamento de comandos seriais.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.15.0
//  Codename   : ProcessingTask
//  Data       : 2026-06-28
// =============================================================

#pragma once

#include "../storage/NvsHorimeter.h"
#include "../storage/NvsBaseline.h"
#include "../storage/NvsCalibration.h"
#include "../storage/NvsConfig.h"
#include "../drivers/Mpu6050Driver.h"
#include "../services/CalibrationService.h"
#include "SerialUI.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// -------------------------
//  Contexto de Comandos
// -------------------------
// Estrutura contendo referencias diretas aos objetos do sistema.
// Evita o uso de variaveis globais e mantem o desacoplamento.
struct CommandContext {
  NvsHorimeter&       nvsHor;
  NvsHorimeterData&   horData;
  NvsBaseline&        nvsBas;
  NvsBaselineData&    basData;
  NvsCalibration&     nvsCal;
  NvsCalibrationData& calData;
  NvsConfig&          nvsCfg;
  NvsConfigData&      cfgData;
  Mpu6050Driver&      driverVib;
  CalibrationService& srvCal;
  SerialUI&           ui;
  SemaphoreHandle_t   i2cMutex;
  uint32_t&           inicioSistemaMs;
};

// =============================================================
//  CLASSE: CommandHandler
//  Ouve a porta Serial e despacha as acoes correspondentes.
// =============================================================
class CommandHandler {
public:
  void processar(CommandContext& ctx);
  bool isCalibrating() const { return _calEstado != 0; }

private:
  uint8_t  _calEstado = 0;  // 0=IDLE, 1=CONFIRM, 2=SAMPLING, 3=DONE
  uint8_t  _calTent   = 0;
  uint32_t _calTimer  = 0;
  uint32_t _calSegTm  = 0;
  uint8_t  _calSegR   = 0;
};