// =============================================================
//  Project SIGMA
//  Arquivo    : PinConfig.h
//  Descricao  : Definicao de pinos de hardware (Camada 1).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.9.0
//  Codename   : Qualidade dos Sensores
//  Data       : 2026-06-27
// =============================================================

#pragma once

// =============================================================
//  DEFINICAO DE PINOS
// =============================================================

#define PINO_ONE_WIRE  4   // (GPIO) DATA do DS18B20
#define PINO_MPU_SDA   8   // (GPIO) SDA do barramento I2C
#define PINO_MPU_SCL   9   // (GPIO) SCL do barramento I2C
#define PINO_MPU_INT   14  // (GPIO) INT MPU6050 (reservado)