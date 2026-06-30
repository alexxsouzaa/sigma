// =============================================================
//  Project SIGMA
//  Arquivo    : Version.h
//  Descricao  : Configuracoes globais e versoes do firmware.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.3
//  Codename   : Calibracao Nao-Bloqueante
//  Data       : 2026-06-28
// =============================================================

#pragma once

// -------------------------
//  Versao e Release
// -------------------------
#define SIGMA_FIRMWARE_VERSION "0.1.7.3"
#define SIGMA_CODENAME         "Calibracao Nao-Bloqueante"

// -------------------------
//  Analytics Engine
// -------------------------
// (300 amostras x ~24 bytes = ~7.2 KB na SRAM)
#define ANALYTICS_BUFFER_SIZE  300