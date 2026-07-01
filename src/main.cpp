// =============================================================
//  Project SIGMA
//  Arquivo    : main.cpp
//  Descricao  : Camada 5 (Application) — Orquestracao principal.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.17.0
//  Codename   : Boot Diagnostics
//  Data       : 2026-06-27
// =============================================================

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>

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
#include "services/alarm/AlarmManager.h"
#include "services/diag/DiagnosticReport.h"
#include "storage/NvsAlarm.h"
#include "storage/NvsBoot.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

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

AlarmManager       alarmManager;
NvsAlarm           nvsAlarm;
NvsBoot            nvsBoot;
DiagnosticReport   diag;
bool               _ds18b20Ok = false;

CommandHandler     cmdHandler;

CalibrationService srvCal;

// =============================================================
//  FreeRTOS — Fila e Mutex (Fase 1 — Multitarefa SENSOR)
// =============================================================
struct SensorQueueItem {
  Ds18b20Data temp;
  Mpu6050Data vib;
  uint32_t timestamp;
};

xQueueHandle      sensorQueue;
SemaphoreHandle_t i2cMutex;
TaskHandle_t      sensorTaskHandle;
TaskHandle_t      eventTaskHandle;
TaskHandle_t      uiTaskHandle;
TaskHandle_t      processingTaskHandle;

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
//  TAREFA: sensorTask (Nucleo 1, prioridade 3)
//  Le sensores a cada 500ms e envia para fila.
//  MPU6050 (I2C) protegido por mutex.
//  DS18B20 (OneWire) nao precisa de mutex.
// =============================================================
static void sensorTask(void* pvParams) {
  (void)pvParams;
  uint32_t ultLeitura = 0;
  while (true) {
    uint32_t agora = millis();
    if (agora - ultLeitura >= 500) {
      ultLeitura = agora;

      SensorQueueItem item;
      item.timestamp = agora;

      // Le temperatura (OneWire)
      item.temp = driverTemp.lerTemperatura();

      // Le vibracao (I2C) com mutex
      if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        item.vib = driverVib.lerAceleracao(
          calData.offsetAX,
          calData.offsetAY,
          calData.offsetAZ);
        xSemaphoreGive(i2cMutex);
      } else {
        item.vib.valido = false;
      }

      // Envia para fila (overwrite — mantem sempre o dado mais
      // recente, descartando leituras antigas nao consumidas)
      xQueueOverwrite(sensorQueue, &item);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// =============================================================
//  TAREFA: eventTask (Nucleo 1, prioridade 2)
//  Processa fila de eventos a cada 50ms.
//  Desacopla o dispacho de eventos do barramento do ciclo
//  principal de leitura/processamento.
// =============================================================
static void eventTask(void* pvParams) {
  (void)pvParams;
  while (true) {
    eventBus.processarFila();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// =============================================================
//  TAREFA: uiTask (Nucleo 0, prioridade 1)
//  Reservado para futura entrada serial desacoplada.
//  Atualmente inativo — todo output serial e no core 1.
// =============================================================
static void uiTask(void* pvParams) {
  (void)pvParams;
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// =============================================================
//  TAREFA: processingTask (Nucleo 1, prioridade 3)
//  Processa dados dos sensores quando disponiveis na fila.
//  Descarrega o loop principal para resposta serial rapida.
// =============================================================
static void processingTask(void* pvParams) {
  (void)pvParams;
  SensorQueueItem item;
  while (true) {
    if (xQueueReceive(sensorQueue, &item, portMAX_DELAY) != pdTRUE)
      continue;

    // Durante calibracao, nao processa dados
    if (cmdHandler.isCalibrating()) continue;

    uint32_t agora = item.timestamp;

    float horasRodando = (float)(agora - inicioSistemaMs) / 3600000.0f;
    float horasTotais  = horData.horimetro + horasRodando;

    Ds18b20Data tempDado = item.temp;
    Mpu6050Data vibDado  = item.vib;

    // Validacao estrita e filtragem digital
    float tBruta = tempDado.valido ? tempDado.temperaturaCelsius : 0.0f;
    float vBruta = 0.0f;

    sensorQuality.atualizarTemp(tempDado.valido);
    if (tempDado.valido) {
      tBruta = outlierTemp.filtrar(tBruta);
      if (isnan(tBruta)) {
        tBruta = 0.0f;
        EventoDados ev = { tempDado.temperaturaCelsius, 0.0f, 0, 0 };
        eventBus.enfileirar(EventType::OUTLIER_TEMP, ev);
      }
    }
    {
      EventoDados ev = { tBruta, 0.0f, 0,
                         (uint8_t)(tempDado.valido ? 0 : 1) };
      eventBus.enfileirar(EventType::TEMP_LEITURA, ev);
    }

    sensorQuality.atualizarVib(vibDado.valido);
    if (vibDado.valido) {
      const float alfaDC = 0.005f;
      float axAC = vibDado.ax - _dcX;
      float ayAC = vibDado.ay - _dcY;
      float azAC = vibDado.az - _dcZ;
      _dcX += alfaDC * (vibDado.ax - _dcX);
      _dcY += alfaDC * (vibDado.ay - _dcY);
      _dcZ += alfaDC * (vibDado.az - _dcZ);

      vBruta = sqrt((axAC * axAC) + (ayAC * ayAC) + (azAC * azAC));
      vBruta = outlierVib.filtrar(vBruta);
      if (isnan(vBruta)) {
        vBruta = 0.0f;
        EventoDados ev = { 0.0f, 0.0f, 1, 0 };
        eventBus.enfileirar(EventType::OUTLIER_VIB, ev);
      }
    }
    {
      EventoDados ev = { vBruta, 0.0f, 1, 0 };
      eventBus.enfileirar(EventType::VIB_LEITURA, ev);
    }

    float tAtual = filtroTemp.filtrar(tBruta);
    float vAtual = filtroVib.filtrar(vBruta);

    HealthContext hCtx = {
      .tempAvisoMax = 55.0f, .tempCritica = 65.0f,
      .intervaloManutencaoH = 500.0f,
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

    float score = srvHealth.calcularScore(tAtual, vAtual,
                                          horasTotais, hCtx);
    const char* txtH   = srvHealth.classificar(score);
    const char* txtAlm = srvAlarm.classificar(tAtual, vAtual, aCtx);

    AlarmLevel almCode = AlarmLevel::NORMAL;
    if (strcmp(txtAlm, "AVISO") == 0)   almCode = AlarmLevel::WARNING;
    if (strcmp(txtAlm, "CRITICO") == 0) almCode = AlarmLevel::CRITICAL;

    // Niveis por sensor para o AlarmManager (T016)
    float limVA = cfgData.vibAviso;
    float limVC = cfgData.vibCritica;
    if (basData.basAtivo) {
      limVA = basData.basMedia +
              (basData.fatorK * basData.basStddev);
      limVC = basData.basMedia +
              (2.0f * basData.fatorK * basData.basStddev);
    }
    AlarmLevel nivelTemp = AlarmLevel::NORMAL;
    if (tAtual >= 55.0f) {
      nivelTemp = (tAtual >= 65.0f)
        ? AlarmLevel::CRITICAL : AlarmLevel::WARNING;
    }
    AlarmLevel nivelVib = AlarmLevel::NORMAL;
    if (vAtual >= limVA) {
      nivelVib = (vAtual >= limVC)
        ? AlarmLevel::CRITICAL : AlarmLevel::WARNING;
    }

    TransicaoAlarme trans = alarmManager.atualizar(
      nivelTemp, nivelVib, agora);

    if (trans.tempMudou) {
      EventoDados ev = { tAtual, vAtual, 0, (uint8_t)nivelTemp };
      eventBus.enfileirar(EventType::TEMP_ALARME, ev);
    }
    if (trans.vibMudou) {
      EventoDados ev = { tAtual, vAtual, 1, (uint8_t)nivelVib };
      eventBus.enfileirar(EventType::VIB_ALARME, ev);
    }
    if ((trans.tempMudou && !trans.tempAtivo) ||
        (trans.vibMudou && !trans.vibAtivo)) {
      AlarmaHistorico buf_[AlarmManager::MAX_HISTORICO];
      uint8_t qtd_ = alarmManager.salvarHistorico(buf_);
      if (qtd_ > 0) nvsAlarm.salvar(buf_, qtd_);
    }
    {
      EventoDados ev = { score, 0.0f, 2, 0 };
      eventBus.enfileirar(EventType::SAUDE_ALTERADA, ev);
    }

    AnalyticsSample novaAmostra = {
      .timestamp = agora, .temperature = tAtual,
      .vibration = vAtual, .health = score,
      .runtime = horasTotais, .alarm = almCode
    };
    analytics.processSample(novaAmostra);
    AnalyticsResult res = analytics.getLatestResult();
    SensorQualityReport qRel = sensorQuality.obterRelatorio();

    ui.imprimirRelatorio(agora, tAtual, vAtual, horasTotais,
                         score, txtH, txtAlm, res, qRel);
  }
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

  // Carrega historico de alarmes da NVS (T016)
  {
    AlarmaHistorico buf[AlarmManager::MAX_HISTORICO];
    uint8_t qtd = nvsAlarm.carregar(buf, AlarmManager::MAX_HISTORICO);
    if (qtd > 0) alarmManager.carregarHistorico(buf, qtd);
    ui.imprimirMensagem("ALM", "Historico de alarmes carregado.");
  }

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

  _ds18b20Ok = driverTemp.begin(PINO_ONE_WIRE);
  if (!_ds18b20Ok) {
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

  // Criacao do mutex I2C e fila de sensores (Fase 1)
  i2cMutex    = xSemaphoreCreateMutex();
  sensorQueue = xQueueCreate(1, sizeof(SensorQueueItem));
  
  if (i2cMutex == NULL || sensorQueue == NULL) {
    ui.imprimirErroFatal("Falha ao criar recursos FreeRTOS.",
                         "RAM insuficiente.");
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }
  
  // Cria a tarefa de leitura de sensores no nucleo 1
  if (xTaskCreatePinnedToCore(
        sensorTask, "SensorTask", 4096, NULL, 3,
        &sensorTaskHandle, 1) != pdPASS) {
    ui.imprimirErroFatal("Falha ao criar SensorTask.",
                         "RAM insuficiente.");
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }
  ui.imprimirMensagem("OK", "SensorTask criada (nucleo 1, prioridade 3).");

  // Cria a tarefa de processamento de eventos no nucleo 1
  if (xTaskCreatePinnedToCore(
        eventTask, "EventTask", 3072, NULL, 2,
        &eventTaskHandle, 1) != pdPASS) {
    ui.imprimirErroFatal("Falha ao criar EventTask.",
                         "RAM insuficiente.");
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }
  ui.imprimirMensagem("OK", "EventTask criada (nucleo 1, prioridade 2).");

  // Cria tarefa de UI no nucleo 0 (reservada para uso futuro)
  if (xTaskCreatePinnedToCore(
        uiTask, "UITask", 2048, NULL, 1,
        &uiTaskHandle, 0) != pdPASS) {
    ui.imprimirErroFatal("Falha ao criar UITask.",
                         "RAM insuficiente.");
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }
  ui.imprimirMensagem("OK", "UITask criada (nucleo 0, reserva).");

  // Cria a tarefa de processamento de dados no nucleo 1
  if (xTaskCreatePinnedToCore(
        processingTask, "ProcessingTask", 4096, NULL, 3,
        &processingTaskHandle, 1) != pdPASS) {
    ui.imprimirErroFatal("Falha ao criar ProcessingTask.",
                         "RAM insuficiente.");
    esp_task_wdt_delete(NULL);
    while (true) delay(1000);
  }
  ui.imprimirMensagem("OK", "ProcessingTask criada "
                      "(nucleo 1, prioridade 3).");

  // Diagnosticos de inicializacao (T017)
  nvsBoot.incrementar();
  DiagData dd;
  dd.bootCount  = nvsBoot.lerContador();
  dd.freeHeap   = esp_get_free_heap_size();
  dd.mpuOk      = true;  // Fatal se falhou — nao chegaria aqui
  dd.ds18b20Ok  = _ds18b20Ok;
  dd.calOk      = calData.calibrado;
  dd.horOk      = (horData.horimetro >= 0.0f);
  dd.cfgOk      = true;  // Sempre carrega com defaults
  dd.basOk      = basData.basAtivo;
  dd.tasksOk    = true;  // Fatal se falhou — nao chegaria aqui
  diag.imprimir(dd);

  inicioSistemaMs = millis();
  ui.imprimirMensagem("INFO", "Monitoramento iniciado.");
}

// =============================================================
//  LOOP
// =============================================================
void loop() {
  esp_task_wdt_reset();

  CommandContext cmdCtx = {
    .nvsHor = nvsHor,       .horData = horData,
    .nvsBas = nvsBas,       .basData = basData,
    .nvsCal = nvsCal,       .calData = calData,
    .nvsCfg = nvsCfg,       .cfgData = cfgData,
    .driverVib = driverVib, .srvCal  = srvCal,
    .alarmManager = alarmManager,
    .ui = ui,               .i2cMutex = i2cMutex,
    .inicioSistemaMs = inicioSistemaMs
  };

  cmdHandler.processar(cmdCtx);
}
