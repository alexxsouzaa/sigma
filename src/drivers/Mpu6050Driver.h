// =============================================================
//  Project SIGMA
//  Arquivo    : Mpu6050Driver.h
//  Descricao  : Driver de encapsulamento do sensor MPU6050.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.4
//  Codename   : Calibracao Passo Fix
//  Data       : 2026-06-27
// =============================================================

#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------------------------
//  Retorno de dados
// -------------------------
struct Mpu6050Data {
  float ax;     // (g) Aceleracao X (com offset aplicado)
  float ay;     // (g) Aceleracao Y (com offset aplicado)
  float az;     // (g) Aceleracao Z (com offset aplicado)
  bool  valido; // true se leitura foi bem-sucedida
};

class Mpu6050Driver {
private:
  Adafruit_MPU6050 _mpu;

public:
  bool        begin();
  void        configurarEscala(int escalaG);
  Mpu6050Data lerAceleracao(float offX, float offY, float offZ);
};