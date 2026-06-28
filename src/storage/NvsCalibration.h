// =============================================================
//  Project SIGMA
//  Arquivo    : NvsCalibration.h
//  Descricao  : Persistencia NVS para offsets do acelerometro.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.5.1
//  Data       : 2026-06-27
// =============================================================

#pragma once

// -------------------------
//  Contrato de dados
// -------------------------
struct NvsCalibrationData {
  float offsetAX;
  float offsetAY;
  float offsetAZ;
  bool  calibrado;
};

class NvsCalibration {
public:
  bool salvar(const NvsCalibrationData& dados);
  bool carregar(NvsCalibrationData& dados);
  void apagar();
  bool existe();
};