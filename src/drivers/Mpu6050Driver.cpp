// =============================================================
//  Project SIGMA
//  Arquivo    : Mpu6050Driver.cpp
//  Descricao  : Implementacao do driver MPU6050.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.1
//  Data       : 2026-06-27
// =============================================================

#include "Mpu6050Driver.h"

// =============================================================
//  FUNCAO: begin
//  Verifica resposta do sensor no barramento I2C.
// -------------------------------------------------------------
//  Retorno  : true se sensor encontrado e inicializado.
//  Observ.  : I2C (Wire) deve ser inicializado antes.
// =============================================================
bool Mpu6050Driver::begin() {
  return _mpu.begin();
}

// =============================================================
//  FUNCAO: configurarEscala
//  Aplica filtros de hardware e define escala de medicao.
// -------------------------------------------------------------
//  Parametros:
//    escalaG : Escala desejada (2, 4, 8 ou 16).
// =============================================================
void Mpu6050Driver::configurarEscala(int escalaG) {
  mpu6050_accel_range_t escEnum = MPU6050_RANGE_8_G;
  
  if      (escalaG ==  2) escEnum = MPU6050_RANGE_2_G;
  else if (escalaG ==  4) escEnum = MPU6050_RANGE_4_G;
  else if (escalaG == 16) escEnum = MPU6050_RANGE_16_G;
  
  _mpu.setAccelerometerRange(escEnum);
  _mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

// =============================================================
//  FUNCAO: lerAceleracao
//  Coleta evento e converte m/s^2 para g, subtraindo offset.
// -------------------------------------------------------------
//  Parametros:
//    offX : Offset de calibracao X em g.
//    offY : Offset de calibracao Y em g.
//    offZ : Offset de calibracao Z em g (inclui ~1g gravidade).
//  Retorno  : Struct Mpu6050Data com dados liquidos em g.
// =============================================================
Mpu6050Data Mpu6050Driver::lerAceleracao(float offX, float offY, float offZ) {
  Mpu6050Data dado = { 0.0f, 0.0f, 0.0f, false };
  sensors_event_t accel, gyro, temp;

  if (_mpu.getEvent(&accel, &gyro, &temp)) {
    dado.ax = (accel.acceleration.x / 9.81f) - offX;
    dado.ay = (accel.acceleration.y / 9.81f) - offY;
    dado.az = (accel.acceleration.z / 9.81f) - offZ;
    dado.valido = true;
  }
  
  return dado;
}