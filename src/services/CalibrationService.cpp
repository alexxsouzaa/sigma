// =============================================================
//  Project SIGMA
//  Arquivo    : CalibrationService.cpp
//  Descricao  : Implementacao do calculo de offsets.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.4
//  Codename   : Bugfixes e Limpeza
//  Data       : 2026-06-28
// =============================================================

#include "CalibrationService.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

// =============================================================
//  FUNCAO: executar
//  Coleta 200 amostras, calcula a media e devolve os offsets.
// =============================================================
CalibrationResult CalibrationService::executar(Mpu6050Driver& driver, 
                                               ProgressCallback cb, 
                                               void* context) {
  CalibrationResult res = { 0.0f, 0.0f, 0.0f, false };
  float somaX = 0.0f, somaY = 0.0f, somaZ = 0.0f;
  int amostrasValidas = 0;

  for (int i = 1; i <= 200; i++) {
    esp_task_wdt_reset(); // Evita reset durante operacao longa
    
    // Leitura bruta (sem offsets aplicados)
    Mpu6050Data bruto = driver.lerAceleracao(0.0f, 0.0f, 0.0f);
    
    if (bruto.valido) {
      somaX += bruto.ax;
      somaY += bruto.ay;
      somaZ += bruto.az;
      amostrasValidas++;
    }
    
    // Aciona a UI a cada 10 iteracoes ou no termino
    if (cb != nullptr && (i % 10 == 0 || i == 200)) {
      cb(context, i, 200);
    }
    
    delay(5); // Estabilizacao temporal do I2C
  }

  if (amostrasValidas > 0) {
    res.offsetAX = somaX / amostrasValidas;
    res.offsetAY = somaY / amostrasValidas;
    res.offsetAZ = somaZ / amostrasValidas;
    res.sucesso  = true;
  }

  return res;
}