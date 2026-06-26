// =============================================================================
//  Project SIGMA - Sistema de Monitoramento Preditivo de Motor Industrial
//  Codinome   : Project SIGMA
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Sensores   : DS18B20 (temperatura) + MPU6050 (vibracao)
//  Protocolo  : Serial 115200 baud
//  Versao     : 0.1.2.2
// =============================================================================

// -------------------------
//  Bibliotecas necessarias
// -------------------------
#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------------------------
//  Definicao de pinos
// -------------------------
#define PINO_ONE_WIRE   4   // Pino de dados do DS18B20 (pull-up 4.7k obrigatorio)
#define PINO_MPU_SDA    8   // Pino SDA do barramento I2C
#define PINO_MPU_SCL    9   // Pino SCL do barramento I2C
#define PINO_MPU_INT   14   // Pino INT do MPU6050 (reservado para uso futuro)

// -------------------------
//  Configuracoes do sistema
// -------------------------
#define INTERVALO_LEITURA_MS    500     // Intervalo entre leituras (ms)
#define INTERVALO_MANUTENCAO_H  500.0  // Horas entre manutencoes preventivas
#define TEMP_AVISO_MIN          25.0   // Temperatura minima normal (oC)
#define TEMP_AVISO_MAX          55.0   // Temperatura maxima antes de aviso (oC)
#define TEMP_CRITICA            65.0   // Temperatura critica — alarme maximo (oC)

// VIB_AVISO e VIB_CRITICA sao variaveis globais para permitir ajuste em runtime
// via menu de sensibilidade (comando 'A'). Valores padrao para escala +/-8g.
float VIB_AVISO   = 2.0;  // Limiar de aviso em g — ajustavel via Serial
float VIB_CRITICA = 4.0;  // Limiar critico em g — ajustavel via Serial

// -------------------------
//  Forward declarations
// -------------------------
bool  executarCalibracao();
void  menuCalibracaoSetup();
void  menuCalibracaoLoop();
void  menuPosCalibracao();
bool  aguardarConfirmacaoEntrada(uint32_t timeoutMs);
bool  confirmarSaidaCalibracao();
void  menuSensibilidade();
float calcularVibracaoRMS();
float calcularHealthScore(float temp, float vib, float horas);
String classificarAlarme(float temp, float vib);
String obterClassificacaoHealth(float score);
void  imprimirRelatorio();

// -------------------------
//  Instancias dos sensores
// -------------------------
OneWire            barramento1Wire(PINO_ONE_WIRE);
DallasTemperature  sensorTemp(&barramento1Wire);
Adafruit_MPU6050   mpu;

// -------------------------
//  Variaveis de controle
// -------------------------
unsigned long ultimaLeitura = 0;
unsigned long inicioSistema = 0;
float temperatura  = 0.0;
float vibracaoRMS  = 0.0;
float horimetro    = 0.0;
float healthScore  = 0.0;
String statusAlarme = "";

// -------------------------
//  Offsets de calibracao
// -------------------------
float offsetAX  = 0.0;
float offsetAY  = 0.0;
float offsetAZ  = 0.0;
bool  calibrado    = false;
bool  emCalibracao = false;

// -------------------------
//  Configuracao de escala
// -------------------------
int escalaAtualG = 8; // Escala ativa: 2 / 4 / 8 / 16 g (padrao: 8g)

// =============================================================================
//  FUNCAO: executarCalibracao
//  Coleta 200 amostras em repouso e calcula os offsets de calibracao.
//  SEM menus, SEM confirmacoes, SEM prompts de usuario.
//  Permite cancelar com 'X' durante a coleta — offsets anteriores preservados.
//  Retorna: true = calibracao concluida / false = abortada pelo usuario
// =============================================================================
bool executarCalibracao() {
  const int NUM_AMOSTRAS = 200;
  float somaX = 0.0, somaY = 0.0, somaZ = 0.0;
  int   coletadas = 0;
  sensors_event_t accel, gyro, temp;

  Serial.println(F(""));
  Serial.println(F("  ==============================================="));
  Serial.println(F("  [CAL] INICIANDO CALIBRACAO DE PONTO ZERO"));
  Serial.println(F("  [CAL] Mantenha o sensor PARADO e NIVELADO!"));
  Serial.println(F("  [CAL] Envie 'X' para cancelar a qualquer momento."));
  Serial.println(F("  [CAL] Coletando amostras..."));

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.getEvent(&accel, &gyro, &temp);

    somaX += accel.acceleration.x / 9.81;
    somaY += accel.acceleration.y / 9.81;
    somaZ += accel.acceleration.z / 9.81;
    coletadas++;

    delay(5);

    if ((i + 1) % 50 == 0) {
      Serial.print(F("  [CAL] "));
      Serial.print(i + 1);
      Serial.print(F(" / "));
      Serial.print(NUM_AMOSTRAS);
      Serial.println(F(" amostras coletadas..."));
    }

    // Verifica se usuario quer cancelar
    if (Serial.available() > 0) {
      char tecla = Serial.read();
      if (tecla == 'X' || tecla == 'x') {
        Serial.println(F(""));
        Serial.print(F("  [CAL] Cancelamento solicitado apos "));
        Serial.print(coletadas);
        Serial.println(F(" amostras."));

        bool sair = confirmarSaidaCalibracao();
        if (sair) {
          Serial.println(F("  [CAL] Calibracao ABORTADA. Offsets anteriores mantidos."));
          if (!calibrado) {
            Serial.println(F("  [CAL] ATENCAO: Nenhuma calibracao ativa. Offsets = 0."));
          }
          Serial.println(F("  ==============================================="));
          emCalibracao = false;
          return false;
        }
        // Usuario decidiu continuar
      }
    }
  }

  // Calibracao concluida — aplica offsets globais
  offsetAX = somaX / NUM_AMOSTRAS;
  offsetAY = somaY / NUM_AMOSTRAS;
  offsetAZ = somaZ / NUM_AMOSTRAS; // Valor bruto — inclui ~1g de gravidade

  calibrado = true;

  Serial.println(F("  [CAL] Calibracao concluida com sucesso!"));
  Serial.println(F("  [CAL] ------- Offsets Calculados -------"));
  Serial.print(F("  [CAL]   Offset AX : ")); Serial.print(offsetAX, 5); Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AY : ")); Serial.print(offsetAY, 5); Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AZ : ")); Serial.print(offsetAZ, 5); Serial.println(F(" g  (bruto, inclui ~1g)"));
  Serial.println(F("  [CAL]   Az corrigido em calcularVibracaoRMS(): az - offsetAZ remove 1g"));
  Serial.println(F("  [CAL] Offsets aplicados em todas as leituras."));
  Serial.println(F("  ==============================================="));
  Serial.println();

  return true;
}

// =============================================================================
//  FUNCAO: menuCalibracaoSetup
//  Chamada exclusivamente no boot (setup()).
//  Executa calibracao automatica SEM nenhuma interacao com o usuario.
//  Ao concluir, prossegue direto para o restante do setup() e depois loop().
//  Nao exibe menus, nao pede confirmacao, nao aguarda input.
// =============================================================================
void menuCalibracaoSetup() {
  emCalibracao = true;

  Serial.println(F("  [SIGMA] Calibracao automatica de ponto zero iniciando..."));
  Serial.println(F("  [SIGMA] Mantenha o sensor PARADO e NIVELADO!"));
  Serial.println(F("  [SIGMA] Envie 'X' a qualquer momento para cancelar."));
  delay(2000);

  bool ok = executarCalibracao();

  if (!ok) {
    Serial.println(F("  [SIGMA] Calibracao cancelada no boot. Offsets = 0."));
    Serial.println(F("  [SIGMA] Envie 'C' para calibrar manualmente apos o inicio."));
    Serial.println();
  }

  emCalibracao = false; // Libera loop() incondicionalmente
}

// =============================================================================
//  FUNCAO: menuCalibracaoLoop
//  Chamada quando o usuario envia o comando 'C' durante o monitoramento.
//  Fluxo: confirmacao de entrada (S/N) → executarCalibracao() → menuPosCalibracao()
//  O usuario pode confirmar, cancelar ou recalibrar novamente.
// =============================================================================
void menuCalibracaoLoop() {
  emCalibracao = true;

  Serial.println(F(""));
  Serial.println(F("  [CMD] Comando de recalibracao recebido!"));
  Serial.println(F("  [CMD] Monitoramento pausado."));

  // Confirmacao de entrada — S/N com timeout de 30 segundos
  bool confirmado = aguardarConfirmacaoEntrada(30000);

  if (!confirmado) {
    Serial.println(F("  [CMD] Recalibracao cancelada. Monitoramento retomado."));
    Serial.println();
    emCalibracao = false;
    return;
  }

  // Executa calibracao
  bool ok = executarCalibracao();

  if (!ok) {
    Serial.println(F("  [INFO] Calibracao interrompida. Offsets anteriores mantidos."));
    Serial.println(F("  [INFO] Monitoramento retomado."));
    Serial.println();
    emCalibracao = false;
    return;
  }

  // Calibracao concluida — exibe menu pos-calibracao (S = Sair / R = Recalibrar)
  menuPosCalibracao();
}

// =============================================================================
//  FUNCAO: aguardarConfirmacaoEntrada
//  Exibe prompt interativo e aguarda S (confirmar) ou N (cancelar).
//  Timeout configuravel em milissegundos.
//  Retorna: true = confirmado / false = cancelado ou timeout
// =============================================================================
bool aguardarConfirmacaoEntrada(uint32_t timeoutMs) {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |          CONFIRMACAO NECESSARIA                  |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Deseja iniciar a calibracao de ponto zero?      |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  >> O sensor deve estar PARADO e NIVELADO!       |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  S = CONFIRMAR calibracao                        |"));
  Serial.println(F("  |  N = CANCELAR e voltar ao monitoramento          |"));
  Serial.println(F("  +--------------------------------------------------+"));

  uint32_t inicio           = millis();
  uint32_t segundosRestantes = timeoutMs / 1000;
  uint32_t ultimoSegundo    = millis();

  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s\r"));

  while ((millis() - inicio) < timeoutMs) {
    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read();
      Serial.println(F(""));
      if (r == 'S' || r == 's') {
        Serial.println(F("  [OK] Calibracao CONFIRMADA."));
        return true;
      }
      if (r == 'N' || r == 'n') {
        Serial.println(F("  [INFO] Calibracao CANCELADA pelo usuario."));
        return false;
      }
    }
    delay(50);
  }

  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 30s. Calibracao cancelada."));
  return false;
}

// =============================================================================
//  FUNCAO: confirmarSaidaCalibracao
//  Exibida quando usuario envia 'X' durante a coleta de amostras.
//  Pergunta se quer realmente abortar.
//  Retorna: true = confirma saida / false = continua calibrando
// =============================================================================
bool confirmarSaidaCalibracao() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |       CONFIRMAR SAIDA DA CALIBRACAO?             |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Amostras coletadas serao DESCARTADAS.           |"));
  Serial.println(F("  |  Offsets anteriores serao mantidos.              |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  S = SAIR da calibracao                          |"));
  Serial.println(F("  |  N = CONTINUAR coletando amostras                |"));
  Serial.println(F("  +--------------------------------------------------+"));

  const uint32_t TIMEOUT_MS  = 60000;
  uint32_t inicio            = millis();
  uint32_t segundosRestantes = TIMEOUT_MS / 1000;
  uint32_t ultimoSegundo     = millis();

  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s\r"));

  while ((millis() - inicio) < TIMEOUT_MS) {
    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read();
      Serial.println(F(""));
      if (r == 'S' || r == 's') {
        Serial.println(F("  [CAL] Saida CONFIRMADA."));
        return true;
      }
      if (r == 'N' || r == 'n') {
        Serial.println(F("  [CAL] Saida CANCELADA. Retomando coleta..."));
        return false;
      }
    }
    delay(50);
  }

  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 60s. Coleta retomada automaticamente."));
  return false;
}

// =============================================================================
//  FUNCAO: menuPosCalibracao
//  Exibida apos calibracao concluida via comando 'C' (modoManual).
//  S = sai ao monitoramento / R = recalibra novamente.
//  Timeout de 60 segundos: sai automaticamente ao monitoramento.
// =============================================================================
void menuPosCalibracao() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |        CALIBRACAO CONCLUIDA COM SUCESSO!           |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  S = SAIR e voltar ao monitoramento                |"));
  Serial.println(F("  |  R = RECALIBRAR novamente                          |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Dica: recalibre sempre com o sensor em repouso!   |"));
  Serial.println(F("  +----------------------------------------------------+"));

  const uint32_t TIMEOUT_MS  = 60000;
  uint32_t inicio            = millis();
  uint32_t segundosRestantes = TIMEOUT_MS / 1000;
  uint32_t ultimoSegundo     = millis();

  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s\r"));

  while ((millis() - inicio) < TIMEOUT_MS) {
    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read();
      Serial.println(F(""));
      if (r == 'S' || r == 's') {
        Serial.println(F("  [MENU] Saindo. Monitoramento retomado com novos offsets."));
        Serial.println();
        emCalibracao = false;
        return;
      }
      if (r == 'R' || r == 'r') {
        Serial.println(F("  [MENU] Iniciando nova calibracao..."));
        Serial.println(F("  [MENU] Mantenha o sensor PARADO e NIVELADO!"));
        delay(1000);
        bool ok = executarCalibracao();
        if (ok) {
          menuPosCalibracao(); // Exibe menu novamente apos nova calibracao
        } else {
          Serial.println(F("  [INFO] Calibracao interrompida. Offsets anteriores mantidos."));
          Serial.println();
          emCalibracao = false;
        }
        return;
      }
    }
    delay(50);
  }

  // Timeout — sai automaticamente
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 60s. Monitoramento retomado."));
  Serial.println();
  emCalibracao = false;
}

// =============================================================================
//  FUNCAO: menuSensibilidade
//  Acessado via comando 'A'. Permite ajustar escala e limiares do acelerometro.
//  Escalas: 2g / 4g / 8g / 16g. Limiares: VIB_AVISO e VIB_CRITICA.
//  Apos mudanca de escala, recalibra automaticamente sem menus.
// =============================================================================
void menuSensibilidade() {
  emCalibracao = true;
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +====================================================+"));
  Serial.println(F("  |       MENU DE SENSIBILIDADE DO ACELEROMETRO        |"));
  Serial.println(F("  +====================================================+"));
  Serial.print(F(  "  |  Escala atual     : +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g                         |"));
  Serial.print(F(  "  |  VIB_AVISO atual  : ")); Serial.print(VIB_AVISO, 2); Serial.println(F(" g                             |"));
  Serial.print(F(  "  |  VIB_CRITICA atual: ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g                             |"));
  Serial.println(F("  |----------------------------------------------------|"));
  Serial.println(F("  |  1 = +/-  2g  Alta sens.  — bancada / laboratorio  |"));
  Serial.println(F("  |  2 = +/-  4g  Media sens. — motores pequenos       |"));
  Serial.println(F("  |  3 = +/-  8g  Uso geral   — industrial medio       |"));
  Serial.println(F("  |  4 = +/- 16g  Baixa sens. — motores pesados        |"));
  Serial.println(F("  |  M = Ajuste MANUAL dos limiares de alarme          |"));
  Serial.println(F("  |  S = SAIR sem alterar configuracao                 |"));
  Serial.println(F("  +====================================================+"));

  const uint32_t TIMEOUT_MS  = 60000;
  uint32_t inicio            = millis();
  uint32_t segundosRestantes = TIMEOUT_MS / 1000;
  uint32_t ultimoSegundo     = millis();
  bool     menuAtivo         = true;

  Serial.print(F("  Aguardando selecao... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s\r"));

  while (menuAtivo && (millis() - inicio) < TIMEOUT_MS) {
    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando selecao... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }

    if (Serial.available() > 0) {
      char opcao = Serial.read();
      Serial.println(F(""));

      if (opcao == 'S' || opcao == 's') {
        Serial.println(F("  [SENS] Configuracao mantida. Saindo do menu..."));
        menuAtivo = false;
        break;
      }

      // Selecao de escala
      mpu6050_accel_range_t novaEscalaEnum;
      int   novaEscalaG     = 0;
      float sugestaoAviso   = 0.0;
      float sugestaoCritica = 0.0;
      bool  escalaValida    = false;

      if      (opcao == '1') { novaEscalaG = 2;  novaEscalaEnum = MPU6050_RANGE_2_G;  sugestaoAviso = 0.5; sugestaoCritica = 1.0;  escalaValida = true; }
      else if (opcao == '2') { novaEscalaG = 4;  novaEscalaEnum = MPU6050_RANGE_4_G;  sugestaoAviso = 1.0; sugestaoCritica = 2.0;  escalaValida = true; }
      else if (opcao == '3') { novaEscalaG = 8;  novaEscalaEnum = MPU6050_RANGE_8_G;  sugestaoAviso = 2.0; sugestaoCritica = 4.0;  escalaValida = true; }
      else if (opcao == '4') { novaEscalaG = 16; novaEscalaEnum = MPU6050_RANGE_16_G; sugestaoAviso = 4.0; sugestaoCritica = 8.0;  escalaValida = true; }

      if (escalaValida) {
        Serial.print(F("  [SENS] Nova escala: +/- ")); Serial.print(novaEscalaG); Serial.println(F(" g"));
        Serial.print(F("  [SENS] Limiares sugeridos — Aviso: ")); Serial.print(sugestaoAviso, 2);
        Serial.print(F(" g  |  Critico: ")); Serial.print(sugestaoCritica, 2); Serial.println(F(" g"));
        Serial.println(F("  S = Aplicar sugestao  |  M = Manual  |  N = Manter limiares atuais"));

        while (Serial.available()) Serial.read();
        uint32_t inicioLimiar = millis();
        bool aguardando = true;

        while (aguardando && (millis() - inicioLimiar) < 30000) {
          if (Serial.available() > 0) {
            char res = Serial.read();
            Serial.println(F(""));
            if (res == 'S' || res == 's') {
              VIB_AVISO = sugestaoAviso; VIB_CRITICA = sugestaoCritica;
              Serial.println(F("  [SENS] Limiares sugeridos aplicados."));
              aguardando = false;
            } else if (res == 'M' || res == 'm') {
              Serial.println(F("  [SENS] Digite o novo VIB_AVISO e pressione Enter:"));
              while (Serial.available()) Serial.read();
              String sAviso = ""; uint32_t t = millis();
              while ((millis() - t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c == '\n' || c == '\r') break; sAviso += c; } }
              Serial.println(F("  [SENS] Digite o novo VIB_CRITICA e pressione Enter:"));
              while (Serial.available()) Serial.read();
              String sCritica = ""; t = millis();
              while ((millis() - t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c == '\n' || c == '\r') break; sCritica += c; } }
              float nA = sAviso.toFloat(), nC = sCritica.toFloat();
              if (nA > 0.0 && nC > 0.0 && nA < nC) { VIB_AVISO = nA; VIB_CRITICA = nC; Serial.println(F("  [SENS] Limiares manuais aplicados.")); }
              else { Serial.println(F("  [SENS] ERRO: valores invalidos. Limiares nao alterados.")); }
              aguardando = false;
            } else if (res == 'N' || res == 'n') {
              Serial.println(F("  [SENS] Limiares mantidos.")); aguardando = false;
            }
          }
          delay(50);
        }
        if (aguardando) {
          VIB_AVISO = sugestaoAviso; VIB_CRITICA = sugestaoCritica;
          Serial.println(F("  [TIMEOUT] Limiares sugeridos aplicados automaticamente."));
        }

        // Aplica nova escala no hardware
        mpu.setAccelerometerRange(novaEscalaEnum);
        escalaAtualG = novaEscalaG;
        Serial.print(F("  [SENS] Escala aplicada: +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));

        // Recalibra automaticamente — sem menus
        Serial.println(F("  [SENS] Mudanca de escala invalida offsets. Recalibrando..."));
        Serial.println(F("  [SENS] Mantenha o sensor PARADO e NIVELADO!"));
        delay(2000);
        calibrado = false; offsetAX = 0.0; offsetAY = 0.0; offsetAZ = 0.0;
        executarCalibracao(); // Sem menu pos — direto
        menuAtivo = false;

      } else if (opcao == 'M' || opcao == 'm') {
        // Ajuste manual sem mudar escala
        Serial.println(F("  [SENS] Ajuste manual de limiares (escala mantida)."));
        Serial.print(F("  [SENS] VIB_AVISO atual: ")); Serial.print(VIB_AVISO, 2); Serial.println(F(" g"));
        Serial.println(F("  [SENS] Digite o novo VIB_AVISO e pressione Enter:"));
        while (Serial.available()) Serial.read();
        String sA = ""; uint32_t t = millis();
        while ((millis() - t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c == '\n' || c == '\r') break; sA += c; } }
        Serial.print(F("  [SENS] VIB_CRITICA atual: ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
        Serial.println(F("  [SENS] Digite o novo VIB_CRITICA e pressione Enter:"));
        while (Serial.available()) Serial.read();
        String sC = ""; t = millis();
        while ((millis() - t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c == '\n' || c == '\r') break; sC += c; } }
        float nA = sA.toFloat(), nC = sC.toFloat();
        if (nA > 0.0 && nC > 0.0 && nA < nC) { VIB_AVISO = nA; VIB_CRITICA = nC; Serial.println(F("  [SENS] Limiares aplicados.")); }
        else { Serial.println(F("  [SENS] ERRO: valores invalidos.")); }
        menuAtivo = false;

      } else {
        Serial.println(F("  [SENS] Opcao invalida. Digite 1, 2, 3, 4, M ou S."));
      }
    }
    delay(50);
  }

  if (menuAtivo) {
    Serial.println(F(""));
    Serial.println(F("  [TIMEOUT] Sem selecao em 60s. Menu encerrado sem alteracoes."));
  }

  Serial.println(F(""));
  Serial.println(F("  [SENS] -------- Configuracao Ativa --------"));
  Serial.print(F(  "  [SENS]   Escala      : +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
  Serial.print(F(  "  [SENS]   VIB_AVISO   : ")); Serial.print(VIB_AVISO, 2); Serial.println(F(" g"));
  Serial.print(F(  "  [SENS]   VIB_CRITICA : ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
  Serial.println(F("  [SENS] Monitoramento retomado."));
  Serial.println();

  emCalibracao = false;
}

// =============================================================================
//  FUNCAO: calcularVibracaoRMS
//  Le 50 amostras do MPU6050, aplica offsets e calcula magnitude RMS.
//  offsetAZ contem ~1g bruto — ao subtrair, a gravidade e removida uma unica vez.
//  Retorna: vibracao resultante em g
// =============================================================================
float calcularVibracaoRMS() {
  const int NUM_AMOSTRAS = 50;
  float somaQ = 0.0;
  sensors_event_t accel, gyro, temp;

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.getEvent(&accel, &gyro, &temp);
    float ax = (accel.acceleration.x / 9.81) - offsetAX;
    float ay = (accel.acceleration.y / 9.81) - offsetAY;
    float az = (accel.acceleration.z / 9.81) - offsetAZ; // Remove ~1g via offset bruto
    somaQ += (ax * ax) + (ay * ay) + (az * az);
    delay(2);
  }
  return sqrt(somaQ / NUM_AMOSTRAS);
}

// =============================================================================
//  FUNCAO: calcularHealthScore
//  Pontuacao de saude ponderada: Temp 40% + Vibracao 40% + Horimetro 20%
//  Retorna: 0.0 a 100.0
// =============================================================================
float calcularHealthScore(float temp, float vib, float horas) {
  float sT, sV, sH;

  // Componente temperatura
  if      (temp <= TEMP_AVISO_MAX) sT = 1.0;
  else if (temp >= TEMP_CRITICA)   sT = 0.0;
  else sT = 1.0 - ((temp - TEMP_AVISO_MAX) / (TEMP_CRITICA - TEMP_AVISO_MAX));

  // Componente vibracao
  if      (vib <= VIB_AVISO)   sV = 1.0;
  else if (vib >= VIB_CRITICA) sV = 0.0;
  else sV = 1.0 - ((vib - VIB_AVISO) / (VIB_CRITICA - VIB_AVISO));

  // Componente horimetro
  float limDeg = INTERVALO_MANUTENCAO_H * 0.80;
  if      (horas <= limDeg)                  sH = 1.0;
  else if (horas >= INTERVALO_MANUTENCAO_H)  sH = 0.0;
  else sH = 1.0 - ((horas - limDeg) / (INTERVALO_MANUTENCAO_H - limDeg));

  return (sT * 40.0) + (sV * 40.0) + (sH * 20.0);
}

// =============================================================================
//  FUNCAO: classificarAlarme
//  CRITICO > AVISO > NORMAL
// =============================================================================
String classificarAlarme(float temp, float vib) {
  if (temp >= TEMP_CRITICA || vib >= VIB_CRITICA) return "*** CRITICO ***";
  if (temp >= TEMP_AVISO_MAX || vib >= VIB_AVISO) return "    AVISO    ";
  return "    NORMAL   ";
}

// =============================================================================
//  FUNCAO: obterClassificacaoHealth
// =============================================================================
String obterClassificacaoHealth(float score) {
  if (score >= 90.0) return "Excelente";
  if (score >= 70.0) return "Bom";
  if (score >= 50.0) return "Manutencao Recomendada";
  return "CONDICAO CRITICA";
}

// =============================================================================
//  FUNCAO: imprimirRelatorio
//  Imprime no Serial Monitor o relatorio completo de status do motor.
// =============================================================================
void imprimirRelatorio() {
  float horasRestantes = INTERVALO_MANUTENCAO_H - horimetro;
  if (horasRestantes < 0) horasRestantes = 0;

  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Timestamp       : ")); Serial.print(millis()); Serial.println(F(" ms"));
  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Temperatura     : ")); Serial.print(temperatura, 1); Serial.println(F(" oC"));
  Serial.print(F("  Vibracao        : ")); Serial.print(vibracaoRMS, 3); Serial.println(F(" g"));
  Serial.print(F("  Horimetro       : ")); Serial.print(horimetro, 4); Serial.println(F(" h"));
  Serial.print(F("  Prox. Manut.    : ")); Serial.print(horasRestantes, 1); Serial.println(F(" h restantes"));
  Serial.print(F("  Health Score    : ")); Serial.print(healthScore, 1); Serial.print(F(" %  (")); Serial.print(obterClassificacaoHealth(healthScore)); Serial.println(F(")"));
  Serial.print(F("  Escala Acel.    : +/- ")); Serial.print(escalaAtualG); Serial.print(F(" g  |  Aviso: ")); Serial.print(VIB_AVISO, 2); Serial.print(F(" g  |  Critico: ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
  Serial.print(F("  Alarme          : ")); Serial.println(statusAlarme);
  Serial.println(F("------------------------------------------------"));
  Serial.println();
}

// =============================================================================
//  SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("=============================================="));
  Serial.println(F("  Project SIGMA v0.1.2.3 - Iniciando..."));
  Serial.println(F("=============================================="));

  // I2C
  Wire.begin(PINO_MPU_SDA, PINO_MPU_SCL);
  Wire.setClock(400000);
  delay(150);
  Serial.println(F("  [I2C] Barramento iniciado (400 kHz Fast Mode)."));

  // MPU6050 — ate 5 tentativas
  bool mpuOk = false;
  for (int t = 1; t <= 5; t++) {
    Serial.print(F("  [I2C] Tentativa ")); Serial.print(t); Serial.print(F(" de 5 - procurando MPU6050 (0x68)..."));
    if (mpu.begin()) { mpuOk = true; Serial.println(F(" ENCONTRADO!")); break; }
    Serial.println(F(" nao respondeu.")); delay(200);
  }

  if (!mpuOk) {
    Serial.println(F("  [ERRO FATAL] MPU6050 nao encontrado!"));
    Serial.println(F("  Checklist:"));
    Serial.println(F("    1. Pull-up 4.7k no SDA (GPIO8) ao 3.3V?"));
    Serial.println(F("    2. Pull-up 4.7k no SCL (GPIO9) ao 3.3V?"));
    Serial.println(F("    3. VCC MPU6050 = 3.3V?"));
    Serial.println(F("    4. GND MPU6050 = GND?"));
    Serial.println(F("    5. AD0 no GND = 0x68 | AD0 no 3.3V = 0x69"));
    Serial.println(F("  Sistema pausado. Corrija e reinicie."));
    while (true) delay(1000);
  }
  Serial.println(F("  [OK]  MPU6050 inicializado."));
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println(F("  [MPU] Escala: +/- 8g"));
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.println(F("  [MPU] Giroscopio: +/- 500 deg/s"));
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println(F("  [MPU] Filtro passa-baixa: 21 Hz"));

  // Calibracao automatica no boot — sem interacao com usuario
  menuCalibracaoSetup();

  // DS18B20
  sensorTemp.begin();
  int qtd = sensorTemp.getDeviceCount();
  Serial.print(F("  [DS18B20] Sensores encontrados: ")); Serial.println(qtd);
  if (qtd == 0) { Serial.println(F("  [AVISO] Nenhum DS18B20 detectado!")); }
  else { sensorTemp.setResolution(12); Serial.println(F("  [DS18B20] Resolucao: 12 bits")); }

  // Inicia horimetro APOS toda a inicializacao
  inicioSistema = millis();
  ultimaLeitura = millis();

  Serial.println(F("=============================================="));
  Serial.println(F("  Sistema pronto. Monitoramento iniciado."));
  Serial.println(F("  Comandos: C = Recalibrar | A = Sensibilidade"));
  Serial.println(F("=============================================="));
  Serial.println();
}

// =============================================================================
//  LOOP
// =============================================================================
void loop() {
  unsigned long agora = millis();

  // Verifica comandos Serial
  if (!emCalibracao && Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'C' || cmd == 'c') {
      menuCalibracaoLoop(); // Confirmacao + calibracao + menu pos
    }

    if (cmd == 'A' || cmd == 'a') {
      Serial.println(F(""));
      Serial.println(F("  [CMD] Menu de sensibilidade ativado!"));
      menuSensibilidade();
    }
  }

  // Leitura dos sensores a cada INTERVALO_LEITURA_MS
  if (!emCalibracao && (agora - ultimaLeitura) >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora;

    // Horimetro
    horimetro = (float)(agora - inicioSistema) / 3600000.0;

    // Temperatura
    sensorTemp.requestTemperatures();
    float lT = sensorTemp.getTempCByIndex(0);
    if (lT != DEVICE_DISCONNECTED_C) temperatura = lT;
    else Serial.println(F("  [AVISO] Falha na leitura do DS18B20!"));

    // Vibracao
    vibracaoRMS = calcularVibracaoRMS();

    // Health Score e Alarme
    healthScore  = calcularHealthScore(temperatura, vibracaoRMS, horimetro);
    statusAlarme = classificarAlarme(temperatura, vibracaoRMS);

    // Relatorio
    imprimirRelatorio();
  }
}
