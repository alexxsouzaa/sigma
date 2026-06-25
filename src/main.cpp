// =============================================================================
//  Project SIGMA - Sistema de Monitoramento Preditivo de Motor Industrial
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Sensores   : DS18B20 (temperatura) + MPU6050 (vibração)
//  Protocolo  : Serial 115200 baud (MQTT será adicionado futuramente)
// =============================================================================
//
// -----------------------------------------------------------------------------
//  HISTÓRICO DE VERSÕES (COMMITS)
// -----------------------------------------------------------------------------
//
//  [v0.1.0] - 2026-06-25 - COMMIT SNAPSHOT (baseline)
//  Autor  : Bruno Alex Souza da Silva
//  Status : Estável — salvo antes da adição de calibração do MPU6050
//  Resumo :
//    - Leitura de temperatura via DS18B20 (1-Wire, GPIO4, pull-up 4.7kΩ)
//    - Leitura de vibração via MPU6050 (I2C 400kHz, SDA=GPIO8, SCL=GPIO9)
//    - Pull-ups externos 4.7kΩ no SDA e SCL
//    - Cálculo de Health Score ponderado (Temp 40% + Vib 40% + Horas 20%)
//    - Horímetro por software (millis)
//    - Alarme em 3 níveis: NORMAL / AVISO / CRITICO
//    - Retry x5 na inicialização do MPU6050 com checklist de diagnóstico
//    - Relatório no Serial Monitor a cada 500 ms
//    - Sem MQTT, sem display, sem LEDs
//
//  [v0.2.0] - 2026-06-25 - EM DESENVOLVIMENTO
//  Resumo :
//    - Adição de rotina de calibração de ponto zero do MPU6050
//    - Offset calculado por média de 200 amostras em repouso no boot
//    - Offsets aplicados em todas as leituras subsequentes
//    - Opção de recalibrar via Serial Monitor enviando o comando 'C'
// -----------------------------------------------------------------------------

// -------------------------
//  Bibliotecas necessárias
// -------------------------
#include <Arduino.h>
#include <Wire.h>                   // Comunicação I2C (MPU6050)
#include <OneWire.h>                // Protocolo 1-Wire (DS18B20)
#include <DallasTemperature.h>      // Biblioteca do sensor DS18B20
#include <Adafruit_MPU6050.h>       // Biblioteca do acelerômetro/giroscópio MPU6050
#include <Adafruit_Sensor.h>        // Abstração de sensor da Adafruit (dependência)

// -------------------------
//  Definição de pinos
// -------------------------
#define PINO_ONE_WIRE   4   // Pino de dados do DS18B20 (com resistor pull-up 4.7kΩ)
#define PINO_MPU_SDA    8   // Pino SDA do barramento I2C
#define PINO_MPU_SCL    9   // Pino SCL do barramento I2C
#define PINO_MPU_INT   14   // Pino de interrupção do MPU6050 (não usado ativamente)

// -------------------------
//  Configurações do sistema
// -------------------------
#define INTERVALO_LEITURA_MS     500    // Intervalo entre leituras (milissegundos)
#define INTERVALO_MANUTENCAO_H   500.0  // Horas entre manutenções preventivas
#define TEMP_AVISO_MIN           25.0   // Temperatura mínima para operação normal (°C)
#define TEMP_AVISO_MAX           55.0   // Temperatura máxima antes de gerar aviso (°C)
#define TEMP_CRITICA             65.0   // Temperatura crítica - alarme máximo (°C)
#define VIB_AVISO                2.0    // Vibração em g para gerar aviso
#define VIB_CRITICA              4.0    // Vibração em g para estado crítico

// -------------------------
//  Instâncias dos sensores
// -------------------------

// Forward declarations
float calcularVibracaoRMS();
float calcularHealthScore(float temp, float vib, float horas);
String classificarAlarme(float temp, float vib);
String obterClassificacaoHealth(float score);
void imprimirRelatorio();
void calibrarPontoZero();

OneWire         barramento1Wire(PINO_ONE_WIRE); // Barramento 1-Wire no pino definido
DallasTemperature sensorTemp(&barramento1Wire); // Sensor DS18B20 no barramento
Adafruit_MPU6050  mpu;                          // Objeto do MPU6050

// -------------------------
//  Variáveis de controle
// -------------------------
unsigned long ultimaLeitura    = 0;   // Timestamp da última leitura (ms)
unsigned long inicioSistema    = 0;   // Timestamp do início do sistema (ms)
float         temperatura      = 0.0; // Temperatura lida em °C
float         vibracaoRMS      = 0.0; // Vibração resultante em g
float         horimetro        = 0.0; // Horas de operação acumuladas
float         healthScore      = 0.0; // Pontuação de saúde do motor (0–100%)
String        statusAlarme     = "";  // Status do alarme: NORMAL / AVISO / CRITICO

// -------------------------
//  Offsets de calibração do MPU6050
//  Calculados durante a rotina de ponto zero no boot.
//  Subtraídos de cada leitura para compensar o erro estático do sensor.
// -------------------------
float offsetAX = 0.0;  // Offset do eixo X do acelerômetro (em g)
float offsetAY = 0.0;  // Offset do eixo Y do acelerômetro (em g)
float offsetAZ = 0.0;  // Offset do eixo Z — após remover 1g da gravidade
bool  calibrado = false; // Flag: indica se a calibração já foi realizada

// =============================================================================
//  FUNÇÃO: calibrarPontoZero
//
//  Realiza a calibração estática do MPU6050 coletando 200 amostras em repouso.
//  O sensor DEVE estar parado e nivelado durante esta operação.
//
//  O que faz:
//    1. Coleta NUM_AMOSTRAS_CAL leituras brutas do acelerômetro
//    2. Calcula a média de cada eixo (X, Y, Z)
//    3. No eixo Z, subtrai 1g (gravidade) — o residual é o erro do sensor
//    4. Armazena os offsets nas variáveis globais offsetAX, offsetAY, offsetAZ
//    5. Define a flag 'calibrado = true'
//
//  Pode ser chamada novamente via Serial Monitor enviando o caractere 'C'.
// =============================================================================
void calibrarPontoZero() {
  const int NUM_AMOSTRAS_CAL = 200; // Número de amostras para calcular o offset

  Serial.println(F(""));
  Serial.println(F("  ==============================================="));
  Serial.println(F("  [CAL] INICIANDO CALIBRACAO DE PONTO ZERO"));
  Serial.println(F("  [CAL] Mantenha o sensor PARADO e NIVELADO!"));
  Serial.println(F("  [CAL] Coletando amostras..."));

  float somaX = 0.0; // Acumulador do eixo X
  float somaY = 0.0; // Acumulador do eixo Y
  float somaZ = 0.0; // Acumulador do eixo Z

  sensors_event_t accel, gyro, temp; // Estruturas de evento do sensor

  for (int i = 0; i < NUM_AMOSTRAS_CAL; i++) {
    mpu.getEvent(&accel, &gyro, &temp); // Lê os três eixos do acelerômetro

    // Converte de m/s² para g e acumula cada eixo
    somaX += accel.acceleration.x / 9.81;
    somaY += accel.acceleration.y / 9.81;
    somaZ += accel.acceleration.z / 9.81;

    delay(5); // Pequena pausa entre amostras para evitar leituras duplicadas

    // Imprime progresso a cada 50 amostras
    if ((i + 1) % 50 == 0) {
      Serial.print(F("  [CAL] "));
      Serial.print(i + 1);
      Serial.print(F(" / "));
      Serial.print(NUM_AMOSTRAS_CAL);
      Serial.println(F(" amostras coletadas..."));
    }
  }

  // Calcula a média de cada eixo
  offsetAX = somaX / NUM_AMOSTRAS_CAL;
  offsetAY = somaY / NUM_AMOSTRAS_CAL;
  // No eixo Z: o sensor deve ler +1g em repouso (gravidade).
  // O offset é o que sobrar além de 1g — ou seja, o erro estático do sensor.
  offsetAZ = (somaZ / NUM_AMOSTRAS_CAL) - 1.0;

  calibrado = true; // Marca calibração como concluída

  // Exibe resultado da calibração no Serial Monitor
  Serial.println(F("  [CAL] Calibracao concluida!"));
  Serial.println(F("  [CAL] ------- Offsets Calculados -------"));
  Serial.print(F("  [CAL]   Offset AX : "));
  Serial.print(offsetAX, 5);
  Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AY : "));
  Serial.print(offsetAY, 5);
  Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AZ : "));
  Serial.print(offsetAZ, 5);
  Serial.println(F(" g"));
  Serial.println(F("  [CAL] Offsets aplicados em todas as leituras."));
  Serial.println(F("  [CAL] Para recalibrar, envie o caractere 'C' no Serial."));
  Serial.println(F("  ==============================================="));
  Serial.println();
}

// =============================================================================
//  FUNÇÃO: calcularVibracaoRMS
//  Lê 50 amostras do acelerômetro MPU6050 e calcula a magnitude RMS resultante.
//  Aplica os offsets de calibração em cada eixo antes do cálculo.
//  Subtrai 1g do eixo Z para remover o efeito da gravidade estática.
//  Retorna: vibração em unidades g (força gravitacional)
// =============================================================================
float calcularVibracaoRMS() {
  const int   NUM_AMOSTRAS = 50;      // Quantidade de amostras para média
  float       somaQuadrados = 0.0;    // Acumulador dos quadrados das magnitudes
  sensors_event_t accel, gyro, temp;  // Estruturas de evento do sensor

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.getEvent(&accel, &gyro, &temp); // Lê todos os eixos do MPU6050

    // Converte de m/s² para g
    float ax = accel.acceleration.x / 9.81;
    float ay = accel.acceleration.y / 9.81;
    float az = accel.acceleration.z / 9.81;

    // Aplica offsets de calibração — remove o erro estático de cada eixo
    // (offsetAX/AY/AZ são zero até que calibrarPontoZero() seja chamada)
    ax = ax - offsetAX;
    ay = ay - offsetAY;
    az = az - offsetAZ - 1.0; // Remove também a gravidade estática do eixo Z

    // Acumula o quadrado da magnitude resultante dos 3 eixos
    somaQuadrados += (ax * ax) + (ay * ay) + (az * az);
  }

  // Retorna a raiz quadrada da média dos quadrados (RMS)
  return sqrt(somaQuadrados / NUM_AMOSTRAS);
}

// =============================================================================
//  FUNÇÃO: calcularHealthScore
//  Calcula a pontuação de saúde do motor de 0 a 100%.
//
//  Pesos:
//    - Temperatura  : 40%
//    - Vibração     : 40%
//    - Horímetro    : 20%
//
//  Retorna: healthScore entre 0.0 e 100.0
// =============================================================================
float calcularHealthScore(float temp, float vib, float horas) {
  // --- Componente de temperatura (0.0 a 1.0) ---
  // Abaixo de TEMP_AVISO_MAX → saúde plena
  // Acima de TEMP_CRITICA    → saúde zero neste componente
  float scoreTemp;
  if (temp <= TEMP_AVISO_MAX) {
    scoreTemp = 1.0; // Temperatura dentro do range normal
  } else if (temp >= TEMP_CRITICA) {
    scoreTemp = 0.0; // Temperatura crítica - componente zerado
  } else {
    // Degradação linear entre TEMP_AVISO_MAX e TEMP_CRITICA
    scoreTemp = 1.0 - ((temp - TEMP_AVISO_MAX) / (TEMP_CRITICA - TEMP_AVISO_MAX));
  }

  // --- Componente de vibração (0.0 a 1.0) ---
  // Abaixo de VIB_AVISO  → saúde plena
  // Acima de VIB_CRITICA → saúde zero neste componente
  float scoreVib;
  if (vib <= VIB_AVISO) {
    scoreVib = 1.0; // Vibração dentro do range normal
  } else if (vib >= VIB_CRITICA) {
    scoreVib = 0.0; // Vibração crítica - componente zerado
  } else {
    // Degradação linear entre VIB_AVISO e VIB_CRITICA
    scoreVib = 1.0 - ((vib - VIB_AVISO) / (VIB_CRITICA - VIB_AVISO));
  }

  // --- Componente de horímetro (0.0 a 1.0) ---
  // Degrada nos últimos 20% do intervalo de manutenção
  float limiteDegrade = INTERVALO_MANUTENCAO_H * 0.80; // 80% do intervalo = 400h
  float scoreHoras;
  if (horas <= limiteDegrade) {
    scoreHoras = 1.0; // Dentro do período seguro
  } else if (horas >= INTERVALO_MANUTENCAO_H) {
    scoreHoras = 0.0; // Manutenção vencida
  } else {
    // Degradação linear nos últimos 20% do intervalo
    scoreHoras = 1.0 - ((horas - limiteDegrade) / (INTERVALO_MANUTENCAO_H - limiteDegrade));
  }

  // --- Score final ponderado (0 a 100%) ---
  // Temperatura: 40% | Vibração: 40% | Horímetro: 20%
  return (scoreTemp * 40.0) + (scoreVib * 40.0) + (scoreHoras * 20.0);
}

// =============================================================================
//  FUNÇÃO: classificarAlarme
//  Determina o nível de alarme com base nos valores medidos.
//
//  Prioridade: CRITICO > AVISO > NORMAL
//  Retorna: String com o nível de alarme
// =============================================================================
String classificarAlarme(float temp, float vib) {
  // Verifica condições críticas (qualquer sensor em estado crítico)
  if (temp >= TEMP_CRITICA || vib >= VIB_CRITICA) {
    return "*** CRITICO ***";
  }
  // Verifica condições de aviso (qualquer sensor acima do limite normal)
  if (temp >= TEMP_AVISO_MAX || vib >= VIB_AVISO) {
    return "    AVISO    ";
  }
  // Operação normal
  return "    NORMAL   ";
}

// =============================================================================
//  FUNÇÃO: obterClassificacaoHealth
//  Retorna uma string descritiva para o health score calculado.
// =============================================================================
String obterClassificacaoHealth(float score) {
  if (score >= 90.0) return "Excelente";
  if (score >= 70.0) return "Bom";
  if (score >= 50.0) return "Manutencao Recomendada";
  return "CONDICAO CRITICA";
}

// =============================================================================
//  FUNÇÃO: imprimirRelatorio
//  Formata e imprime no Monitor Serial o relatório completo de status do motor.
// =============================================================================
void imprimirRelatorio() {
  float horasRestantes = INTERVALO_MANUTENCAO_H - horimetro;
  if (horasRestantes < 0) horasRestantes = 0; // Nunca exibir valor negativo

  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Timestamp       : "));
  Serial.print(millis());
  Serial.println(F(" ms"));
  Serial.println(F("------------------------------------------------"));

  // --- Temperatura ---
  Serial.print(F("  Temperatura     : "));
  Serial.print(temperatura, 1);
  Serial.println(F(" oC"));

  // --- Vibração ---
  Serial.print(F("  Vibracao        : "));
  Serial.print(vibracaoRMS, 3);
  Serial.println(F(" g"));

  // --- Horímetro ---
  Serial.print(F("  Horimetro       : "));
  Serial.print(horimetro, 4);
  Serial.println(F(" h"));

  // --- Próxima manutenção ---
  Serial.print(F("  Prox. Manut.    : "));
  Serial.print(horasRestantes, 1);
  Serial.println(F(" h restantes"));

  // --- Health Score ---
  Serial.print(F("  Health Score    : "));
  Serial.print(healthScore, 1);
  Serial.print(F(" %  ("));
  Serial.print(obterClassificacaoHealth(healthScore));
  Serial.println(F(")"));

  // --- Alarme ---
  Serial.print(F("  Alarme          : "));
  Serial.println(statusAlarme);

  Serial.println(F("------------------------------------------------"));
  Serial.println(); // Linha em branco para separar leituras
}

// =============================================================================
//  SETUP - Executado uma única vez na inicialização
// =============================================================================
void setup() {
  // Inicia comunicação serial para debug e monitoramento
  Serial.begin(115200);
  delay(500); // Aguarda estabilização da porta serial

  Serial.println(F("=============================================="));
  Serial.println(F("  Project SIGMA - Iniciando Sistema...       "));
  Serial.println(F("=============================================="));

  // ------------------------------------------
  //  Inicialização do barramento I2C (MPU6050)
  // ------------------------------------------
  // Inicia I2C com pinos definidos e clock explícito de 400 kHz (Fast Mode)
  // IMPORTANTE: sem Wire.setClock() a biblioteca Adafruit pode falhar no begin()
  Wire.begin(PINO_MPU_SDA, PINO_MPU_SCL);
  Wire.setClock(400000); // 400 kHz - Fast Mode I2C
  delay(150); // Aguarda MPU6050 completar boot interno (mínimo 100 ms recomendado)
  Serial.println(F("  [I2C] Barramento I2C iniciado (400 kHz Fast Mode)."));

  // Tenta conectar ao MPU6050 no endereço padrão 0x68
  // Realiza até 5 tentativas com intervalo de 200 ms entre elas
  // (o sensor pode demorar a responder logo após o power-on)
  bool mpuOk = false;
  for (int tentativa = 1; tentativa <= 5; tentativa++) {
    Serial.print(F("  [I2C] Tentativa "));
    Serial.print(tentativa);
    Serial.print(F(" de 5 - procurando MPU6050 (0x68)..."));
    if (mpu.begin()) {
      mpuOk = true;
      Serial.println(F(" ENCONTRADO!"));
      break;
    }
    Serial.println(F(" nao respondeu."));
    delay(200); // Pausa antes da próxima tentativa
  }

  if (!mpuOk) {
    // Todas as tentativas falharam - exibe checklist de diagnóstico
    Serial.println(F(""));
    Serial.println(F("  [ERRO FATAL] MPU6050 nao encontrado!"));
    Serial.println(F("  Checklist de diagnostico:"));
    Serial.println(F("    1. Pull-up 4.7k no SDA (GPIO8) ligado ao 3.3V?"));
    Serial.println(F("    2. Pull-up 4.7k no SCL (GPIO9) ligado ao 3.3V?"));
    Serial.println(F("    3. VCC do MPU6050 ligado ao 3.3V?"));
    Serial.println(F("    4. GND do MPU6050 ligado ao GND?"));
    Serial.println(F("    5. Pino AD0 no GND = endereco 0x68 (padrao)"));
    Serial.println(F("    5. Pino AD0 no 3.3V = endereco 0x69"));
    Serial.println(F("  Sistema pausado. Corrija a fiacao e reinicie."));
    while (true) {
      delay(1000); // Trava o sistema - aguarda intervenção do usuário
    }
  }
  Serial.println(F("  [OK] MPU6050 inicializado com sucesso."));

  // Configura escala do acelerômetro para ±8g (adequado para vibração industrial)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println(F("  [MPU] Escala do acelerometro: +/- 8g"));

  // Configura escala do giroscópio para ±500°/s
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.println(F("  [MPU] Escala do giroscopio: +/- 500 deg/s"));

  // Configura filtro passa-baixa para reduzir ruído (banda de 21 Hz)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println(F("  [MPU] Filtro passa-baixa: 21 Hz"));

  // ------------------------------------------
  //  Calibração de ponto zero do MPU6050
  //  Executada automaticamente após a configuração do sensor.
  //  O sensor deve estar em repouso neste momento.
  // ------------------------------------------
  calibrarPontoZero();

  // ------------------------------------------
  //  Inicialização do DS18B20 (temperatura)
  // ------------------------------------------
  sensorTemp.begin(); // Inicia o barramento 1-Wire e detecta sensores
  int qtdSensores = sensorTemp.getDeviceCount();
  Serial.print(F("  [DS18B20] Sensores encontrados: "));
  Serial.println(qtdSensores);

  if (qtdSensores == 0) {
    // Nenhum sensor de temperatura encontrado - avisa mas continua
    Serial.println(F("  [AVISO] Nenhum DS18B20 detectado! Verifique pull-up e fiacao."));
  } else {
    // Configura resolução máxima de 12 bits (precisão de 0.0625°C)
    sensorTemp.setResolution(12);
    Serial.println(F("  [DS18B20] Resolucao configurada: 12 bits (0.0625 oC)"));
  }

  // Salva o momento de início para calcular horímetro
  inicioSistema = millis();
  ultimaLeitura = millis();

  Serial.println(F("=============================================="));
  Serial.println(F("  Sistema pronto. Iniciando monitoramento..."));
  Serial.println(F("  Dica: envie 'C' no Serial para recalibrar MPU"));
  Serial.println(F("=============================================="));
  Serial.println();
}

// =============================================================================
//  LOOP — Executado continuamente
// =============================================================================
void loop() {
  unsigned long agora = millis(); // Captura tempo atual em ms

  // ------------------------------------------
  //  Verifica comando de recalibração via Serial
  //  Envie o caractere 'C' (maiúsculo) no Monitor Serial
  //  para iniciar uma nova calibração de ponto zero.
  //  O motor deve estar PARADO durante a recalibração.
  // ------------------------------------------
  if (Serial.available() > 0) {
    char cmd = Serial.read(); // Lê o caractere enviado
    if (cmd == 'C' || cmd == 'c') {
      Serial.println(F("  [CMD] Comando de recalibracao recebido!"));
      Serial.println(F("  [CMD] ATENCAO: Pare o motor antes de continuar."));
      delay(2000); // Pausa de 2 segundos para o operador parar o motor
      calibrarPontoZero(); // Executa nova calibração
    }
  }

  // Verifica se o intervalo de leitura foi atingido
  if (agora - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora; // Atualiza timestamp da última leitura

    // ------------------------------------------
    //  1. Atualiza horímetro de operação
    // ------------------------------------------
    // Converte milissegundos de operação para horas (1h = 3.600.000 ms)
    horimetro = (float)(agora - inicioSistema) / 3600000.0;

    // ------------------------------------------
    //  2. Leitura de temperatura (DS18B20)
    // ------------------------------------------
    sensorTemp.requestTemperatures(); // Solicita conversão de temperatura
    float leituraTemp = sensorTemp.getTempCByIndex(0); // Lê primeiro sensor em °C

    // Verifica se a leitura é válida (-127 indica erro de comunicação)
    if (leituraTemp != DEVICE_DISCONNECTED_C) {
      temperatura = leituraTemp;
    } else {
      // Mantém último valor válido e notifica via serial
      Serial.println(F("  [AVISO] Falha na leitura do DS18B20!"));
    }

    // ------------------------------------------
    //  3. Leitura de vibração (MPU6050)
    // ------------------------------------------
    vibracaoRMS = calcularVibracaoRMS(); // Calcula RMS de 50 amostras

    // ------------------------------------------
    //  4. Cálculo do Health Score e Alarme
    // ------------------------------------------
    healthScore  = calcularHealthScore(temperatura, vibracaoRMS, horimetro);
    statusAlarme = classificarAlarme(temperatura, vibracaoRMS);

    // ------------------------------------------
    //  5. Impressão do relatório no Serial Monitor
    // ------------------------------------------
    imprimirRelatorio();
  }
}