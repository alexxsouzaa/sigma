// =============================================================================
//  Project SIGMA - Sistema de Monitoramento Preditivo de Motor Industrial
//  Codinome   : Project SIGMA
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Sensores   : DS18B20 (temperatura) + MPU6050 (vibracao)
//  Protocolo  : Serial 115200 baud
//  Versao     : 0.1.5.0
// =============================================================================
//
//  SERIE v0.1.5.x — Baseline de Vibracao
//  [v0.1.5.0]   - 2026-06-26                                [CURRENT]
//  Autor  : Bruno Alex Souza da Silva
//  Status : Em desenvolvimento
//  Resumo :
//    - Coleta de baseline de vibracao em operacao normal (cmd 'B')
//    - 300 amostras com motor em funcionamento normal
//    - Calcula media e desvio padrao da vibracao
//    - Salvo na NVS (sigma_bas): basMedia, basStddev, fatorK, basAtivo, basN
//    - Alarme relativo: Aviso = media + K*stddev | Critico = media + 2K*stddev
//    - Health Score usa desvio em sigmas quando baseline ativo
//    - Fator K ajustavel via cmd 'K' (1.0 a 5.0, padrao 2.0)
//    - Apagar baseline via cmd 'Z'
//    - Relatorio exibe baseline + desvio atual + limiar calculado
//    - Quando baseline inativo: usa VIB_AVISO/VIB_CRITICA absolutos
//    - Novas funcoes: coletarBaseline(), ajustarFatorK()
//    - salvarBaselineNVS() / carregarBaselineNVS() / apagarBaselineNVS()
//    - Namespace NVS: sigma_bas
//
// =============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <math.h>

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
#define WDT_TIMEOUT_MS              10000
#define MAX_FALHAS_DS18B20          5
#define NUM_AMOSTRAS_BASELINE       300     // Amostras para coleta do baseline
#define FATOR_K_PADRAO              2.0     // Multiplicador padrao (2 sigma)
#define FATOR_K_MIN                 1.0     // Fator K minimo configuravel
#define FATOR_K_MAX                 5.0     // Fator K maximo configuravel

float VIB_AVISO   = 2.0;
float VIB_CRITICA = 4.0;

// -------------------------
//  Forward declarations
// -------------------------
void   reiniciarSistema(String motivo);
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
void   salvarResetNVS(String motivo);
void   carregarResetNVS();
void   salvarBaselineNVS();
bool   carregarBaselineNVS();
void   apagarBaselineNVS();
void   coletarBaseline();
void   ajustarFatorK();
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
Preferences        prefsBaseline;

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

// -------------------------
//  Watchdog e reset
// -------------------------
int    resetCount    = 0;
String resetMotivo   = "Nenhum";
int    falhasDS18B20 = 0;

// -------------------------
//  Baseline de vibracao
// -------------------------
float basMedia  = 0.0;    // Media da vibracao em operacao normal (g)
float basStddev = 0.0;    // Desvio padrao da vibracao (g)
float fatorK    = FATOR_K_PADRAO; // Multiplicador de sensibilidade
bool  basAtivo  = false;  // Flag: baseline valido e carregado
int   basN      = 0;      // Numero de amostras coletadas

// =============================================================================
//  FUNCAO: reiniciarSistema
// =============================================================================
void reiniciarSistema(String motivo) {
  Serial.println(F(""));
  Serial.println(F("  ================================================"));
  Serial.println(F("  [RESET] REINICIANDO SISTEMA..."));
  Serial.print(F("  [RESET] Motivo: ")); Serial.println(motivo);
  Serial.println(F("  ================================================"));
  salvarHorimetroNVS();
  salvarResetNVS(motivo);
  Serial.println(F("  [RESET] NVS salva. Reiniciando em 2 segundos..."));
  Serial.flush(); delay(2000);
  esp_restart();
}

// =============================================================================
//  NVS — RESET (sigma_sys)
// =============================================================================
void salvarResetNVS(String motivo) {
  prefsSistema.begin("sigma_sys", false);
  int count = prefsSistema.getInt("resetCount", 0) + 1;
  prefsSistema.putInt("resetCount", count);
  prefsSistema.putString("resetMotivo", motivo);
  prefsSistema.end();
}

void carregarResetNVS() {
  prefsSistema.begin("sigma_sys", true);
  resetCount  = prefsSistema.getInt("resetCount", 0);
  resetMotivo = prefsSistema.getString("resetMotivo", "Nenhum");
  prefsSistema.end();
}

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
  Serial.println(F("  [NVS] Offsets salvos (sigma_cal)."));
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
  Serial.println(F("  [NVS] Offsets apagados (sigma_cal)."));
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
  prefsSistema.putInt("resetCount", 0);
  prefsSistema.putString("resetMotivo", "Nenhum");
  prefsSistema.end();
  horasBase = 0.0; horimetro = 0.0;
  resetCount = 0; resetMotivo = "Nenhum";
  inicioSistema = millis();
  Serial.println(F("  [NVS] Horimetro e resets zerados."));
}

// =============================================================================
//  NVS — CONFIGURACAO (sigma_cfg)
// =============================================================================
void salvarConfigNVS() {
  prefsConfig.begin("sigma_cfg", false);
  prefsConfig.putInt("escalaG",      escalaAtualG);
  prefsConfig.putFloat("vibAviso",   VIB_AVISO);
  prefsConfig.putFloat("vibCritica", VIB_CRITICA);
  prefsConfig.end();
  Serial.println(F("  [NVS] Configuracao salva (sigma_cfg)."));
}

bool carregarConfigNVS() {
  prefsConfig.begin("sigma_cfg", true);
  bool existe = prefsConfig.isKey("escalaG");
  if (existe) {
    escalaAtualG = prefsConfig.getInt("escalaG",       8);
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
  escalaAtualG = 8; VIB_AVISO = 2.0; VIB_CRITICA = 4.0;
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  salvarConfigNVS();
  Serial.println(F("  [NVS] Config resetada para padroes de fabrica."));
  Serial.print(F("  [NVS]   Escala: +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
  Serial.print(F("  [NVS]   Aviso : ")); Serial.print(VIB_AVISO,   2); Serial.println(F(" g"));
  Serial.print(F("  [NVS]   Crit. : ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g"));
}

// =============================================================================
//  NVS — BASELINE (sigma_bas)
// =============================================================================
void salvarBaselineNVS() {
  prefsBaseline.begin("sigma_bas", false);
  prefsBaseline.putFloat("basMedia",  basMedia);
  prefsBaseline.putFloat("basStddev", basStddev);
  prefsBaseline.putFloat("fatorK",    fatorK);
  prefsBaseline.putBool("basAtivo",   basAtivo);
  prefsBaseline.putInt("basN",        basN);
  prefsBaseline.end();
  Serial.println(F("  [NVS] Baseline salvo (sigma_bas)."));
}

bool carregarBaselineNVS() {
  prefsBaseline.begin("sigma_bas", true);
  bool ativo = prefsBaseline.getBool("basAtivo", false);
  if (ativo) {
    basMedia  = prefsBaseline.getFloat("basMedia",  0.0);
    basStddev = prefsBaseline.getFloat("basStddev", 0.0);
    fatorK    = prefsBaseline.getFloat("fatorK",    FATOR_K_PADRAO);
    basN      = prefsBaseline.getInt("basN",        0);
    basAtivo  = true;
  }
  prefsBaseline.end();
  return ativo;
}

void apagarBaselineNVS() {
  prefsBaseline.begin("sigma_bas", false);
  prefsBaseline.clear();
  prefsBaseline.end();
  basMedia = 0.0; basStddev = 0.0;
  fatorK   = FATOR_K_PADRAO;
  basAtivo = false; basN = 0;
  Serial.println(F("  [BAS] Baseline apagado da NVS e RAM zerada."));
  Serial.println(F("  [BAS] Sistema voltara a usar limiares absolutos."));
}

// =============================================================================
//  FUNCAO: coletarBaseline
//  Coleta NUM_AMOSTRAS_BASELINE leituras com motor em operacao normal.
//  Calcula media e desvio padrao. Salva na NVS (sigma_bas).
//  Chamada pelo comando 'B' no Serial Monitor.
// =============================================================================
void coletarBaseline() {
  emCalibracao = true;
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +====================================================+"));
  Serial.println(F("  |         COLETA DE BASELINE DE VIBRACAO             |"));
  Serial.println(F("  +====================================================+"));
  Serial.println(F("  |  IMPORTANTE:                                       |"));
  Serial.println(F("  |  O motor deve estar em OPERACAO NORMAL!            |"));
  Serial.println(F("  |  Nao em repouso — o baseline aprende o             |"));
  Serial.println(F("  |  padrao normal de vibração em funcionamento.       |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  S = INICIAR coleta de baseline                    |"));
  Serial.println(F("  |  N = CANCELAR                                      |"));
  Serial.println(F("  +====================================================+"));

  // Confirmacao de entrada
  uint32_t ini = millis(); uint32_t segs = 30; uint32_t ultSeg = millis();
  bool confirmado = false;
  Serial.print(F("  Aguardando... tempo restante: 30 s\r"));

  while ((millis() - ini) < 30000) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg = millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r == 'S' || r == 's') { confirmado = true; break; }
      if (r == 'N' || r == 'n') { break; }
    }
    delay(50);
  }

  if (!confirmado) {
    Serial.println(F("  [BAS] Coleta CANCELADA. Baseline nao alterado."));
    Serial.println(); emCalibracao = false; return;
  }

  // Coleta das amostras
  Serial.println(F("  [BAS] Iniciando coleta..."));
  Serial.println(F("  [BAS] Mantenha o motor em operacao normal!"));
  Serial.println(F("  [BAS] Envie 'X' para cancelar."));
  delay(2000); // Pausa para estabilizacao

  float soma   = 0.0;
  float somaQ  = 0.0; // Para calculo do desvio padrao (soma dos quadrados)
  int   colet  = 0;
  bool  abortado = false;

  for (int i = 0; i < NUM_AMOSTRAS_BASELINE; i++) {
    esp_task_wdt_reset();

    // Le vibracao instantanea (media de 10 amostras rapidas)
    float somaRapida = 0.0;
    sensors_event_t accel, gyro, temp;
    for (int j = 0; j < 10; j++) {
      mpu.getEvent(&accel, &gyro, &temp);
      float ax = (accel.acceleration.x / 9.81) - offsetAX;
      float ay = (accel.acceleration.y / 9.81) - offsetAY;
      float az = (accel.acceleration.z / 9.81) - offsetAZ;
      somaRapida += sqrt((ax*ax) + (ay*ay) + (az*az));
      delay(2);
    }
    float leitura = somaRapida / 10.0;

    soma  += leitura;
    somaQ += leitura * leitura;
    colet++;

    if ((i + 1) % 50 == 0) {
      Serial.print(F("  [BAS] ")); Serial.print(i + 1);
      Serial.print(F(" / ")); Serial.print(NUM_AMOSTRAS_BASELINE);
      Serial.print(F(" amostras  |  Media parcial: "));
      Serial.print(soma / colet, 4); Serial.println(F(" g"));
    }

    // Verifica cancelamento
    if (Serial.available() > 0) {
      char t = Serial.read();
      if (t == 'X' || t == 'x') {
        Serial.println(F("  [BAS] Coleta CANCELADA pelo usuario."));
        Serial.println(F("  [BAS] Baseline nao alterado."));
        abortado = true; break;
      }
    }
    delay(10);
  }

  if (abortado) { emCalibracao = false; return; }

  // Calcula media e desvio padrao
  // Desvio padrao: sqrt( (somaQ/N) - (soma/N)^2 )
  basMedia  = soma / colet;
  basStddev = sqrt((somaQ / colet) - (basMedia * basMedia));
  if (basStddev < 0.001) basStddev = 0.001; // Evita divisao por zero
  basAtivo  = true;
  basN      = colet;

  // Exibe resultado
  Serial.println(F("  [BAS] ======= Baseline Calculado ======="));
  Serial.print(F("  [BAS]   Amostras   : ")); Serial.println(basN);
  Serial.print(F("  [BAS]   Media      : ")); Serial.print(basMedia,  5); Serial.println(F(" g"));
  Serial.print(F("  [BAS]   Std Dev    : ")); Serial.print(basStddev, 5); Serial.println(F(" g"));
  Serial.print(F("  [BAS]   Fator K    : ")); Serial.println(fatorK, 1);
  Serial.print(F("  [BAS]   Lim.Aviso  : ")); Serial.print(basMedia + fatorK * basStddev, 5); Serial.println(F(" g"));
  Serial.print(F("  [BAS]   Lim.Crit.  : ")); Serial.print(basMedia + 2.0 * fatorK * basStddev, 5); Serial.println(F(" g"));
  Serial.println(F("  [BAS] ====================================="));
  Serial.println(F("  [BAS] Baseline salvo na NVS."));
  Serial.println();

  salvarBaselineNVS();
  emCalibracao = false;
}

// =============================================================================
//  FUNCAO: ajustarFatorK
//  Permite ajustar o fator K de sensibilidade do alarme relativo.
//  Fator K: numero de desvios padrao acima da media para gerar alarme.
//  K menor = mais sensivel | K maior = menos sensivel
//  Range valido: FATOR_K_MIN (1.0) a FATOR_K_MAX (5.0)
// =============================================================================
void ajustarFatorK() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |         AJUSTE DO FATOR K (SENSIBILIDADE)        |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  O Fator K define quantos desvios padrao         |"));
  Serial.println(F("  |  acima da media disparam o alarme.               |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  K menor = mais sensivel (mais alarmes)          |"));
  Serial.println(F("  |  K maior = menos sensivel (menos alarmes)        |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Range valido: 1.0 a 5.0                        |"));
  Serial.println(F("  |  Padrao: 2.0 (2 sigma)                          |"));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.print(F("  Fator K atual: ")); Serial.println(fatorK, 1);

  if (basAtivo) {
    Serial.print(F("  Limiar Aviso  atual: ")); Serial.print(basMedia + fatorK * basStddev, 4); Serial.println(F(" g"));
    Serial.print(F("  Limiar Critico atual: ")); Serial.print(basMedia + 2.0 * fatorK * basStddev, 4); Serial.println(F(" g"));
  }

  Serial.println(F("  Digite o novo Fator K e pressione Enter:"));
  while (Serial.available()) Serial.read();

  String input = ""; uint32_t t = millis();
  while ((millis() - t) < 30000) {
    esp_task_wdt_reset();
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') break;
      input += c;
    }
  }

  float novoK = input.toFloat();
  if (novoK >= FATOR_K_MIN && novoK <= FATOR_K_MAX) {
    fatorK = novoK;
    if (basAtivo) salvarBaselineNVS(); // Salva novo K na NVS junto com o baseline
    Serial.print(F("  [BAS] Fator K atualizado para: ")); Serial.println(fatorK, 1);
    if (basAtivo) {
      Serial.print(F("  [BAS] Novo Lim.Aviso  : ")); Serial.print(basMedia + fatorK * basStddev, 4); Serial.println(F(" g"));
      Serial.print(F("  [BAS] Novo Lim.Crit.  : ")); Serial.print(basMedia + 2.0 * fatorK * basStddev, 4); Serial.println(F(" g"));
    }
  } else {
    Serial.print(F("  [BAS] ERRO: valor invalido. Range: "));
    Serial.print(FATOR_K_MIN, 1); Serial.print(F(" a ")); Serial.print(FATOR_K_MAX, 1);
    Serial.println(F(". Fator K nao alterado."));
  }
  Serial.println();
}

// =============================================================================
//  FUNCAO: confirmarResetConfig — confirmacao dupla
// =============================================================================
void confirmarResetConfig() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |          *** ATENCAO — ACAO IRREVERSIVEL ***       |"));
  Serial.println(F("  |  Esta acao apagara PERMANENTEMENTE:                |"));
  Serial.println(F("  |    - Escala do acelerometro                        |"));
  Serial.println(F("  |    - Limiar VIB_AVISO                              |"));
  Serial.println(F("  |    - Limiar VIB_CRITICA                            |"));
  Serial.println(F("  |  Padroes: Escala=8g | Aviso=2.00g | Crit=4.00g    |"));
  Serial.println(F("  |  1a CONFIRMACAO: S = Continuar  |  N = Cancelar   |"));
  Serial.println(F("  +----------------------------------------------------+"));

  uint32_t inicio = millis(); uint32_t segs = 30; uint32_t ultSeg = millis();
  bool confirmou1 = false;
  Serial.print(F("  Aguardando... tempo restante: 30 s\r"));

  while ((millis() - inicio) < 30000) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg = millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r == 'S' || r == 's') { confirmou1 = true; break; }
      if (r == 'N' || r == 'n') { break; }
    }
    delay(50);
  }

  if (!confirmou1) { Serial.println(F("  [RESET] Cancelado. Config mantida.")); Serial.println(); return; }

  while (Serial.available()) Serial.read();
  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |          *** CONFIRMACAO FINAL ***                 |"));
  Serial.println(F("  |  Digite  RESET  + Enter para confirmar             |"));
  Serial.println(F("  |  Digite  N      para cancelar                      |"));
  Serial.println(F("  +----------------------------------------------------+"));

  inicio = millis(); segs = 30; ultSeg = millis(); String input = "";
  Serial.print(F("  Aguardando... tempo restante: 30 s\r"));

  while ((millis() - inicio) < 30000) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg = millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        Serial.println(F("")); input.trim();
        if (input == "RESET") { Serial.println(F("  [RESET] Confirmado!")); resetarConfigNVS(); Serial.println(); return; }
        else { Serial.println(F("  [RESET] Entrada invalida. Cancelado.")); Serial.println(); return; }
      } else if (c == 'N' || c == 'n') { Serial.println(F("")); Serial.println(F("  [RESET] Cancelado.")); Serial.println(); return; }
      else { input += c; }
    }
    delay(50);
  }
  Serial.println(F("")); Serial.println(F("  [TIMEOUT] Cancelado por seguranca.")); Serial.println();
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
  Serial.println(F("  [CAL] Envie 'X' para cancelar."));
  Serial.println(F("  [CAL] Coletando amostras..."));

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    esp_task_wdt_reset();
    mpu.getEvent(&accel, &gyro, &temp);
    somaX += accel.acceleration.x / 9.81;
    somaY += accel.acceleration.y / 9.81;
    somaZ += accel.acceleration.z / 9.81;
    coletadas++; delay(5);
    if ((i+1) % 50 == 0) { Serial.print(F("  [CAL] ")); Serial.print(i+1); Serial.print(F(" / ")); Serial.print(NUM_AMOSTRAS); Serial.println(F(" amostras...")); }
    if (Serial.available() > 0) {
      char t = Serial.read();
      if (t == 'X' || t == 'x') {
        Serial.println(F("")); Serial.print(F("  [CAL] Cancelamento apos ")); Serial.print(coletadas); Serial.println(F(" amostras."));
        bool sair = confirmarSaidaCalibracao();
        if (sair) {
          Serial.println(F("  [CAL] Calibracao ABORTADA. Offsets mantidos."));
          if (!calibrado) Serial.println(F("  [CAL] ATENCAO: Offsets = 0."));
          Serial.println(F("  ==============================================="));
          emCalibracao = false; return false;
        }
      }
    }
  }

  offsetAX = somaX / NUM_AMOSTRAS;
  offsetAY = somaY / NUM_AMOSTRAS;
  offsetAZ = somaZ / NUM_AMOSTRAS;
  calibrado = true;

  Serial.println(F("  [CAL] Calibracao concluida!"));
  Serial.println(F("  [CAL] ------- Offsets -------"));
  Serial.print(F("  [CAL]   AX: ")); Serial.print(offsetAX,5); Serial.println(F(" g"));
  Serial.print(F("  [CAL]   AY: ")); Serial.print(offsetAY,5); Serial.println(F(" g"));
  Serial.print(F("  [CAL]   AZ: ")); Serial.print(offsetAZ,5); Serial.println(F(" g (inclui ~1g)"));
  Serial.println(F("  ===============================================")); Serial.println();
  salvarOffsetNVS();
  return true;
}

// =============================================================================
//  FUNCAO: menuCalibracaoSetup
// =============================================================================
void menuCalibracaoSetup() {
  emCalibracao = true;
  Serial.println(F("  [NVS] Verificando offsets na flash..."));
  bool ok = carregarOffsetNVS();
  if (ok) {
    Serial.println(F("  [NVS] Offsets carregados!"));
    Serial.print(F("  [NVS]   AX:")); Serial.print(offsetAX,5); Serial.print(F("  AY:")); Serial.print(offsetAY,5); Serial.print(F("  AZ:")); Serial.println(offsetAZ,5);
    Serial.println(F("  [NVS] Calibracao automatica PULADA."));
    Serial.println(F("  [NVS] Envie 'C' para recalibrar.")); Serial.println();
  } else {
    Serial.println(F("  [NVS] Nenhum offset. Calibracao automatica..."));
    delay(2000);
    bool calOk = executarCalibracao();
    if (!calOk) { Serial.println(F("  [SIGMA] Calibracao cancelada. Offsets=0.")); Serial.println(); }
  }
  emCalibracao = false;
}

// =============================================================================
//  FUNCAO: menuCalibracaoLoop
// =============================================================================
void menuCalibracaoLoop() {
  emCalibracao = true;
  Serial.println(F("")); Serial.println(F("  [CMD] Recalibracao recebida!"));
  bool confirmado = aguardarConfirmacaoEntrada(30000);
  if (!confirmado) { Serial.println(F("  [CMD] Cancelado.")); Serial.println(); emCalibracao = false; return; }
  bool ok = executarCalibracao();
  if (!ok) { Serial.println(F("  [INFO] Interrompida. Offsets mantidos.")); Serial.println(); emCalibracao = false; return; }
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
  Serial.println(F("  |  Sensor deve estar PARADO e NIVELADO!            |"));
  Serial.println(F("  |  S = CONFIRMAR  |  N = CANCELAR                 |"));
  Serial.println(F("  +--------------------------------------------------+"));

  uint32_t inicio = millis(); uint32_t segs = timeoutMs/1000; uint32_t ultSeg = millis();
  Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s\r"));

  while ((millis() - inicio) < timeoutMs) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg=millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r=='S'||r=='s') { Serial.println(F("  [OK] CONFIRMADA.")); return true; }
      if (r=='N'||r=='n') { Serial.println(F("  [INFO] CANCELADA.")); return false; }
    }
    delay(50);
  }
  Serial.println(F("")); Serial.println(F("  [TIMEOUT] Cancelada automaticamente.")); return false;
}

// =============================================================================
//  FUNCAO: confirmarSaidaCalibracao
// =============================================================================
bool confirmarSaidaCalibracao() {
  while (Serial.available()) Serial.read();
  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |       CONFIRMAR SAIDA DA CALIBRACAO?             |"));
  Serial.println(F("  |  Amostras serao DESCARTADAS.                     |"));
  Serial.println(F("  |  S = SAIR  |  N = CONTINUAR                     |"));
  Serial.println(F("  +--------------------------------------------------+"));

  const uint32_t TO = 60000;
  uint32_t inicio = millis(); uint32_t segs = TO/1000; uint32_t ultSeg = millis();
  Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s\r"));

  while ((millis() - inicio) < TO) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg=millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r=='S'||r=='s') { Serial.println(F("  [CAL] Saida CONFIRMADA.")); return true; }
      if (r=='N'||r=='n') { Serial.println(F("  [CAL] Retomando...")); return false; }
    }
    delay(50);
  }
  Serial.println(F("")); Serial.println(F("  [TIMEOUT] Coleta retomada.")); return false;
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
  Serial.println(F("  +----------------------------------------------------+"));

  const uint32_t TO = 60000;
  uint32_t inicio = millis(); uint32_t segs = TO/1000; uint32_t ultSeg = millis();
  Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s\r"));

  while ((millis() - inicio) < TO) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg=millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }
    if (Serial.available() > 0) {
      char r = Serial.read(); Serial.println(F(""));
      if (r=='S'||r=='s') { Serial.println(F("  [MENU] Monitoramento retomado.")); Serial.println(); emCalibracao=false; return; }
      if (r=='R'||r=='r') {
        Serial.println(F("  [MENU] Nova calibracao...")); delay(1000);
        bool ok = executarCalibracao();
        if (ok) { menuPosCalibracao(); } else { Serial.println(F("  [INFO] Interrompida.")); Serial.println(); emCalibracao=false; }
        return;
      }
    }
    delay(50);
  }
  Serial.println(F("")); Serial.println(F("  [TIMEOUT] Monitoramento retomado.")); Serial.println(); emCalibracao=false;
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
  Serial.print(F(  "  |  VIB_AVISO atual  : ")); Serial.print(VIB_AVISO,   2); Serial.println(F(" g                             |"));
  Serial.print(F(  "  |  VIB_CRITICA atual: ")); Serial.print(VIB_CRITICA, 2); Serial.println(F(" g                             |"));
  Serial.println(F("  |----------------------------------------------------|"));
  Serial.println(F("  |  1 = +/-  2g  Alta sens.  — bancada / laboratorio  |"));
  Serial.println(F("  |  2 = +/-  4g  Media sens. — motores pequenos       |"));
  Serial.println(F("  |  3 = +/-  8g  Uso geral   — industrial medio       |"));
  Serial.println(F("  |  4 = +/- 16g  Baixa sens. — motores pesados        |"));
  Serial.println(F("  |  M = Ajuste MANUAL dos limiares                    |"));
  Serial.println(F("  |  S = SAIR sem alterar                              |"));
  Serial.println(F("  +====================================================+"));

  const uint32_t TO = 60000;
  uint32_t inicio = millis(); uint32_t segs = TO/1000; uint32_t ultSeg = millis();
  bool menuAtivo = true;
  Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s\r"));

  while (menuAtivo && (millis() - inicio) < TO) {
    esp_task_wdt_reset();
    if ((millis() - ultSeg) >= 1000) { ultSeg=millis(); segs--; Serial.print(F("  Aguardando... tempo restante: ")); Serial.print(segs); Serial.print(F(" s   \r")); }

    if (Serial.available() > 0) {
      char opcao = Serial.read(); Serial.println(F(""));
      if (opcao=='S'||opcao=='s') { Serial.println(F("  [SENS] Mantida. Saindo...")); menuAtivo=false; break; }

      mpu6050_accel_range_t novaEnum; int novaG=0; float sAv=0,sCr=0; bool valida=false;
      if      (opcao=='1') { novaG=2;  novaEnum=MPU6050_RANGE_2_G;  sAv=0.5; sCr=1.0; valida=true; }
      else if (opcao=='2') { novaG=4;  novaEnum=MPU6050_RANGE_4_G;  sAv=1.0; sCr=2.0; valida=true; }
      else if (opcao=='3') { novaG=8;  novaEnum=MPU6050_RANGE_8_G;  sAv=2.0; sCr=4.0; valida=true; }
      else if (opcao=='4') { novaG=16; novaEnum=MPU6050_RANGE_16_G; sAv=4.0; sCr=8.0; valida=true; }

      if (valida) {
        Serial.print(F("  [SENS] Escala: +/- ")); Serial.print(novaG); Serial.println(F(" g"));
        Serial.print(F("  [SENS] Sugestao — Av: ")); Serial.print(sAv,2); Serial.print(F("  Cr: ")); Serial.print(sCr,2); Serial.println(F(" g"));
        Serial.println(F("  S=Aplicar | M=Manual | N=Manter"));
        while (Serial.available()) Serial.read();
        uint32_t iL=millis(); bool ag=true;
        while (ag && (millis()-iL)<30000) {
          esp_task_wdt_reset();
          if (Serial.available()>0) {
            char res=Serial.read(); Serial.println(F(""));
            if (res=='S'||res=='s') { VIB_AVISO=sAv; VIB_CRITICA=sCr; Serial.println(F("  [SENS] Sugestao aplicada.")); ag=false; }
            else if (res=='M'||res=='m') {
              Serial.println(F("  VIB_AVISO + Enter:"));
              while(Serial.available()) Serial.read(); String sA=""; uint32_t t=millis();
              while((millis()-t)<30000){esp_task_wdt_reset(); if(Serial.available()){char c=Serial.read(); if(c=='\n'||c=='\r') break; sA+=c;}}
              Serial.println(F("  VIB_CRITICA + Enter:"));
              while(Serial.available()) Serial.read(); String sC=""; t=millis();
              while((millis()-t)<30000){esp_task_wdt_reset(); if(Serial.available()){char c=Serial.read(); if(c=='\n'||c=='\r') break; sC+=c;}}
              float nA=sA.toFloat(),nC=sC.toFloat();
              if(nA>0&&nC>0&&nA<nC){VIB_AVISO=nA;VIB_CRITICA=nC;Serial.println(F("  [SENS] Manuais aplicados."));}
              else{Serial.println(F("  [SENS] ERRO invalidos."));}
              ag=false;
            } else if(res=='N'||res=='n'){Serial.println(F("  [SENS] Mantidos.")); ag=false;}
          }
          delay(50);
        }
        if(ag){VIB_AVISO=sAv;VIB_CRITICA=sCr;Serial.println(F("  [TIMEOUT] Sugestao aplicada."));}
        mpu.setAccelerometerRange(novaEnum); escalaAtualG=novaG; salvarConfigNVS();
        Serial.println(F("  [SENS] Recalibrando apos mudanca..."));
        delay(2000); calibrado=false; offsetAX=0; offsetAY=0; offsetAZ=0;
        executarCalibracao(); menuAtivo=false;

      } else if(opcao=='M'||opcao=='m') {
        Serial.println(F("  [SENS] Ajuste manual."));
        Serial.println(F("  VIB_AVISO + Enter:"));
        while(Serial.available()) Serial.read(); String sA=""; uint32_t t=millis();
        while((millis()-t)<30000){esp_task_wdt_reset(); if(Serial.available()){char c=Serial.read(); if(c=='\n'||c=='\r') break; sA+=c;}}
        Serial.println(F("  VIB_CRITICA + Enter:"));
        while(Serial.available()) Serial.read(); String sC=""; t=millis();
        while((millis()-t)<30000){esp_task_wdt_reset(); if(Serial.available()){char c=Serial.read(); if(c=='\n'||c=='\r') break; sC+=c;}}
        float nA=sA.toFloat(),nC=sC.toFloat();
        if(nA>0&&nC>0&&nA<nC){VIB_AVISO=nA;VIB_CRITICA=nC;salvarConfigNVS();Serial.println(F("  [SENS] Salvos na NVS."));}
        else{Serial.println(F("  [SENS] ERRO invalidos."));}
        menuAtivo=false;
      } else { Serial.println(F("  [SENS] Invalido. 1,2,3,4,M ou S.")); }
    }
    delay(50);
  }

  if(menuAtivo){Serial.println(F(""));Serial.println(F("  [TIMEOUT] Menu encerrado."));}
  Serial.println(F(""));
  Serial.println(F("  [SENS] -------- Config Ativa --------"));
  Serial.print(F(  "  [SENS]   Escala : +/- ")); Serial.print(escalaAtualG); Serial.println(F(" g"));
  Serial.print(F(  "  [SENS]   Aviso  : ")); Serial.print(VIB_AVISO,2); Serial.println(F(" g"));
  Serial.print(F(  "  [SENS]   Crit.  : ")); Serial.print(VIB_CRITICA,2); Serial.println(F(" g"));
  Serial.println(F("  [SENS] Monitoramento retomado.")); Serial.println();
  emCalibracao=false;
}

// =============================================================================
//  FUNCAO: calcularVibracaoRMS
// =============================================================================
float calcularVibracaoRMS() {
  const int N = 50; float somaQ = 0.0;
  sensors_event_t accel, gyro, temp;
  for (int i = 0; i < N; i++) {
    mpu.getEvent(&accel, &gyro, &temp);
    float ax = (accel.acceleration.x/9.81) - offsetAX;
    float ay = (accel.acceleration.y/9.81) - offsetAY;
    float az = (accel.acceleration.z/9.81) - offsetAZ;
    somaQ += (ax*ax)+(ay*ay)+(az*az); delay(2);
  }
  return sqrt(somaQ / N);
}

// =============================================================================
//  FUNCAO: calcularHealthScore
//  Quando baseline ativo: componente vibracao usa desvio em sigmas.
//  Quando baseline inativo: usa limiares absolutos VIB_AVISO/VIB_CRITICA.
// =============================================================================
float calcularHealthScore(float temp, float vib, float horas) {
  float sT, sV, sH;

  if      (temp <= TEMP_AVISO_MAX) sT = 1.0;
  else if (temp >= TEMP_CRITICA)   sT = 0.0;
  else sT = 1.0 - ((temp-TEMP_AVISO_MAX)/(TEMP_CRITICA-TEMP_AVISO_MAX));

  if (basAtivo && basStddev > 0.0) {
    // Componente vibracao baseada em desvio relativo (sigmas)
    float desvioSigma = (vib - basMedia) / basStddev;
    if      (desvioSigma <= 0.0)            sV = 1.0; // Abaixo da media
    else if (desvioSigma >= 2.0 * fatorK)   sV = 0.0; // Acima do limiar critico
    else if (desvioSigma <= fatorK)          sV = 1.0; // Dentro do range normal
    else sV = 1.0 - ((desvioSigma - fatorK) / fatorK); // Degradacao linear
  } else {
    // Sem baseline: usa limiares absolutos
    if      (vib <= VIB_AVISO)   sV = 1.0;
    else if (vib >= VIB_CRITICA) sV = 0.0;
    else sV = 1.0 - ((vib-VIB_AVISO)/(VIB_CRITICA-VIB_AVISO));
  }

  float lim = INTERVALO_MANUTENCAO_H * 0.80;
  if      (horas <= lim)                    sH = 1.0;
  else if (horas >= INTERVALO_MANUTENCAO_H) sH = 0.0;
  else sH = 1.0 - ((horas-lim)/(INTERVALO_MANUTENCAO_H-lim));

  return (sT*40.0)+(sV*40.0)+(sH*20.0);
}

// =============================================================================
//  FUNCAO: classificarAlarme
//  Quando baseline ativo: usa limiares relativos (media + K*stddev).
//  Quando baseline inativo: usa VIB_AVISO / VIB_CRITICA absolutos.
// =============================================================================
String classificarAlarme(float temp, float vib) {
  float limAviso, limCritico;

  if (basAtivo) {
    limAviso   = basMedia + fatorK * basStddev;
    limCritico = basMedia + 2.0 * fatorK * basStddev;
  } else {
    limAviso   = VIB_AVISO;
    limCritico = VIB_CRITICA;
  }

  if (temp >= TEMP_CRITICA || vib >= limCritico) return "*** CRITICO ***";
  if (temp >= TEMP_AVISO_MAX || vib >= limAviso)  return "    AVISO    ";
  return "    NORMAL   ";
}

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
  float hR = INTERVALO_MANUTENCAO_H - horimetro;
  if (hR < 0) hR = 0;

  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Timestamp       : ")); Serial.print(millis()); Serial.println(F(" ms"));
  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Temperatura     : ")); Serial.print(temperatura,1); Serial.println(F(" oC"));
  Serial.print(F("  Vibracao        : ")); Serial.print(vibracaoRMS,3); Serial.println(F(" g"));

  // Exibe baseline quando ativo
  if (basAtivo) {
    float limAv = basMedia + fatorK * basStddev;
    float limCr = basMedia + 2.0 * fatorK * basStddev;
    float desvio = (vibracaoRMS - basMedia) / basStddev;
    Serial.print(F("  Baseline Vib.   : ")); Serial.print(basMedia,4); Serial.print(F(" g  |  Stddev: ")); Serial.print(basStddev,4); Serial.println(F(" g"));
    Serial.print(F("  Desvio Atual    : ")); Serial.print(desvio,2); Serial.print(F(" sigma  |  K=")); Serial.print(fatorK,1); Serial.print(F("  |  Lim.Av: ")); Serial.print(limAv,4); Serial.print(F(" g  |  Lim.Cr: ")); Serial.print(limCr,4); Serial.println(F(" g"));
  } else {
    Serial.println(F("  Baseline Vib.   : Nao coletado (usando limiares absolutos)"));
  }

  Serial.print(F("  Horimetro       : ")); Serial.print(horimetro,4); Serial.print(F(" h  (base: ")); Serial.print(horasBase,1); Serial.println(F(" h)"));
  Serial.print(F("  Prox. Manut.    : ")); Serial.print(hR,1); Serial.println(F(" h restantes"));
  Serial.print(F("  Health Score    : ")); Serial.print(healthScore,1); Serial.print(F(" %  (")); Serial.print(obterClassificacaoHealth(healthScore)); Serial.println(F(")"));
  Serial.print(F("  Escala Acel.    : +/- ")); Serial.print(escalaAtualG); Serial.print(F(" g  |  Aviso: ")); Serial.print(VIB_AVISO,2); Serial.print(F(" g  |  Critico: ")); Serial.print(VIB_CRITICA,2); Serial.println(F(" g"));
  Serial.print(F("  Offsets NVS     : ")); Serial.println(calibrado ? F("Salvos") : F("Zerados"));
  Serial.print(F("  Resets Sistema  : ")); Serial.print(resetCount); Serial.print(F("  | Ultimo: ")); Serial.println(resetMotivo);
  Serial.print(F("  Alarme          : ")); Serial.println(statusAlarme);
  Serial.println(F("------------------------------------------------")); Serial.println();
}

// =============================================================================
//  SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("=============================================="));
  Serial.println(F("  Project SIGMA v0.1.5.0 - Iniciando..."));
  Serial.println(F("=============================================="));

  // Hardware Watchdog — ESP-IDF v5.x
  esp_task_wdt_config_t wdtCfg = {
    .timeout_ms     = WDT_TIMEOUT_MS,
    .idle_core_mask = 0,
    .trigger_panic  = true
  };
  esp_task_wdt_reconfigure(&wdtCfg);
  esp_task_wdt_add(NULL);
  Serial.print(F("  [WDT] Watchdog configurado (")); Serial.print(WDT_TIMEOUT_MS/1000); Serial.println(F("s)."));

  // Carrega NVS
  carregarResetNVS();
  Serial.print(F("  [SYS] Resets: ")); Serial.print(resetCount); Serial.print(F(" | ")); Serial.println(resetMotivo);
  horasBase = carregarHorimetroNVS();
  Serial.print(F("  [NVS] Horimetro: ")); Serial.print(horasBase > 0 ? horasBase : 0, 4); Serial.println(F(" h"));

  bool cfgOk = carregarConfigNVS();
  if (cfgOk) { Serial.print(F("  [NVS] Config: Escala=+/-")); Serial.print(escalaAtualG); Serial.print(F("g  Av=")); Serial.print(VIB_AVISO,2); Serial.print(F("  Cr=")); Serial.print(VIB_CRITICA,2); Serial.println(F(" g")); }
  else { Serial.println(F("  [NVS] Config: padroes de fabrica.")); }

  bool basOk = carregarBaselineNVS();
  if (basOk) {
    Serial.println(F("  [NVS] Baseline carregado (sigma_bas):"));
    Serial.print(F("  [NVS]   Media: ")); Serial.print(basMedia,4); Serial.print(F(" g  |  Stddev: ")); Serial.print(basStddev,4); Serial.print(F(" g  |  K=")); Serial.println(fatorK,1);
  } else { Serial.println(F("  [NVS] Baseline: nenhum. Envie 'B' para coletar.")); }

  // I2C + MPU6050
  Wire.begin(PINO_MPU_SDA, PINO_MPU_SCL);
  Wire.setClock(400000); delay(150);
  Serial.println(F("  [I2C] Iniciado (400 kHz)."));

  bool mpuOk = false;
  for (int t = 1; t <= 5; t++) {
    esp_task_wdt_reset();
    Serial.print(F("  [I2C] Tentativa ")); Serial.print(t); Serial.print(F("/5..."));
    if (mpu.begin()) { mpuOk=true; Serial.println(F(" OK!")); break; }
    Serial.println(F(" falhou.")); delay(200);
  }
  if (!mpuOk) {
    Serial.println(F("  [ERRO FATAL] MPU6050 nao encontrado!"));
    Serial.println(F("  Verifique pull-ups SDA/SCL, VCC=3.3V, GND."));
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }

  mpu6050_accel_range_t escEnum = MPU6050_RANGE_8_G;
  if      (escalaAtualG==2)  escEnum = MPU6050_RANGE_2_G;
  else if (escalaAtualG==4)  escEnum = MPU6050_RANGE_4_G;
  else if (escalaAtualG==16) escEnum = MPU6050_RANGE_16_G;
  mpu.setAccelerometerRange(escEnum);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print(F("  [MPU] Escala: +/-")); Serial.print(escalaAtualG); Serial.println(F("g | Gyro:500dps | Filtro:21Hz"));

  // Calibracao
  menuCalibracaoSetup();

  // DS18B20
  sensorTemp.begin();
  int qtd = sensorTemp.getDeviceCount();
  Serial.print(F("  [DS18B20] Sensores: ")); Serial.println(qtd);
  if (qtd==0) { Serial.println(F("  [AVISO] DS18B20 nao encontrado!")); }
  else { sensorTemp.setResolution(12); Serial.println(F("  [DS18B20] 12 bits.")); }

  inicioSistema = millis(); ultimaLeitura = millis(); ultimoSaveHorimetro = millis();

  Serial.println(F("=============================================="));
  Serial.println(F("  Sistema pronto!"));
  Serial.println(F("  C=Calibrar   A=Sensibilidade  B=Baseline"));
  Serial.println(F("  N=Limpar NVS H=Zerar Hor.     R=Reset Cfg"));
  Serial.println(F("  K=Fator K    Z=Apagar Baseline"));
  Serial.println(F("==============================================")); Serial.println();
}

// =============================================================================
//  LOOP
// =============================================================================
void loop() {
  unsigned long agora = millis();
  esp_task_wdt_reset();

  // Comandos Serial
  if (!emCalibracao && Serial.available() > 0) {
    char cmd = Serial.read();
    if      (cmd=='C'||cmd=='c') { menuCalibracaoLoop(); }
    else if (cmd=='A'||cmd=='a') { Serial.println(F("  [CMD] Sensibilidade!")); menuSensibilidade(); }
    else if (cmd=='N'||cmd=='n') { apagarOffsetNVS(); menuCalibracaoLoop(); }
    else if (cmd=='H'||cmd=='h') { zerarHorimetroNVS(); }
    else if (cmd=='R'||cmd=='r') { confirmarResetConfig(); }
    else if (cmd=='B'||cmd=='b') { coletarBaseline(); }
    else if (cmd=='K'||cmd=='k') { ajustarFatorK(); }
    else if (cmd=='Z'||cmd=='z') { apagarBaselineNVS(); }
  }

  // Salvamento automatico do horimetro (5 min)
  if (!emCalibracao && (agora - ultimoSaveHorimetro) >= INTERVALO_SAVE_HORIMETRO_MS) {
    ultimoSaveHorimetro = agora; salvarHorimetroNVS();
  }

  // Leitura dos sensores
  if (!emCalibracao && (agora - ultimaLeitura) >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora;
    horimetro = horasBase + (float)(agora - inicioSistema) / 3600000.0;

    sensorTemp.requestTemperatures();
    float lT = sensorTemp.getTempCByIndex(0);
    if (lT != DEVICE_DISCONNECTED_C) { temperatura=lT; falhasDS18B20=0; }
    else {
      falhasDS18B20++;
      Serial.print(F("  [AVISO] Falha DS18B20 (")); Serial.print(falhasDS18B20); Serial.print(F("/")); Serial.print(MAX_FALHAS_DS18B20); Serial.println(F(")"));
      if (falhasDS18B20 >= MAX_FALHAS_DS18B20) { reiniciarSistema("Falha consecutiva DS18B20 (5x)"); }
    }

    vibracaoRMS  = calcularVibracaoRMS();
    healthScore  = calcularHealthScore(temperatura, vibracaoRMS, horimetro);
    statusAlarme = classificarAlarme(temperatura, vibracaoRMS);
    imprimirRelatorio();
  }
}
