// =============================================================
//  Project SIGMA
//  Arquivo    : SerialUI.cpp
//  Descricao  : Implementacao da Interface Serial (SUS v2.1.0).
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.6.0
//  Data       : 2026-06-27
// =============================================================

#include "SerialUI.h"
#include <esp_task_wdt.h>

void SerialUI::begin(uint32_t baudRate) {
  Serial.begin(baudRate);
  delay(500); // Aguarda estabilizacao do terminal
}

// =============================================================
//  FUNCAO: imprimirBarraProgresso
//  Renderiza barra ASCII atualizada via \r. Alimenta WDT.
// =============================================================
void SerialUI::imprimirBarraProgresso(int atual, int total) {
  int pct   = (atual * 100) / total;
  int cheio = (atual * 30)  / total;

  Serial.print(F("  ["));
  for (int i = 0; i < 30; i++) {
    Serial.print(i < cheio ? '#' : '-');
  }
  Serial.print(F("] "));
  Serial.print(pct);
  Serial.print(F("%  "));
  Serial.print(atual);
  Serial.print(F(" / "));
  Serial.print(total);
  Serial.print(F(" \r"));

  esp_task_wdt_reset();
}

// =============================================================
//  FUNCAO: aguardarConfirmacao
//  Aguarda [S] ou [N] com timeout decrescente na mesma linha.
// =============================================================
bool SerialUI::aguardarConfirmacao(const char* msgS, const char* msgN, uint32_t timeoutMs) {
  while (Serial.available()) Serial.read();

  uint32_t inicio = millis();
  uint32_t segs   = timeoutMs / 1000;
  uint32_t ultSeg = millis();

  Serial.println(F(""));
  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segs);
  Serial.print(F(" s\r"));

  while ((millis() - inicio) < timeoutMs) {
    esp_task_wdt_reset();
    
    if ((millis() - ultSeg) >= 1000) {
      ultSeg = millis();
      segs--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segs);
      Serial.print(F(" s   \r"));
    }

    if (Serial.available() > 0) {
      char r = Serial.read();
      Serial.println(F(""));
      if (r == 'S' || r == 's') {
        imprimirMensagem("INFO", msgS);
        return true;
      }
      if (r == 'N' || r == 'n') {
        imprimirMensagem("INFO", msgN);
        return false;
      }
    }
    delay(50);
  }

  Serial.println(F(""));
  imprimirMensagem("TIMEOUT", "Sem resposta. Operacao cancelada.");
  return false;
}

void SerialUI::imprimirMensagem(const char* prefixo, const char* mensagem) {
  Serial.print(F("  ["));
  Serial.print(prefixo);
  Serial.print(F("] "));
  Serial.println(mensagem);
}

void SerialUI::imprimirCabecalhoBoot() {
  Serial.println(F(""));
  Serial.println(F(LINHA_SEP));
  Serial.print(F(" Project SIGMA v"));
  Serial.print(F(SIGMA_FIRMWARE_VERSION));
  Serial.println(F(" - Iniciando..."));
  Serial.println(F(LINHA_SEP));
}

void SerialUI::imprimirErroFatal(const char* erro, const char* acao) {
  Serial.print(F("  [ERRO] ")); Serial.println(erro);
  Serial.print(F("  [ERRO] ")); Serial.println(acao);
  Serial.println(F(LINHA_SEP));
}

void SerialUI::imprimirRelatorio(uint32_t tempoMs, float temp, float vib, float horas, 
                                 float score, const char* clHealth, const char* clAlarme) {
  Serial.println(F(LINHA_SUB));
  Serial.print(F("  Timestamp       : ")); Serial.print(tempoMs); Serial.println(F(" ms"));
  Serial.println(F(LINHA_SUB));
  
  Serial.print(F("  Temperatura     : ")); Serial.print(temp, 1); Serial.println(F(" oC"));
  Serial.print(F("  Vibracao RMS    : ")); Serial.print(vib, 3); Serial.println(F(" g"));
  Serial.print(F("  Horimetro       : ")); Serial.print(horas, 1); Serial.println(F(" h"));
  Serial.print(F("  Health Score    : ")); Serial.print(score, 1); Serial.print(F(" % (")); Serial.print(clHealth); Serial.println(F(")"));
  Serial.print(F("  Alarme          : ")); Serial.println(clAlarme);
  Serial.println(F(LINHA_SUB));
  Serial.println();
}