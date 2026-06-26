// =============================================================================
//  Project SIGMA - Sistema de Monitoramento Preditivo de Motor Industrial
//  Codinome   : Project SIGMA
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Sensores   : DS18B20 (temperatura) + MPU6050 (vibracao)
//  Protocolo  : Serial 115200 baud
//  Versao     : 0.1.3.3
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
#include <Preferences.h>

// -------------------------
//  Definicao de pinos
// -------------------------
#define PINO_ONE_WIRE   4
#define PINO_MPU_SDA    8
#define PINO_MPU_SCL    9
#define PINO_MPU_INT   14

// -------------------------
//  Configuracoes do sistema
// -------------------------
#define INTERVALO_LEITURA_MS        500
#define INTERVALO_MANUTENCAO_H      500.0
#define INTERVALO_SAVE_HORIMETRO_MS 300000UL
#define TEMP_AVISO_MIN              25.0
#define TEMP_AVISO_MAX              55.0
#define TEMP_CRITICA                65.0

float VIB_AVISO   = 2.0;
float VIB_CRITICA = 4.0;

// -------------------------
//  Forward declarations
// -------------------------
bool   executarCalibracao();
void   menuCalibracaoSetup();
void   menuCalibracaoLoop();
void   menuPosCalibracao();
bool   aguardarConfirmacaoEntrada(uint32_t timeoutMs);
bool   confirmarSaidaCalibracao();
void   menuSensibilidade();
void   salvarOffsetNVS();
bool   carregarOffsetNVS();
void   apagarOffsetNVS();
void   salvarHorimetroNVS();
float  carregarHorimetroNVS();
void   zerarHorimetroNVS();
void   salvarConfigNVS();
bool   carregarConfigNVS();
void   resetarConfigNVS();
void   confirmarResetConfig();
float  calcularVibracaoRMS();
float  calcularHealthScore(float temp, float vib, float horas);
String classificarAlarme(float temp, float vib);
String obterClassificacaoHealth(float score);
void   imprimirRelatorio();

// -------------------------
//  Instancias
// -------------------------
OneWire            barramento1Wire(PINO_ONE_WIRE);
DallasTemperature  sensorTemp(&barramento1Wire);
Adafruit_MPU6050   mpu;
Preferences        prefsCalibra;
Preferences        prefsSistema;
Preferences        prefsConfig;

// -------------------------
//  Variaveis de controle
// -------------------------
unsigned long ultimaLeitura       = 0;
unsigned long inicioSistema       = 0;
unsigned long ultimoSaveHorimetro = 0;
float temperatura   = 0.0;
float vibracaoRMS   = 0.0;
float horimetro     = 0.0;
float horasBase     = 0.0;
float healthScore   = 0.0;
String statusAlarme = "";

// -------------------------
//  Offsets de calibracao
// -------------------------
float offsetAX   = 0.0;
float offsetAY   = 0.0;
float offsetAZ   = 0.0;
bool  calibrado    = false;
bool  emCalibracao = false;

// -------------------------
//  Configuracao de escala
// -------------------------
int escalaAtualG = 8;

// =============================================================================
//  NVS — CALIBRACAO (sigma_cal)
// =============================================================================
void salvarOffsetNVS() {
  prefsCalibra.begin("sigma_cal", false);
  prefsCalibra.putFloat("offsetAX", offsetAX);
  prefsCalibra.putFloat("offsetAY", offsetAY);
  prefsCalibra.putFloat("offsetAZ", offsetAZ);
  prefsCalibra.putBool("calibrado", calibrado);
  prefsCalibra.end();
  Serial.println(F("  [NVS] Offsets salvos na flash (sigma_cal)."));
}

bool carregarOffsetNVS() {
  prefsCalibra.begin("sigma_cal", true);
  bool cal = prefsCalibra.getBool("calibrado", false);
  if (cal) {
    offsetAX  = prefsCalibra.getFloat("offsetAX", 0.0);
    offsetAY  = prefsCalibra.getFloat("offsetAY", 0.0);
    offsetAZ  = prefsCalibra.getFloat("offsetAZ", 0.0);
    calibrado = true;
  }
  prefsCalibra.end();
  return cal;
}

void apagarOffsetNVS() {
  prefsCalibra.begin("sigma_cal", false);
  prefsCalibra.clear();
  prefsCalibra.end();
  offsetAX = 0.0; offsetAY = 0.0; offsetAZ = 0.0;
  calibrado = false;
  Serial.println(F("  [NVS] Offsets apagados (sigma_cal). RAM zerada."));
}

// =============================================================================
//  NVS — HORIMETRO (sigma_sys)
// =============================================================================
void salvarHorimetroNVS() {
  prefsSistema.begin("sigma_sys", false);
  prefsSistema.putFloat("horimetro", horimetro);
  prefsSistema.end();
}

float carregarHorimetroNVS() {
  prefsSistema.begin("sigma_sys", true);
  float h = prefsSistema.getFloat("horimetro", 0.0);
  prefsSistema.end();
  return h;
}

void zerarHorimetroNVS() {
  prefsSistema.begin("sigma_sys", false);
  prefsSistema.putFloat("horimetro", 0.0);
  prefsSistema.end();
  horasBase     = 0.0;
  horimetro     = 0.0;
  inicioSistema = millis();
  Serial.println(F("  [NVS] Horimetro zerado na flash e em RAM."));
  Serial.println(F("  [NVS] Contagem de horas reiniciada."));
}

// =============================================================================
//  NVS — CONFIGURACAO (sigma_cfg)
// =============================================================================
void salvarConfigNVS() {
  prefsConfig.begin("sigma_cfg", false);
  prefsConfig.putInt("escalaG",    escalaAtualG);
  prefsConfig.putFloat("vibAviso",   VIB_AVISO);
  prefsConfig.putFloat("vibCritica", VIB_CRITICA);
  prefsConfig.end();
  Serial.println(F("  [NVS] Configuracao salva na flash (sigma_cfg)."));
}

bool carregarConfigNVS() {
  prefsConfig.begin("sigma_cfg", true);
  bool existe = prefsConfig.isKey("escalaG");
  if (existe) {
    escalaAtualG = prefsConfig.getInt("escalaG",      8);
    VIB_AVISO    = prefsConfig.getFloat("vibAviso",   2.0);
    VIB_CRITICA  = prefsConfig.getFloat("vibCritica", 4.0);
  }
  prefsConfig.end();
  return existe;
}

void resetarConfigNVS() {
  prefsConfig.begin("sigma_cfg", false);
  prefsConfig.clear();
  prefsConfig.end();
  escalaAtualG = 8;
  VIB_AVISO    = 2.0;
  VIB_CRITICA  = 4.0;
  // Aplica escala padrao no hardware
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  salvarConfigNVS();
  Serial.println(F("  [NVS] Configuracao resetada para padroes de fabrica."));
  Serial.print(F("  [NVS]   Escala      : +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
  Serial.print(F("  [NVS]   VIB_AVISO   : ")); Serial.print(VIB_AVISO, 2); Serial.println(F(" g"));
  Serial.print(F("  [NVS]   VIB_CRITICA : ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
}

// =============================================================================
//  FUNCAO: confirmarResetConfig
//  Confirmacao dupla antes de resetar configuracao.
//  1a confirmacao : S/N com timeout 30s
//  2a confirmacao : digitar "RESET" + Enter com timeout 30s
// =============================================================================
void confirmarResetConfig() {
  while (Serial.available()) Serial.read();

  // --- Aviso e 1a confirmacao ---
  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |          *** ATENCAO — ACAO IRREVERSIVEL ***       |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Esta acao apagara PERMANENTEMENTE:                |"));
  Serial.println(F("  |    - Escala do acelerometro                        |"));
  Serial.println(F("  |    - Limiar VIB_AVISO                              |"));
  Serial.println(F("  |    - Limiar VIB_CRITICA                            |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Os valores voltarao ao padrao de fabrica:         |"));
  Serial.println(F("  |    Escala      : +/- 8g                            |"));
  Serial.println(F("  |    VIB_AVISO   : 2.00 g                            |"));
  Serial.println(F("  |    VIB_CRITICA : 4.00 g                            |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  1a CONFIRMACAO:                                   |"));
  Serial.println(F("  |  S = Continuar para confirmacao final              |"));
  Serial.println(F("  |  N = Cancelar e voltar ao monitoramento            |"));
  Serial.println(F("  +----------------------------------------------------+"));

  // Aguarda 1a confirmacao com timeout de 30s
  uint32_t inicio        = millis();
  uint32_t segsRestantes = 30;
  uint32_t ultimoSeg     = millis();
  bool     confirmou1    = false;

  Serial.print(F("  Aguardando... tempo restante: 30 s\r"));

  while ((millis() - inicio) < 30000) {
    if ((millis() - ultimoSeg) >= 1000) {
      ultimoSeg = millis();
      segsRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segsRestantes);
      Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read();
      Serial.println(F(""));
      if (r == 'S' || r == 's') { confirmou1 = true;  break; }
      if (r == 'N' || r == 'n') { confirmou1 = false; break; }
    }
    delay(50);
  }

  if (!confirmou1) {
    Serial.println(F("  [RESET] Operacao CANCELADA na 1a confirmacao."));
    Serial.println(F("  [RESET] Configuracao mantida sem alteracoes."));
    Serial.println();
    return;
  }

  // --- 2a confirmacao — digitar RESET ---
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |          *** CONFIRMACAO FINAL ***                 |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Tem CERTEZA ABSOLUTA que deseja resetar?          |"));
  Serial.println(F("  |  Esta acao NAO pode ser desfeita!                  |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Digite  RESET  e pressione Enter para confirmar   |"));
  Serial.println(F("  |  Digite  N      para cancelar                      |"));
  Serial.println(F("  +----------------------------------------------------+"));

  inicio        = millis();
  segsRestantes = 30;
  ultimoSeg     = millis();
  String input  = "";

  Serial.print(F("  Aguardando... tempo restante: 30 s\r"));

  while ((millis() - inicio) < 30000) {
    if ((millis() - ultimoSeg) >= 1000) {
      ultimoSeg = millis();
      segsRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segsRestantes);
      Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        // Enter recebido — verifica se digitou RESET
        Serial.println(F(""));
        input.trim();
        if (input == "RESET") {
          Serial.println(F("  [RESET] 2a confirmacao recebida. Resetando configuracao..."));
          resetarConfigNVS();
          Serial.println(F("  [RESET] Configuracao resetada para padroes de fabrica!"));
          Serial.println(F("  [RESET] Monitoramento retomado com configuracao padrao."));
          Serial.println();
          return;
        } else if (input == "N" || input == "n") {
          Serial.println(F("  [RESET] Operacao CANCELADA na 2a confirmacao."));
          Serial.println(F("  [RESET] Configuracao mantida sem alteracoes."));
          Serial.println();
          return;
        } else {
          // Input invalido — cancela por seguranca
          Serial.println(F("  [RESET] Entrada invalida. Operacao CANCELADA por seguranca."));
          Serial.println(F("  [RESET] Para resetar, digite exatamente: RESET"));
          Serial.println();
          return;
        }
      } else if (c == 'N' || c == 'n') {
        // N sem Enter tambem cancela
        Serial.println(F(""));
        Serial.println(F("  [RESET] Operacao CANCELADA."));
        Serial.println();
        return;
      } else {
        input += c; // Acumula caracteres digitados
      }
    }
    delay(50);
  }

  // Timeout — cancela por seguranca
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 30s. Operacao CANCELADA por seguranca."));
  Serial.println();
}

// =============================================================================
//  FUNCAO: executarCalibracao
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
      Serial.print(F("  [CAL] ")); Serial.print(i + 1);
      Serial.print(F(" / ")); Serial.print(NUM_AMOSTRAS);
      Serial.println(F(" amostras coletadas..."));
    }

    if (Serial.available() > 0) {
      char tecla = Serial.read();
      if (tecla == 'X' || tecla == 'x') {
        Serial.println(F(""));
        Serial.print(F("  [CAL] Cancelamento solicitado apos "));
        Serial.print(coletadas); Serial.println(F(" amostras."));
        bool sair = confirmarSaidaCalibracao();
        if (sair) {
          Serial.println(F("  [CAL] Calibracao ABORTADA. Offsets anteriores mantidos."));
          if (!calibrado) Serial.println(F("  [CAL] ATENCAO: Nenhuma calibracao ativa. Offsets = 0."));
          Serial.println(F("  ==============================================="));
          emCalibracao = false;
          return false;
        }
      }
    }
  }

  offsetAX = somaX / NUM_AMOSTRAS;
  offsetAY = somaY / NUM_AMOSTRAS;
  offsetAZ = somaZ / NUM_AMOSTRAS;
  calibrado = true;

  Serial.println(F("  [CAL] Calibracao concluida com sucesso!"));
  Serial.println(F("  [CAL] ------- Offsets Calculados -------"));
  Serial.print(F("  [CAL]   Offset AX : ")); Serial.print(offsetAX, 5); Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AY : ")); Serial.print(offsetAY, 5); Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AZ : ")); Serial.print(offsetAZ, 5); Serial.println(F(" g  (bruto, inclui ~1g)"));
  Serial.println(F("  [CAL] Offsets aplicados e salvos na NVS."));
  Serial.println(F("  ==============================================="));
  Serial.println();

  salvarOffsetNVS();
  return true;
}

// =============================================================================
//  FUNCAO: menuCalibracaoSetup
// =============================================================================
void menuCalibracaoSetup() {
  emCalibracao = true;
  Serial.println(F("  [NVS] Verificando offsets salvos na flash..."));

  bool ok = carregarOffsetNVS();
  if (ok) {
    Serial.println(F("  [NVS] Offsets encontrados e carregados!"));
    Serial.print(F("  [NVS]   Offset AX : ")); Serial.print(offsetAX, 5); Serial.println(F(" g"));
    Serial.print(F("  [NVS]   Offset AY : ")); Serial.print(offsetAY, 5); Serial.println(F(" g"));
    Serial.print(F("  [NVS]   Offset AZ : ")); Serial.print(offsetAZ, 5); Serial.println(F(" g"));
    Serial.println(F("  [NVS] Calibracao automatica PULADA — offsets salvos em uso."));
    Serial.println(F("  [NVS] Envie 'C' para recalibrar ou 'N' para apagar NVS."));
    Serial.println();
  } else {
    Serial.println(F("  [NVS] Nenhum offset encontrado. Iniciando calibracao automatica..."));
    Serial.println(F("  [SIGMA] Mantenha o sensor PARADO e NIVELADO!"));
    Serial.println(F("  [SIGMA] Envie 'X' a qualquer momento para cancelar."));
    delay(2000);
    bool calOk = executarCalibracao();
    if (!calOk) {
      Serial.println(F("  [SIGMA] Calibracao cancelada. Offsets = 0."));
      Serial.println(F("  [SIGMA] Envie 'C' para calibrar manualmente."));
      Serial.println();
    }
  }
  emCalibracao = false;
}

// =============================================================================
//  FUNCAO: menuCalibracaoLoop
// =============================================================================
void menuCalibracaoLoop() {
  emCalibracao = true;
  Serial.println(F(""));
  Serial.println(F("  [CMD] Comando de recalibracao recebido!"));
  Serial.println(F("  [CMD] Monitoramento pausado."));

  bool confirmado = aguardarConfirmacaoEntrada(30000);
  if (!confirmado) {
    Serial.println(F("  [CMD] Recalibracao cancelada. Monitoramento retomado."));
    Serial.println();
    emCalibracao = false;
    return;
  }

  bool ok = executarCalibracao();
  if (!ok) {
    Serial.println(F("  [INFO] Calibracao interrompida. Offsets anteriores mantidos."));
    Serial.println();
    emCalibracao = false;
    return;
  }
  menuPosCalibracao();
}

// =============================================================================
//  FUNCAO: aguardarConfirmacaoEntrada
// =============================================================================
bool aguardarConfirmacaoEntrada(uint32_t timeoutMs) {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |          CONFIRMACAO NECESSARIA                  |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Deseja iniciar a calibracao de ponto zero?      |"));
  Serial.println(F("  |  >> O sensor deve estar PARADO e NIVELADO!       |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  S = CONFIRMAR  |  N = CANCELAR                 |"));
  Serial.println(F("  +--------------------------------------------------+"));

  uint32_t inicio        = millis();
  uint32_t segsRestantes = timeoutMs / 1000;
  uint32_t ultimoSeg     = millis();

  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segsRestantes); Serial.print(F(" s\r"));

  while ((millis() - inicio) < timeoutMs) {
    if ((millis() - ultimoSeg) >= 1000) {
      ultimoSeg = millis(); segsRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segsRestantes); Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r == 'S' || r == 's') { Serial.println(F("  [OK] Calibracao CONFIRMADA.")); return true; }
      if (r == 'N' || r == 'n') { Serial.println(F("  [INFO] Calibracao CANCELADA.")); return false; }
    }
    delay(50);
  }
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Calibracao cancelada automaticamente."));
  return false;
}

// =============================================================================
//  FUNCAO: confirmarSaidaCalibracao
// =============================================================================
bool confirmarSaidaCalibracao() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |       CONFIRMAR SAIDA DA CALIBRACAO?             |"));
  Serial.println(F("  |  Amostras coletadas serao DESCARTADAS.           |"));
  Serial.println(F("  |  Offsets anteriores serao mantidos.              |"));
  Serial.println(F("  |  S = SAIR  |  N = CONTINUAR                     |"));
  Serial.println(F("  +--------------------------------------------------+"));

  const uint32_t TIMEOUT_MS  = 60000;
  uint32_t inicio        = millis();
  uint32_t segsRestantes = TIMEOUT_MS / 1000;
  uint32_t ultimoSeg     = millis();

  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segsRestantes); Serial.print(F(" s\r"));

  while ((millis() - inicio) < TIMEOUT_MS) {
    if ((millis() - ultimoSeg) >= 1000) {
      ultimoSeg = millis(); segsRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segsRestantes); Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r == 'S' || r == 's') { Serial.println(F("  [CAL] Saida CONFIRMADA.")); return true; }
      if (r == 'N' || r == 'n') { Serial.println(F("  [CAL] Saida CANCELADA. Retomando...")); return false; }
    }
    delay(50);
  }
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Coleta retomada automaticamente."));
  return false;
}

// =============================================================================
//  FUNCAO: menuPosCalibracao
// =============================================================================
void menuPosCalibracao() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |        CALIBRACAO CONCLUIDA COM SUCESSO!           |"));
  Serial.println(F("  |  S = SAIR ao monitoramento                         |"));
  Serial.println(F("  |  R = RECALIBRAR novamente                          |"));
  Serial.println(F("  |  Dica: recalibre sempre com sensor em repouso!     |"));
  Serial.println(F("  +----------------------------------------------------+"));

  const uint32_t TIMEOUT_MS  = 60000;
  uint32_t inicio        = millis();
  uint32_t segsRestantes = TIMEOUT_MS / 1000;
  uint32_t ultimoSeg     = millis();

  Serial.print(F("  Aguardando... tempo restante: "));
  Serial.print(segsRestantes); Serial.print(F(" s\r"));

  while ((millis() - inicio) < TIMEOUT_MS) {
    if ((millis() - ultimoSeg) >= 1000) {
      ultimoSeg = millis(); segsRestantes--;
      Serial.print(F("  Aguardando... tempo restante: "));
      Serial.print(segsRestantes); Serial.print(F(" s   \r"));
    }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r == 'S' || r == 's') {
        Serial.println(F("  [MENU] Monitoramento retomado com novos offsets."));
        Serial.println(); emCalibracao = false; return;
      }
      if (r == 'R' || r == 'r') {
        Serial.println(F("  [MENU] Iniciando nova calibracao..."));
        delay(1000);
        bool ok = executarCalibracao();
        if (ok) { menuPosCalibracao(); }
        else { Serial.println(F("  [INFO] Calibracao interrompida.")); Serial.println(); emCalibracao = false; }
        return;
      }
    }
    delay(50);
  }
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Monitoramento retomado automaticamente."));
  Serial.println(); emCalibracao = false;
}

// =============================================================================
//  FUNCAO: menuSensibilidade
// =============================================================================
void menuSensibilidade() {
  emCalibracao = true;
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +====================================================+"));
  Serial.println(F("  |       MENU DE SENSIBILIDADE DO ACELEROMETRO        |"));
  Serial.println(F("  +====================================================+"));
  Serial.print(F(  "  |  Escala atual     : +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g                         |"));
  Serial.print(F(  "  |  VIB_AVISO atual  : ")); Serial.print(VIB_AVISO, 2);   Serial.println(F(" g                             |"));
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
  uint32_t inicio        = millis();
  uint32_t segsRestantes = TIMEOUT_MS / 1000;
  uint32_t ultimoSeg     = millis();
  bool     menuAtivo     = true;

  Serial.print(F("  Aguardando selecao... tempo restante: "));
  Serial.print(segsRestantes); Serial.print(F(" s\r"));

  while (menuAtivo && (millis() - inicio) < TIMEOUT_MS) {
    if ((millis() - ultimoSeg) >= 1000) {
      ultimoSeg = millis(); segsRestantes--;
      Serial.print(F("  Aguardando selecao... tempo restante: "));
      Serial.print(segsRestantes); Serial.print(F(" s   \r"));
    }

    if (Serial.available() > 0) {
      char opcao = Serial.read();
      Serial.println(F(""));

      if (opcao == 'S' || opcao == 's') {
        Serial.println(F("  [SENS] Configuracao mantida. Saindo..."));
        menuAtivo = false; break;
      }

      mpu6050_accel_range_t novaEscalaEnum;
      int   novaEscalaG     = 0;
      float sugestaoAviso   = 0.0;
      float sugestaoCritica = 0.0;
      bool  escalaValida    = false;

      if      (opcao == '1') { novaEscalaG = 2;  novaEscalaEnum = MPU6050_RANGE_2_G;  sugestaoAviso = 0.5; sugestaoCritica = 1.0; escalaValida = true; }
      else if (opcao == '2') { novaEscalaG = 4;  novaEscalaEnum = MPU6050_RANGE_4_G;  sugestaoAviso = 1.0; sugestaoCritica = 2.0; escalaValida = true; }
      else if (opcao == '3') { novaEscalaG = 8;  novaEscalaEnum = MPU6050_RANGE_8_G;  sugestaoAviso = 2.0; sugestaoCritica = 4.0; escalaValida = true; }
      else if (opcao == '4') { novaEscalaG = 16; novaEscalaEnum = MPU6050_RANGE_16_G; sugestaoAviso = 4.0; sugestaoCritica = 8.0; escalaValida = true; }

      if (escalaValida) {
        Serial.print(F("  [SENS] Nova escala: +/- ")); Serial.print(novaEscalaG); Serial.println(F(" g"));
        Serial.print(F("  [SENS] Sugestao — Aviso: ")); Serial.print(sugestaoAviso, 2);
        Serial.print(F(" g  |  Critico: ")); Serial.print(sugestaoCritica, 2); Serial.println(F(" g"));
        Serial.println(F("  S = Aplicar sugestao  |  M = Manual  |  N = Manter atuais"));

        while (Serial.available()) Serial.read();
        uint32_t inicioL = millis(); bool aguardando = true;

        while (aguardando && (millis() - inicioL) < 30000) {
          if (Serial.available() > 0) {
            char res = Serial.read(); Serial.println(F(""));
            if (res == 'S' || res == 's') {
              VIB_AVISO = sugestaoAviso; VIB_CRITICA = sugestaoCritica;
              Serial.println(F("  [SENS] Limiares sugeridos aplicados.")); aguardando = false;
            } else if (res == 'M' || res == 'm') {
              Serial.println(F("  [SENS] Digite o novo VIB_AVISO e pressione Enter:"));
              while (Serial.available()) Serial.read();
              String sA = ""; uint32_t t = millis();
              while ((millis()-t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c=='\n'||c=='\r') break; sA+=c; } }
              Serial.println(F("  [SENS] Digite o novo VIB_CRITICA e pressione Enter:"));
              while (Serial.available()) Serial.read();
              String sC = ""; t = millis();
              while ((millis()-t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c=='\n'||c=='\r') break; sC+=c; } }
              float nA = sA.toFloat(), nC = sC.toFloat();
              if (nA > 0.0 && nC > 0.0 && nA < nC) { VIB_AVISO = nA; VIB_CRITICA = nC; Serial.println(F("  [SENS] Limiares manuais aplicados.")); }
              else { Serial.println(F("  [SENS] ERRO: valores invalidos.")); }
              aguardando = false;
            } else if (res == 'N' || res == 'n') {
              Serial.println(F("  [SENS] Limiares mantidos.")); aguardando = false;
            }
          }
          delay(50);
        }
        if (aguardando) { VIB_AVISO = sugestaoAviso; VIB_CRITICA = sugestaoCritica; Serial.println(F("  [TIMEOUT] Sugestao aplicada automaticamente.")); }

        mpu.setAccelerometerRange(novaEscalaEnum);
        escalaAtualG = novaEscalaG;
        salvarConfigNVS(); // Salva nova configuracao na NVS
        Serial.print(F("  [SENS] Escala aplicada: +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
        Serial.println(F("  [SENS] Configuracao salva na NVS."));
        Serial.println(F("  [SENS] Mudanca de escala invalida offsets. Recalibrando..."));
        Serial.println(F("  [SENS] Mantenha o sensor PARADO e NIVELADO!"));
        delay(2000);
        calibrado = false; offsetAX = 0.0; offsetAY = 0.0; offsetAZ = 0.0;
        executarCalibracao();
        menuAtivo = false;

      } else if (opcao == 'M' || opcao == 'm') {
        Serial.println(F("  [SENS] Ajuste manual (escala mantida)."));
        Serial.print(F("  [SENS] VIB_AVISO atual: ")); Serial.print(VIB_AVISO, 2); Serial.println(F(" g"));
        Serial.println(F("  [SENS] Digite o novo VIB_AVISO e pressione Enter:"));
        while (Serial.available()) Serial.read();
        String sA = ""; uint32_t t = millis();
        while ((millis()-t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c=='\n'||c=='\r') break; sA+=c; } }
        Serial.print(F("  [SENS] VIB_CRITICA atual: ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
        Serial.println(F("  [SENS] Digite o novo VIB_CRITICA e pressione Enter:"));
        while (Serial.available()) Serial.read();
        String sC = ""; t = millis();
        while ((millis()-t) < 30000) { if (Serial.available()) { char c = Serial.read(); if (c=='\n'||c=='\r') break; sC+=c; } }
        float nA = sA.toFloat(), nC = sC.toFloat();
        if (nA > 0.0 && nC > 0.0 && nA < nC) { VIB_AVISO = nA; VIB_CRITICA = nC; salvarConfigNVS(); Serial.println(F("  [SENS] Limiares aplicados e salvos na NVS.")); }
        else { Serial.println(F("  [SENS] ERRO: valores invalidos.")); }
        menuAtivo = false;
      } else {
        Serial.println(F("  [SENS] Opcao invalida. Digite 1, 2, 3, 4, M ou S."));
      }
    }
    delay(50);
  }

  if (menuAtivo) { Serial.println(F("")); Serial.println(F("  [TIMEOUT] Menu encerrado sem alteracoes.")); }

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
// =============================================================================
float calcularVibracaoRMS() {
  const int NUM_AMOSTRAS = 50;
  float somaQ = 0.0;
  sensors_event_t accel, gyro, temp;
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.getEvent(&accel, &gyro, &temp);
    float ax = (accel.acceleration.x / 9.81) - offsetAX;
    float ay = (accel.acceleration.y / 9.81) - offsetAY;
    float az = (accel.acceleration.z / 9.81) - offsetAZ;
    somaQ += (ax*ax) + (ay*ay) + (az*az);
    delay(2);
  }
  return sqrt(somaQ / NUM_AMOSTRAS);
}

// =============================================================================
//  FUNCAO: calcularHealthScore
// =============================================================================
float calcularHealthScore(float temp, float vib, float horas) {
  float sT, sV, sH;
  if      (temp <= TEMP_AVISO_MAX) sT = 1.0;
  else if (temp >= TEMP_CRITICA)   sT = 0.0;
  else sT = 1.0 - ((temp - TEMP_AVISO_MAX) / (TEMP_CRITICA - TEMP_AVISO_MAX));

  if      (vib <= VIB_AVISO)   sV = 1.0;
  else if (vib >= VIB_CRITICA) sV = 0.0;
  else sV = 1.0 - ((vib - VIB_AVISO) / (VIB_CRITICA - VIB_AVISO));

  float limDeg = INTERVALO_MANUTENCAO_H * 0.80;
  if      (horas <= limDeg)                 sH = 1.0;
  else if (horas >= INTERVALO_MANUTENCAO_H) sH = 0.0;
  else sH = 1.0 - ((horas - limDeg) / (INTERVALO_MANUTENCAO_H - limDeg));

  return (sT * 40.0) + (sV * 40.0) + (sH * 20.0);
}

// =============================================================================
//  FUNCAO: classificarAlarme
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
// =============================================================================
void imprimirRelatorio() {
  float horasRestantes = INTERVALO_MANUTENCAO_H - horimetro;
  if (horasRestantes < 0) horasRestantes = 0;

  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Timestamp       : ")); Serial.print(millis()); Serial.println(F(" ms"));
  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Temperatura     : ")); Serial.print(temperatura, 1); Serial.println(F(" oC"));
  Serial.print(F("  Vibracao        : ")); Serial.print(vibracaoRMS, 3); Serial.println(F(" g"));
  Serial.print(F("  Horimetro       : ")); Serial.print(horimetro, 4);
  Serial.print(F(" h  (base NVS: ")); Serial.print(horasBase, 1); Serial.println(F(" h)"));
  Serial.print(F("  Prox. Manut.    : ")); Serial.print(horasRestantes, 1); Serial.println(F(" h restantes"));
  Serial.print(F("  Health Score    : ")); Serial.print(healthScore, 1);
  Serial.print(F(" %  (")); Serial.print(obterClassificacaoHealth(healthScore)); Serial.println(F(")"));
  Serial.print(F("  Escala Acel.    : +/- ")); Serial.print(escalaAtualG);
  Serial.print(F(" g  |  Aviso: ")); Serial.print(VIB_AVISO, 2);
  Serial.print(F(" g  |  Critico: ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
  Serial.print(F("  Offsets NVS     : "));
  Serial.println(calibrado ? F("Salvos (calibracao ativa)") : F("Zerados (sem calibracao)"));
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
  Serial.println(F("  Project SIGMA v0.1.3.3 - Iniciando..."));
  Serial.println(F("=============================================="));

  // Carrega horimetro da NVS
  horasBase = carregarHorimetroNVS();
  if (horasBase > 0.0) {
    Serial.print(F("  [NVS] Horimetro carregado: ")); Serial.print(horasBase, 4); Serial.println(F(" h"));
  } else {
    Serial.println(F("  [NVS] Horimetro: nenhum valor salvo. Iniciando do zero."));
  }

  // Carrega configuracao da NVS
  bool cfgCarregada = carregarConfigNVS();
  if (cfgCarregada) {
    Serial.println(F("  [NVS] Configuracao carregada da flash (sigma_cfg):"));
    Serial.print(F("  [NVS]   Escala      : +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
    Serial.print(F("  [NVS]   VIB_AVISO   : ")); Serial.print(VIB_AVISO, 2); Serial.println(F(" g"));
    Serial.print(F("  [NVS]   VIB_CRITICA : ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
  } else {
    Serial.println(F("  [NVS] Nenhuma configuracao salva. Usando padroes de fabrica."));
  }

  // I2C
  Wire.begin(PINO_MPU_SDA, PINO_MPU_SCL);
  Wire.setClock(400000);
  delay(150);
  Serial.println(F("  [I2C] Barramento iniciado (400 kHz Fast Mode)."));

  // MPU6050 — ate 5 tentativas
  bool mpuOk = false;
  for (int t = 1; t <= 5; t++) {
    Serial.print(F("  [I2C] Tentativa ")); Serial.print(t); Serial.print(F(" de 5 - MPU6050 (0x68)..."));
    if (mpu.begin()) { mpuOk = true; Serial.println(F(" ENCONTRADO!")); break; }
    Serial.println(F(" nao respondeu.")); delay(200);
  }

  if (!mpuOk) {
    Serial.println(F("  [ERRO FATAL] MPU6050 nao encontrado!"));
    Serial.println(F("    1. Pull-up 4.7k no SDA (GPIO8) ao 3.3V?"));
    Serial.println(F("    2. Pull-up 4.7k no SCL (GPIO9) ao 3.3V?"));
    Serial.println(F("    3. VCC MPU6050 = 3.3V?"));
    Serial.println(F("    4. GND MPU6050 = GND?"));
    Serial.println(F("    5. AD0 no GND = 0x68 | AD0 no 3.3V = 0x69"));
    Serial.println(F("  Sistema pausado. Corrija e reinicie."));
    while (true) delay(1000);
  }

  Serial.println(F("  [OK]  MPU6050 inicializado."));

  // Aplica escala carregada da NVS (ou padrao 8g)
  mpu6050_accel_range_t escalaEnum = MPU6050_RANGE_8_G;
  if      (escalaAtualG == 2)  escalaEnum = MPU6050_RANGE_2_G;
  else if (escalaAtualG == 4)  escalaEnum = MPU6050_RANGE_4_G;
  else if (escalaAtualG == 16) escalaEnum = MPU6050_RANGE_16_G;
  mpu.setAccelerometerRange(escalaEnum);
  Serial.print(F("  [MPU] Escala acelerometro: +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.println(F("  [MPU] Giroscopio: +/- 500 deg/s"));
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println(F("  [MPU] Filtro passa-baixa: 21 Hz"));

  // Calibracao — verifica NVS
  menuCalibracaoSetup();

  // DS18B20
  sensorTemp.begin();
  int qtd = sensorTemp.getDeviceCount();
  Serial.print(F("  [DS18B20] Sensores encontrados: ")); Serial.println(qtd);
  if (qtd == 0) { Serial.println(F("  [AVISO] Nenhum DS18B20 detectado!")); }
  else { sensorTemp.setResolution(12); Serial.println(F("  [DS18B20] Resolucao: 12 bits")); }

  inicioSistema       = millis();
  ultimaLeitura       = millis();
  ultimoSaveHorimetro = millis();

  Serial.println(F("=============================================="));
  Serial.println(F("  Sistema pronto. Monitoramento iniciado."));
  Serial.println(F("  Comandos: C=Recalibrar  | A=Sensibilidade"));
  Serial.println(F("            N=Apagar NVS  | H=Zerar Horimetro"));
  Serial.println(F("            R=Reset Config"));
  Serial.println(F("=============================================="));
  Serial.println();
}

// =============================================================================
//  LOOP
// =============================================================================
void loop() {
  unsigned long agora = millis();

  // Comandos Serial
  if (!emCalibracao && Serial.available() > 0) {
    char cmd = Serial.read();
    if      (cmd == 'C' || cmd == 'c') { menuCalibracaoLoop(); }
    else if (cmd == 'A' || cmd == 'a') { Serial.println(F("  [CMD] Menu de sensibilidade ativado!")); menuSensibilidade(); }
    else if (cmd == 'N' || cmd == 'n') { Serial.println(F("  [CMD] Apagando NVS de calibracao...")); apagarOffsetNVS(); menuCalibracaoLoop(); }
    else if (cmd == 'H' || cmd == 'h') { zerarHorimetroNVS(); }
    else if (cmd == 'R' || cmd == 'r') { confirmarResetConfig(); }
  }

  // Salvamento automatico do horimetro a cada 5 minutos
  if (!emCalibracao && (agora - ultimoSaveHorimetro) >= INTERVALO_SAVE_HORIMETRO_MS) {
    ultimoSaveHorimetro = agora;
    salvarHorimetroNVS();
  }

  // Leitura dos sensores
  if (!emCalibracao && (agora - ultimaLeitura) >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora;
    horimetro = horasBase + (float)(agora - inicioSistema) / 3600000.0;

    sensorTemp.requestTemperatures();
    float lT = sensorTemp.getTempCByIndex(0);
    if (lT != DEVICE_DISCONNECTED_C) temperatura = lT;
    else Serial.println(F("  [AVISO] Falha na leitura do DS18B20!"));

    vibracaoRMS  = calcularVibracaoRMS();
    healthScore  = calcularHealthScore(temperatura, vibracaoRMS, horimetro);
    statusAlarme = classificarAlarme(temperatura, vibracaoRMS);
    imprimirRelatorio();
  }
}
