// =============================================================
//  Project SIGMA
//  Arquivo    : NvsBoot.h
//  Descricao  : Persistencia do contador de boots (T017).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.18.0
//  Codename   : Runtime Diagnostics
//  Data       : 2026-07-01
// =============================================================

#pragma once

#include <stdint.h>

class NvsBoot {
public:
  uint32_t lerContador();
  void     incrementar();
  void     apagar();
};
