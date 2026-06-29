// =============================================================
//  Project SIGMA
//  Arquivo    : CommandHandler.h
//  Descricao  : Despacho e processamento de comandos seriais.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.4
//  Codename   : Bugfixes e Limpeza
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
  CalibrationService& srvCal; // <--- Injetado
  SerialUI&           ui;
  uint32_t&           inicioSistemaMs;
};

// =============================================================
//  CLASSE: CommandHandler
//  Ouve a porta Serial e despacha as acoes correspondentes.
// =============================================================
class CommandHandler {
public:
  void processar(CommandContext& ctx);
};