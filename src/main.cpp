// =============================================================
//  Project SIGMA
//  Arquivo    : main.cpp
//  Descricao  : Camada 5 (Application) — Orquestracao principal.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.11.0
//  Codename   : Historico de Eventos
//  Data       : 2026-06-27
// =============================================================

#include <Arduino.h>
#include <esp_task_wdt.h>

// HAL e Drivers
#include "hal/PinConfig.h"
#include "drivers/Ds18b20Driver.h"
#include "drivers/Mpu6050Driver.h"

// Storage
#include "storage/NvsCalibration.h"
#include "storage/NvsHorimeter.h"
#include "storage/NvsConfig.h"
#include "storage/NvsBaseline.h"

// Services e UI
#include "services/HealthService.h"
#include "services/AlarmService.h"
#include "ui/SerialUI.h"

// Analytics
#include "hal/Version.h"
#include "services/analytics/AnalyticsBuffer.h"
#include "services/analytics/AnalyticsEngine.h"

#include "ui/CommandHandler.h"

#include "services/CalibrationService.h"
#include "services/filter/DigitalFilter.h"
#include "services/filter/OutlierFilter.h"
#include "services/SensorQualityService.h"
#include "services/event/EventTypes.h"
#include "services/event/EventManager.h"
#include "services/event/EventHistory.h"

// =============================================================
//  INSTANCIAS GLOBAIS DE ORQUESTRACAO
// =============================================================
Ds18b20Driver      driverTemp;
Mpu6050Driver      driverVib;

NvsCalibration     nvsCal;
NvsHorimeter       nvsHor;
NvsConfig          nvsCfg;
NvsBaseline        nvsBas;

HealthService      srvHealth;
AlarmService       srvAlarm;
SerialUI           ui;

AnalyticsEngine    analytics;

DigitalFilter filtroTemp;
DigitalFilter filtroVib;
OutlierFilter outlierTemp;
OutlierFilter outlierVib;
SensorQualityService sensorQuality;
EventManager eventBus;
EventHistory eventHistory;

CommandHandler     cmdHandler;

CalibrationService srvCal;

// =============================================================
//  ESTADO DA APLICACAO (Em memoria)
// =============================================================
NvsCalibrationData calData;
NvsHorimeterData   horData;
NvsConfigData      cfgData;
NvsBaselineData    basData;

// Estado dos filtros DC para remocao de gravidade
static float _dcX = 0.0f;
static float _dcY = 0.0f;
static float _dcZ = 0.0f;

uint32_t ultimaLeituraMs = 0;
uint32_t inicioSistemaMs = 0;

// =============================================================
//  CALLBACK DE EVENTO (T014)
// =============================================================
static void onEvento(EventType tipo, const EventoDados& dados) {
  eventHistory.registrar(millis(), tipo, dados);
  #ifdef SIGMA_DEBUG_EVENTOS
  Serial.print(F("  [EVT] Tipo="));
  Serial.print((uint8_t)tipo);
  Serial.print(F(" v1="));
  Serial.print(dados.valorPrincipal, 3);
  Serial.print(F(" v2="));
  Serial.print(dados.valorSecundario, 3);
  Serial.println(F(""));
  #endif
}

// =============================================================
//  SETUP
// =============================================================
void setup() {
  ui.begin(115200);
  ui.imprimirCabecalhoBoot();

  // Watchdog Configuration
  esp_task_wdt_config_t wdtCfg = {
    .timeout_ms = 10000, .idle_core_mask = 0, .trigger_panic = true
  };
  esp_task_wdt_reconfigure(&wdtCfg);
  esp_task_wdt_add(NULL);
  ui.imprimirMensagem("WDT", "Watchdog configurado (timeout: 10s).");

  // Load Storage (Camada 3)
  nvsHor.carregar(horData);
  nvsCfg.carregar(cfgData);
  nvsCal.carregar(calData);
  nvsBas.carregar(basData);
  ui.imprimirMensagem("SYS", "Estado persistente carregado da NVS.");

  // Hardware Initialization (Camada 1 e 2)
  Wire.begin(PINO_MPU_SDA, PINO_MPU_SCL);
  Wire.setClock(400000);
  
  if (!driverVib.begin()) {
    ui.imprimirErroFatal("MPU6050 nao encontrado.", "Verifique conexoes I2C.");
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }
  driverVib.configurarEscala(cfgData.escalaG);
  ui.imprimirMensagem("OK", "MPU6050 inicializado.");

  if (!driverTemp.begin(PINO_ONE_WIRE)) {
    ui.imprimirMensagem("WARN", "Nenhum DS18B20 detectado!");
  } else {
    ui.imprimirMensagem("OK", "DS18B20 inicializado.");
  }

  // Configuracao dos filtros digitais (Fase 2 — T011)
  FilterConfig cfgTemp;
  cfgTemp.tipo   = FilterType::MEDIA_MOVEL;
  cfgTemp.janela = 5;
  cfgTemp.alfa   = 0.3f;
  filtroTemp.configurar(cfgTemp);

  FilterConfig cfgVib;
  cfgVib.tipo   = FilterType::MEDIA_MOVEL;
  cfgVib.janela = 5;
  cfgVib.alfa   = 0.3f;
  filtroVib.configurar(cfgVib);

  ui.imprimirMensagem("INFO", "Filtros digitais configurados.");

  // Configuracao dos filtros de outlier (T012)
  outlierTemp.configurar(-55.0f, 125.0f);
  outlierVib.configurar(0.0f, (float)cfgData.escalaG);
  ui.imprimirMensagem("OK", "Filtros de outlier configurados.");

  // Registro de callbacks do barramento de eventos (T014)
  for (uint8_t i = 0; i < (uint8_t)EventType::QTDE; i++) {
    eventBus.inscrever((EventType)i, onEvento);
  }
  ui.imprimirMensagem("OK", "Barramento de eventos inicializado.");

  inicioSistemaMs = millis();
  ui.imprimirMensagem("INFO", "Monitoramento iniciado.");
}

// =============================================================
//  LOOP
// =============================================================
void loop() {
  esp_task_wdt_reset();
  uint32_t agora = millis();

  // Instancia dinamica do contexto contendo apenas as referencias
  CommandContext cmdCtx = {
    .nvsHor = nvsHor,       .horData = horData,
    .nvsBas = nvsBas,       .basData = basData,
    .nvsCal = nvsCal,       .calData = calData,
    .nvsCfg = nvsCfg,       .cfgData = cfgData,
    .driverVib = driverVib, .srvCal  = srvCal, 
    .ui = ui,               .inicioSistemaMs = inicioSistemaMs
  };

  // Despacha para o modulo UI encarregado
  cmdHandler.processar(cmdCtx);

  // Se calibracao estiver ativa, nao imprime estatisticas
  if (cmdHandler.isCalibrating()) return;

  // Tratamento de comandos na UI poderia ser acionado aqui:
  // if (Serial.available()) processarComandos();

  // Ciclo de leitura (500ms)
  if (agora - ultimaLeituraMs >= 500) {
    ultimaLeituraMs = agora;

    // Atualiza horimetro em RAM
    float horasRodando = (float)(agora - inicioSistemaMs) / 3600000.0f;
    float horasTotais  = horData.horimetro + horasRodando;

    // Leitura Camada 2
    Ds18b20Data tempDado = driverTemp.lerTemperatura();
    Mpu6050Data vibDado  = driverVib.lerAceleracao(calData.offsetAX, calData.offsetAY, calData.offsetAZ);

    // Validacao estrita e filtragem digital
    float tBruta = tempDado.valido ? tempDado.temperaturaCelsius : 0.0f;
    float vBruta = 0.0f;

    sensorQuality.atualizarTemp(tempDado.valido);
    if (tempDado.valido) {
      tBruta = outlierTemp.filtrar(tBruta);
      if (isnan(tBruta)) {
        tBruta = 0.0f;
        EventoDados ev = { tempDado.temperaturaCelsius, 0.0f, 0, 0 };
        eventBus.disparar(EventType::OUTLIER_TEMP, ev);
      }
    }
    {
      EventoDados ev = { tBruta, 0.0f, 0, (uint8_t)(tempDado.valido ? 0 : 1) };
      eventBus.disparar(EventType::TEMP_LEITURA, ev);
    }
    
    sensorQuality.atualizarVib(vibDado.valido);
    if (vibDado.valido) {
      // Remove DC (gravidade + bias) de cada eixo via EMA lenta
      // para que o RMS reflita apenas a componente vibratoria
      const float alfaDC = 0.005f; // ~100s de constante no 500ms
      float axAC = vibDado.ax - _dcX;
      float ayAC = vibDado.ay - _dcY;
      float azAC = vibDado.az - _dcZ;
      _dcX += alfaDC * (vibDado.ax - _dcX);
      _dcY += alfaDC * (vibDado.ay - _dcY);
      _dcZ += alfaDC * (vibDado.az - _dcZ);

      // RMS apenas da energia dinamica (AC)
      vBruta = sqrt((axAC * axAC) + (ayAC * ayAC) + (azAC * azAC));

      // Rejeita outliers por Z-Score
      vBruta = outlierVib.filtrar(vBruta);
      if (isnan(vBruta)) {
        vBruta = 0.0f;
        EventoDados ev = { 0.0f, 0.0f, 1, 0 };
        eventBus.disparar(EventType::OUTLIER_VIB, ev);
      }
    }
    {
      EventoDados ev = { vBruta, 0.0f, 1, 0 };
      eventBus.disparar(EventType::VIB_LEITURA, ev);
    }

    // Aplica filtro digital as leituras brutas
    float tAtual = filtroTemp.filtrar(tBruta);
    float vAtual = filtroVib.filtrar(vBruta);

    // Estruturacao do contexto para Camada 4
    HealthContext hCtx = {
      .tempAvisoMax = 55.0f, .tempCritica = 65.0f, .intervaloManutencaoH = 500.0f,
      .basAtivo = basData.basAtivo, .basMedia = basData.basMedia,
      .basStddev = basData.basStddev, .fatorK = basData.fatorK,
      .vibAviso = cfgData.vibAviso, .vibCritica = cfgData.vibCritica
    };

    AlarmContext aCtx = {
      .tempAvisoMax = 55.0f, .tempCritica = 65.0f,
      .basAtivo = basData.basAtivo, .basMedia = basData.basMedia,
      .basStddev = basData.basStddev, .fatorK = basData.fatorK,
      .vibAviso = cfgData.vibAviso, .vibCritica = cfgData.vibCritica
    };

    // Calculos Camada 4
    // 1. Processamento e Classificacao (Instantâneo)
    float score        = srvHealth.calcularScore(tAtual, vAtual, horasTotais, hCtx);
    const char* txtH   = srvHealth.classificar(score);
    const char* txtAlm = srvAlarm.classificar(tAtual, vAtual, aCtx);

    // 2. Empacotamento Seguro via Enum Class
    AlarmLevel almCode = AlarmLevel::NORMAL;
    if (strcmp(txtAlm, "AVISO") == 0)   almCode = AlarmLevel::WARNING;
    if (strcmp(txtAlm, "CRITICO") == 0) almCode = AlarmLevel::CRITICAL;

    // Dispara eventos de alarme (T014)
    if (almCode != AlarmLevel::NORMAL) {
      EventoDados ev = { tAtual, vAtual, 0, (uint8_t)almCode };
      eventBus.disparar(EventType::TEMP_ALARME, ev);
      eventBus.disparar(EventType::VIB_ALARME, ev);
    }
    {
      EventoDados ev = { score, 0.0f, 2, 0 };
      eventBus.disparar(EventType::SAUDE_ALTERADA, ev);
    }

    AnalyticsSample novaAmostra = {
      .timestamp   = agora,
      .temperature = tAtual,
      .vibration   = vAtual,
      .health      = score,
      .runtime     = horasTotais,
      .alarm       = almCode
    };
    
    // 3. Insercao na Fachada Analitica
    analytics.processSample(novaAmostra);

    // 4. Extracao sob demanda da API (Regressao Linear + Medias)
    AnalyticsResult res = analytics.getLatestResult();

    SensorQualityReport qRel = sensorQuality.obterRelatorio();

    // 5. Renderizacao Camada UI conectada a API
    ui.imprimirRelatorio(agora, tAtual, vAtual, horasTotais, score, txtH, txtAlm, res, qRel);
  }

  // Processa eventos enfileirados (T014)
  eventBus.processarFila();
}
