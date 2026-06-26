// =============================================================================
//  Project SIGMA - Sistema de Monitoramento Preditivo de Motor Industrial
//  Codinome   : Project SIGMA
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Sensores   : DS18B20 (temperatura) + MPU6050 (vibracao)
//  Protocolo  : Serial 115200 baud
//  Versao     : 0.1.2.1
// =============================================================================
//
//  [v0.1.2.1]   - 2026-06-26 - VERSAO ATUAL                         [CURRENT]
//  Resumo :
//    - Boot: calibracao automatica sem confirmacao do usuario
//    - Confirmacao (S/N) mantida APENAS quando usuario envia 'C' manualmente
//    - CORRIGIDO: bloco aguardarConfirmacaoSerial() removido do setup()
//
// -------------------------
//  Bibliotecas necessarias
// -------------------------
#include <Arduino.h>
#include <Wire.h>                   // Comunicacao I2C (MPU6050)
#include <OneWire.h>                // Protocolo 1-Wire (DS18B20)
#include <DallasTemperature.h>      // Biblioteca do sensor DS18B20
#include <Adafruit_MPU6050.h>       // Biblioteca do acelerometro/giroscopio MPU6050
#include <Adafruit_Sensor.h>        // Abstracao de sensor da Adafruit (dependencia)

// -------------------------
//  Definicao de pinos
// -------------------------
#define PINO_ONE_WIRE   4   // Pino de dados do DS18B20 (com resistor pull-up 4.7k)
#define PINO_MPU_SDA    8   // Pino SDA do barramento I2C
#define PINO_MPU_SCL    9   // Pino SCL do barramento I2C
#define PINO_MPU_INT   14   // Pino de interrupcao do MPU6050 (nao usado ativamente)

// -------------------------
//  Configuracoes do sistema
// -------------------------
#define INTERVALO_LEITURA_MS     500    // Intervalo entre leituras (milissegundos)
#define INTERVALO_MANUTENCAO_H   500.0  // Horas entre manutencoes preventivas
#define TEMP_AVISO_MIN           25.0   // Temperatura minima para operacao normal (oC)
#define TEMP_AVISO_MAX           55.0   // Temperatura maxima antes de gerar aviso (oC)
#define TEMP_CRITICA             65.0   // Temperatura critica - alarme maximo (oC)

// VIB_AVISO e VIB_CRITICA sao variaveis globais (nao #define)
// para permitir ajuste em runtime via menu de sensibilidade (comando 'A')
// Valores padrao para escala +/-8g (uso geral industrial)
float VIB_AVISO   = 2.0;   // Limiar de aviso em g — ajustavel via Serial
float VIB_CRITICA = 4.0;   // Limiar critico em g — ajustavel via Serial

// -------------------------
//  Forward declarations
// -------------------------
float  calcularVibracaoRMS();
float  calcularHealthScore(float temp, float vib, float horas);
String classificarAlarme(float temp, float vib);
String obterClassificacaoHealth(float score);
void   imprimirRelatorio();
bool   calibrarPontoZero();
bool   aguardarConfirmacaoSerial(uint32_t timeoutMs);
bool   confirmarSaidaCalibracao();
void   menuPosCalibracao();
void   menuSensibilidade();

// -------------------------
//  Instancias dos sensores
// -------------------------
OneWire           barramento1Wire(PINO_ONE_WIRE); // Barramento 1-Wire no pino definido
DallasTemperature sensorTemp(&barramento1Wire);   // Sensor DS18B20 no barramento
Adafruit_MPU6050  mpu;                            // Objeto do MPU6050

// -------------------------
//  Variaveis de controle
// -------------------------
unsigned long ultimaLeitura  = 0;     // Timestamp da ultima leitura (ms)
unsigned long inicioSistema  = 0;     // Timestamp do inicio do sistema (ms)
float         temperatura    = 0.0;   // Temperatura lida em oC
float         vibracaoRMS    = 0.0;   // Vibracao resultante em g
float         horimetro      = 0.0;   // Horas de operacao acumuladas
float         healthScore    = 0.0;   // Pontuacao de saude do motor (0-100%)
String        statusAlarme   = "";    // Status: NORMAL / AVISO / CRITICO

// -------------------------
//  Offsets de calibracao do MPU6050
//  Calculados durante a rotina de ponto zero.
//  Subtraidos de cada leitura para compensar o erro estatico do sensor.
// -------------------------
float offsetAX     = 0.0;   // Offset do eixo X do acelerometro (em g)
float offsetAY     = 0.0;   // Offset do eixo Y do acelerometro (em g)
float offsetAZ     = 0.0;   // Offset do eixo Z — valor bruto (inclui ~1g de gravidade)
bool  calibrado    = false;  // Flag: indica se a calibracao ja foi realizada
bool  emCalibracao = false;  // Flag: bloqueia o loop de monitoramento durante calibracao

// -------------------------
//  Configuracao de escala do acelerometro
//  Valores validos: 2, 4, 8, 16 (em g)
//  Valor padrao: 8g (uso geral industrial)
// -------------------------
int escalaAtualG = 8; // Escala ativa do acelerometro em g

// =============================================================================
//  FUNCAO: aguardarConfirmacaoSerial
//
//  Exibe prompt interativo e aguarda resposta do usuario via Serial.
//  Usada APENAS quando o usuario inicia a calibracao manualmente (comando 'C').
//  NAO e chamada no boot — o boot calibra automaticamente sem confirmacao.
//
//  Parametros:
//    timeoutMs : tempo maximo de espera em milissegundos
//
//  Retorna:
//    true  — usuario confirmou com 'S' ou 's'
//    false — usuario cancelou com 'N'/'n' ou timeout atingido
// =============================================================================
bool aguardarConfirmacaoSerial(uint32_t timeoutMs) {
  // Limpa qualquer dado residual no buffer serial antes de aguardar
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |          CONFIRMACAO NECESSARIA                  |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Deseja iniciar a calibracao de ponto zero?      |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  >> IMPORTANTE: O sensor deve estar PARADO       |"));
  Serial.println(F("  |     e NIVELADO antes de confirmar!               |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Digite  S  para CONFIRMAR calibracao            |"));
  Serial.println(F("  |  Digite  N  para CANCELAR e voltar ao monitor    |"));
  Serial.println(F("  +--------------------------------------------------+"));

  uint32_t inicio            = millis();
  uint32_t segundosRestantes = timeoutMs / 1000;
  uint32_t ultimoSegundo     = millis();

  Serial.print(F("  Aguardando resposta... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  while ((millis() - inicio) < timeoutMs) {

    // Atualiza contador regressivo a cada segundo
    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando resposta... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }

    if (Serial.available() > 0) {
      char resposta = Serial.read();

      if (resposta == 'S' || resposta == 's') {
        Serial.println(F(""));
        Serial.println(F("  [OK] Calibracao CONFIRMADA pelo usuario."));
        return true;
      }

      if (resposta == 'N' || resposta == 'n') {
        Serial.println(F(""));
        Serial.println(F("  [INFO] Calibracao CANCELADA pelo usuario."));
        Serial.println(F("  [INFO] Retornando ao monitoramento normal..."));
        return false;
      }
      // Qualquer outro caractere e ignorado — aguarda S ou N
    }

    delay(50);
  }

  // Timeout atingido — cancela automaticamente por seguranca
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Nenhuma resposta recebida."));
  Serial.println(F("  [TIMEOUT] Calibracao CANCELADA automaticamente."));
  Serial.println(F("  [INFO] Retornando ao monitoramento normal..."));
  return false;
}

// =============================================================================
//  FUNCAO: confirmarSaidaCalibracao
//
//  Exibida quando o usuario envia 'X' durante a coleta de amostras.
//  Pergunta se realmente quer abortar a calibracao em andamento.
//
//  Retorna:
//    true  — usuario confirmou saida (S) — calibracao sera abortada
//    false — usuario cancelou saida (N) — calibracao continua normalmente
// =============================================================================
bool confirmarSaidaCalibracao() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +--------------------------------------------------+"));
  Serial.println(F("  |         CONFIRMAR SAIDA DA CALIBRACAO?           |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  As amostras coletadas ate agora serao           |"));
  Serial.println(F("  |  DESCARTADAS e os offsets NAO serao alterados.   |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  A calibracao anterior (se existir) e mantida.   |"));
  Serial.println(F("  |                                                  |"));
  Serial.println(F("  |  Digite  S  para SAIR da calibracao              |"));
  Serial.println(F("  |  Digite  N  para CONTINUAR coletando amostras    |"));
  Serial.println(F("  +--------------------------------------------------+"));

  // Timeout de 60s: se nao responder, CONTINUA a coleta (comportamento seguro)
  const uint32_t TIMEOUT_SAIDA_MS = 60000;
  uint32_t inicioEspera           = millis();
  uint32_t segundosRestantes      = TIMEOUT_SAIDA_MS / 1000;
  uint32_t ultimoSegundo          = millis();

  Serial.print(F("  Aguardando resposta... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  while ((millis() - inicioEspera) < TIMEOUT_SAIDA_MS) {

    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando resposta... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }

    if (Serial.available() > 0) {
      char resposta = Serial.read();

      if (resposta == 'S' || resposta == 's') {
        Serial.println(F(""));
        Serial.println(F("  [CAL] Saida CONFIRMADA. Calibracao abortada."));
        Serial.println(F("  [CAL] Amostras parciais descartadas."));
        return true;
      }

      if (resposta == 'N' || resposta == 'n') {
        Serial.println(F(""));
        Serial.println(F("  [CAL] Saida CANCELADA. Retomando coleta de amostras..."));
        return false;
      }
    }
    delay(50);
  }

  // Timeout — comportamento seguro: nao abortar, continua coleta
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 60s. Coleta retomada automaticamente."));
  return false;
}

// =============================================================================
//  FUNCAO: menuPosCalibracao
//
//  Exibida apos a calibracao ser concluida com sucesso.
//  Permite sair ao monitoramento (S) ou recalibrar (R).
//  Timeout de 60s: sai automaticamente ao monitoramento.
// =============================================================================
void menuPosCalibracao() {
  while (Serial.available()) Serial.read();

  Serial.println(F(""));
  Serial.println(F("  +----------------------------------------------------+"));
  Serial.println(F("  |         CALIBRACAO CONCLUIDA COM SUCESSO!          |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  O que deseja fazer agora?                        |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Digite  S  para SAIR e voltar ao monitoramento   |"));
  Serial.println(F("  |  Digite  R  para RECALIBRAR novamente             |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  Dica: recalibre sempre com o sensor em repouso!  |"));
  Serial.println(F("  +----------------------------------------------------+"));

  const uint32_t TIMEOUT_MENU_MS = 60000;
  uint32_t inicioMenu            = millis();
  uint32_t segundosRestantes     = TIMEOUT_MENU_MS / 1000;
  uint32_t ultimoSegundo         = millis();

  Serial.print(F("  Aguardando resposta... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  while ((millis() - inicioMenu) < TIMEOUT_MENU_MS) {

    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando resposta... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r"));
    }

    if (Serial.available() > 0) {
      char resposta = Serial.read();

      if (resposta == 'S' || resposta == 's') {
        Serial.println(F(""));
        Serial.println(F("  [MENU] Saindo do menu de calibracao."));
        Serial.println(F("  [MENU] Monitoramento retomado com os novos offsets."));
        Serial.println();
        emCalibracao = false; // Libera o loop de monitoramento
        return;
      }

      if (resposta == 'R' || resposta == 'r') {
        Serial.println(F(""));
        Serial.println(F("  [MENU] Iniciando nova calibracao..."));
        Serial.println(F("  [MENU] Certifique-se que o sensor esta PARADO e NIVELADO!"));
        delay(1000);
        // Recalibra diretamente — intencao ja demonstrada pelo R
        // calibrarPontoZero() chama menuPosCalibracao() novamente se concluir
        calibrarPontoZero();
        return;
      }
    }
    delay(50);
  }

  // Timeout — sai automaticamente com offsets aplicados
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 60s. Retornando ao monitoramento."));
  Serial.println();
  emCalibracao = false;
}

// =============================================================================
//  FUNCAO: calibrarPontoZero
//
//  Realiza a calibracao estatica do MPU6050 coletando 200 amostras em repouso.
//  O sensor DEVE estar parado e nivelado durante esta operacao.
//
//  Fluxo:
//    1. Coleta 200 amostras brutas do acelerometro
//    2. Verifica a cada amostra se o usuario enviou 'X' para sair
//    3. Se 'X' recebido: chama confirmarSaidaCalibracao() (S/N)
//    4. Se confirmar saida: descarta acumuladores, retorna false
//    5. Se concluida: calcula offsets, aplica globalmente, retorna true
//
//  Retorna:
//    true  — calibracao concluida, offsets atualizados
//    false — calibracao abortada, offsets NAO alterados
// =============================================================================
bool calibrarPontoZero() {
  const int NUM_AMOSTRAS_CAL = 200; // Numero de amostras para calcular o offset

  Serial.println(F(""));
  Serial.println(F("  ==============================================="));
  Serial.println(F("  [CAL] INICIANDO CALIBRACAO DE PONTO ZERO"));
  Serial.println(F("  [CAL] Mantenha o sensor PARADO e NIVELADO!"));
  Serial.println(F("  [CAL] Dica: envie 'X' a qualquer momento para sair."));
  Serial.println(F("  [CAL] Coletando amostras..."));

  // Acumuladores temporarios — so transferidos para offsets globais
  // se a calibracao for CONCLUIDA. Se abortada, offsets anteriores sao mantidos.
  float somaX = 0.0;
  float somaY = 0.0;
  float somaZ = 0.0;
  int   amostrasColetadas = 0;

  sensors_event_t accel, gyro, temp;

  for (int i = 0; i < NUM_AMOSTRAS_CAL; i++) {
    mpu.getEvent(&accel, &gyro, &temp);

    // Converte de m/s² para g e acumula cada eixo
    somaX += accel.acceleration.x / 9.81;
    somaY += accel.acceleration.y / 9.81;
    somaZ += accel.acceleration.z / 9.81;
    amostrasColetadas++;

    delay(5); // Pausa entre amostras — evita leituras duplicadas em cache

    // Exibe progresso a cada 50 amostras
    if ((i + 1) % 50 == 0) {
      Serial.print(F("  [CAL] "));
      Serial.print(i + 1);
      Serial.print(F(" / "));
      Serial.print(NUM_AMOSTRAS_CAL);
      Serial.println(F(" amostras coletadas... (envie 'X' para sair)"));
    }

    // Verifica se o usuario enviou 'X' para interromper
    if (Serial.available() > 0) {
      char tecla = Serial.read();

      if (tecla == 'X' || tecla == 'x') {
        Serial.println(F(""));
        Serial.print(F("  [CAL] Comando de saida recebido apos "));
        Serial.print(amostrasColetadas);
        Serial.println(F(" amostras."));

        bool sairConfirmado = confirmarSaidaCalibracao();

        if (sairConfirmado) {
          // Descarta acumuladores — offsets globais NAO sao alterados
          Serial.println(F("  [CAL] Calibracao ABORTADA pelo usuario."));
          Serial.println(F("  [CAL] Offsets anteriores mantidos sem alteracao."));
          if (!calibrado) {
            Serial.println(F("  [CAL] ATENCAO: Nenhuma calibracao ativa. Offsets = 0."));
            Serial.println(F("  [CAL] Leituras podem conter erro estatico do sensor."));
          }
          Serial.println(F("  [CAL] Envie 'C' para tentar calibrar novamente."));
          Serial.println(F("  ==============================================="));
          Serial.println();
          emCalibracao = false;
          return false;
        }
        // Usuario escolheu continuar — o for prossegue normalmente
      }
      // Qualquer outro caractere durante a coleta e ignorado
    }
  }

  // ------------------------------------------------------------------
  //  Calibracao concluida — calcula e aplica os offsets globais
  // ------------------------------------------------------------------
  offsetAX = somaX / NUM_AMOSTRAS_CAL;
  offsetAY = somaY / NUM_AMOSTRAS_CAL;
  // Eixo Z: armazena a media BRUTA (inclui ~1g de gravidade).
  // A subtracao de 1g e feita UMA unica vez dentro de calcularVibracaoRMS().
  // IMPORTANTE: NAO subtrair 1g aqui — evita dupla remocao de gravidade.
  offsetAZ = somaZ / NUM_AMOSTRAS_CAL;

  calibrado = true; // Marca calibracao como concluida

  Serial.println(F("  [CAL] Calibracao concluida com sucesso!"));
  Serial.println(F("  [CAL] ------- Offsets Calculados -------"));
  Serial.print(F("  [CAL]   Offset AX : "));
  Serial.print(offsetAX, 5);
  Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AY : "));
  Serial.print(offsetAY, 5);
  Serial.println(F(" g"));
  Serial.print(F("  [CAL]   Offset AZ : "));
  Serial.print(offsetAZ, 5);
  Serial.println(F(" g  (bruto, inclui ~1g de gravidade)"));
  Serial.println(F("  [CAL]   -> Az corrigido em leitura: offsetAZ - 1g aplicado em calcularVibracaoRMS()"));
  Serial.println(F("  [CAL] Offsets aplicados em todas as leituras."));
  Serial.println(F("  ==============================================="));
  Serial.println();

  // Exibe menu pos-calibracao: usuario escolhe Sair (S) ou Recalibrar (R)
  // A funcao gerencia o flag emCalibracao internamente
  menuPosCalibracao();

  return true;
}

// =============================================================================
//  FUNCAO: menuSensibilidade
//
//  Menu interativo de ajuste de sensibilidade do acelerometro MPU6050.
//  Acessado via comando 'A' no Serial Monitor durante o monitoramento.
//
//  Permite:
//    1. Selecionar a escala do acelerometro (2g / 4g / 8g / 16g)
//    2. Ajustar manualmente os limiares VIB_AVISO e VIB_CRITICA
//    3. Aceitar sugestao automatica de limiares para a escala escolhida
//    4. Recalibrar automaticamente o ponto zero apos mudanca de escala
//
//  Guia de escala x aplicacao:
//    +/-  2g  Alta sensibilidade  — motores pequenos, bancada de teste
//    +/-  4g  Sensibilidade media — motores de pequeno porte
//    +/-  8g  Uso geral industrial — motores de medio porte   [PADRAO]
//    +/- 16g  Baixa sensibilidade — motores pesados, compressores
// =============================================================================
void menuSensibilidade() {
  // Pausa o loop de monitoramento durante o menu
  emCalibracao = true;

  while (Serial.available()) Serial.read();

  // Exibe estado atual e menu de selecao de escala
  Serial.println(F(""));
  Serial.println(F("  +====================================================+"));
  Serial.println(F("  |       MENU DE SENSIBILIDADE DO ACELEROMETRO        |"));
  Serial.println(F("  +====================================================+"));
  Serial.print(F(  "  |  Escala atual     : +/- "));
  Serial.print(escalaAtualG);
  Serial.println(F(" g                           |"));
  Serial.print(F(  "  |  VIB_AVISO atual  : "));
  Serial.print(VIB_AVISO, 2);
  Serial.println(F(" g                               |"));
  Serial.print(F(  "  |  VIB_CRITICA atual: "));
  Serial.print(VIB_CRITICA, 2);
  Serial.println(F(" g                               |"));
  Serial.println(F("  |----------------------------------------------------|"));
  Serial.println(F("  |  Selecione a escala do acelerometro:               |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  [1]  +/-  2g  Alta sens.  — bancada / lab        |"));
  Serial.println(F("  |  [2]  +/-  4g  Media sens. — motores pequenos     |"));
  Serial.println(F("  |  [3]  +/-  8g  Uso geral   — industrial medio     |"));
  Serial.println(F("  |  [4]  +/- 16g  Baixa sens. — motores pesados      |"));
  Serial.println(F("  |                                                    |"));
  Serial.println(F("  |  [M]  Ajuste MANUAL dos limiares de alarme        |"));
  Serial.println(F("  |  [S]  SAIR sem alterar configuracao               |"));
  Serial.println(F("  +====================================================+"));

  const uint32_t TIMEOUT_MENU_MS = 60000;
  uint32_t inicioMenu        = millis();
  uint32_t segundosRestantes = TIMEOUT_MENU_MS / 1000;
  uint32_t ultimoSegundo     = millis();
  bool     menuAtivo         = true;

  Serial.print(F("  Aguardando selecao... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  while (menuAtivo && (millis() - inicioMenu) < TIMEOUT_MENU_MS) {

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

      // [S] Sair sem alterar nada
      if (opcao == 'S' || opcao == 's') {
        Serial.println(F("  [SENS] Configuracao mantida. Saindo do menu..."));
        menuAtivo = false;
        break;
      }

      // [1-4] Selecao de escala do acelerometro
      mpu6050_accel_range_t novaEscalaEnum;
      int   novaEscalaG     = 0;
      float sugestaoAviso   = 0.0;
      float sugestaoCritica = 0.0;
      bool  escalaValida    = true;

      if (opcao == '1') {
        novaEscalaG     = 2;
        novaEscalaEnum  = MPU6050_RANGE_2_G;
        sugestaoAviso   = 0.5;
        sugestaoCritica = 1.0;
      } else if (opcao == '2') {
        novaEscalaG     = 4;
        novaEscalaEnum  = MPU6050_RANGE_4_G;
        sugestaoAviso   = 1.0;
        sugestaoCritica = 2.0;
      } else if (opcao == '3') {
        novaEscalaG     = 8;
        novaEscalaEnum  = MPU6050_RANGE_8_G;
        sugestaoAviso   = 2.0;
        sugestaoCritica = 4.0;
      } else if (opcao == '4') {
        novaEscalaG     = 16;
        novaEscalaEnum  = MPU6050_RANGE_16_G;
        sugestaoAviso   = 4.0;
        sugestaoCritica = 8.0;
      } else if (opcao == 'M' || opcao == 'm') {
        escalaValida = false; // Vai para ajuste manual direto
      } else {
        escalaValida = false;
        Serial.println(F("  [SENS] Opcao invalida. Digite 1, 2, 3, 4, M ou S."));
        continue;
      }

      // Aplica nova escala se selecionada
      if (escalaValida && novaEscalaG > 0) {
        Serial.print(F("  [SENS] Nova escala selecionada: +/- "));
        Serial.print(novaEscalaG);
        Serial.println(F(" g"));
        Serial.println(F(""));
        Serial.println(F("  [SENS] Limiares sugeridos para esta escala:"));
        Serial.print(F(  "  [SENS]   VIB_AVISO   sugerido : "));
        Serial.print(sugestaoAviso, 2);
        Serial.println(F(" g"));
        Serial.print(F(  "  [SENS]   VIB_CRITICA sugerido : "));
        Serial.print(sugestaoCritica, 2);
        Serial.println(F(" g"));
        Serial.println(F(""));
        Serial.println(F("  Deseja aplicar os limiares sugeridos?"));
        Serial.println(F("  [S] Aplicar sugestao automatica"));
        Serial.println(F("  [M] Definir manualmente"));
        Serial.println(F("  [N] Manter limiares atuais"));

        while (Serial.available()) Serial.read();
        uint32_t inicioLimiar = millis();
        bool aguardandoLimiar = true;

        while (aguardandoLimiar && (millis() - inicioLimiar) < 30000) {
          if (Serial.available() > 0) {
            char resLimiar = Serial.read();
            Serial.println(F(""));

            if (resLimiar == 'S' || resLimiar == 's') {
              VIB_AVISO   = sugestaoAviso;
              VIB_CRITICA = sugestaoCritica;
              Serial.println(F("  [SENS] Limiares sugeridos aplicados."));
              aguardandoLimiar = false;

            } else if (resLimiar == 'M' || resLimiar == 'm') {
              Serial.println(F("  [SENS] Ajuste manual selecionado."));
              Serial.print(F(  "  [SENS] VIB_AVISO atual  : "));
              Serial.print(VIB_AVISO, 2);
              Serial.println(F(" g"));
              Serial.println(F("  [SENS] Digite o novo valor de VIB_AVISO (ex: 1.50) e pressione Enter:"));

              while (Serial.available()) Serial.read();
              String inputAviso    = "";
              uint32_t inicioDigit = millis();
              while ((millis() - inicioDigit) < 30000) {
                if (Serial.available() > 0) {
                  char c = Serial.read();
                  if (c == '\n' || c == '\r') break;
                  inputAviso += c;
                }
              }
              float novoAviso = inputAviso.toFloat();

              Serial.print(F(  "  [SENS] VIB_CRITICA atual: "));
              Serial.print(VIB_CRITICA, 2);
              Serial.println(F(" g"));
              Serial.println(F("  [SENS] Digite o novo valor de VIB_CRITICA (ex: 3.00) e pressione Enter:"));

              while (Serial.available()) Serial.read();
              String inputCritica = "";
              inicioDigit = millis();
              while ((millis() - inicioDigit) < 30000) {
                if (Serial.available() > 0) {
                  char c = Serial.read();
                  if (c == '\n' || c == '\r') break;
                  inputCritica += c;
                }
              }
              float novaCritica = inputCritica.toFloat();

              if (novoAviso > 0.0 && novaCritica > 0.0 && novoAviso < novaCritica) {
                VIB_AVISO   = novoAviso;
                VIB_CRITICA = novaCritica;
                Serial.println(F("  [SENS] Limiares manuais aplicados com sucesso."));
              } else {
                Serial.println(F("  [SENS] ERRO: valores invalidos. Limiares NAO alterados."));
                Serial.println(F("  [SENS] Regra: VIB_AVISO > 0 e VIB_CRITICA > VIB_AVISO."));
              }
              aguardandoLimiar = false;

            } else if (resLimiar == 'N' || resLimiar == 'n') {
              Serial.println(F("  [SENS] Limiares mantidos sem alteracao."));
              aguardandoLimiar = false;
            }
          }
          delay(50);
        }

        if (aguardandoLimiar) {
          // Timeout no sub-menu de limiares — aplica sugestao automaticamente
          Serial.println(F("  [TIMEOUT] Sem resposta. Limiares sugeridos aplicados automaticamente."));
          VIB_AVISO   = sugestaoAviso;
          VIB_CRITICA = sugestaoCritica;
        }

        // Aplica a nova escala no hardware MPU6050
        mpu.setAccelerometerRange(novaEscalaEnum);
        escalaAtualG = novaEscalaG;
        Serial.print(F("  [SENS] Escala aplicada no MPU6050: +/- "));
        Serial.print(escalaAtualG);
        Serial.println(F(" g"));

        // Recalibracao obrigatoria apos mudanca de escala.
        // IMPORTANTE: offsets calculados em uma escala sao invalidos em outra.
        Serial.println(F(""));
        Serial.println(F("  [SENS] ATENCAO: Mudanca de escala invalida os offsets anteriores."));
        Serial.println(F("  [SENS] Recalibracao automatica sera iniciada agora."));
        Serial.println(F("  [SENS] Mantenha o sensor PARADO e NIVELADO!"));
        delay(2000);

        calibrado = false; // Invalida calibracao anterior
        offsetAX  = 0.0;   // Zera offsets — evita leituras incorretas
        offsetAY  = 0.0;
        offsetAZ  = 0.0;

        // Executa calibracao sem prompt de confirmacao — intencao ja demonstrada
        calibrarPontoZero();
        // menuPosCalibracao() e chamado dentro de calibrarPontoZero()
        // emCalibracao sera liberado la dentro
        menuAtivo = false;
        return;
      }

      // [M] Ajuste manual direto dos limiares (sem mudar escala)
      if (opcao == 'M' || opcao == 'm') {
        Serial.println(F("  [SENS] Ajuste manual de limiares (escala mantida)."));
        Serial.print(F(  "  [SENS] VIB_AVISO atual  : "));
        Serial.print(VIB_AVISO, 2);
        Serial.println(F(" g"));
        Serial.println(F("  [SENS] Digite o novo valor de VIB_AVISO e pressione Enter:"));

        while (Serial.available()) Serial.read();
        String inputAviso    = "";
        uint32_t inicioDigit = millis();
        while ((millis() - inicioDigit) < 30000) {
          if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') break;
            inputAviso += c;
          }
        }
        float novoAviso = inputAviso.toFloat();

        Serial.print(F(  "  [SENS] VIB_CRITICA atual: "));
        Serial.print(VIB_CRITICA, 2);
        Serial.println(F(" g"));
        Serial.println(F("  [SENS] Digite o novo valor de VIB_CRITICA e pressione Enter:"));

        while (Serial.available()) Serial.read();
        String inputCritica = "";
        inicioDigit = millis();
        while ((millis() - inicioDigit) < 30000) {
          if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') break;
            inputCritica += c;
          }
        }
        float novaCritica = inputCritica.toFloat();

        if (novoAviso > 0.0 && novaCritica > 0.0 && novoAviso < novaCritica) {
          VIB_AVISO   = novoAviso;
          VIB_CRITICA = novaCritica;
          Serial.println(F("  [SENS] Limiares manuais aplicados com sucesso."));
        } else {
          Serial.println(F("  [SENS] ERRO: valores invalidos. Limiares NAO alterados."));
          Serial.println(F("  [SENS] Regra: VIB_AVISO > 0 e VIB_CRITICA > VIB_AVISO."));
        }
        menuAtivo = false;
      }
    }
    delay(50);
  }

  // Timeout do menu principal — sai sem alterar nada
  if (menuAtivo) {
    Serial.println(F(""));
    Serial.println(F("  [TIMEOUT] Sem selecao em 60s. Menu encerrado sem alteracoes."));
  }

  // Resumo final das configuracoes ativas
  Serial.println(F(""));
  Serial.println(F("  [SENS] -------- Configuracao Ativa --------"));
  Serial.print(F(  "  [SENS]   Escala      : +/- "));
  Serial.print(escalaAtualG);
  Serial.println(F(" g"));
  Serial.print(F(  "  [SENS]   VIB_AVISO   : "));
  Serial.print(VIB_AVISO, 2);
  Serial.println(F(" g"));
  Serial.print(F(  "  [SENS]   VIB_CRITICA : "));
  Serial.print(VIB_CRITICA, 2);
  Serial.println(F(" g"));
  Serial.println(F("  [SENS] -------------------------------------------"));
  Serial.println(F("  [SENS] Monitoramento retomado."));
  Serial.println();

  emCalibracao = false; // Libera o loop de monitoramento
}

// =============================================================================
//  FUNCAO: calcularVibracaoRMS
//
//  Le 50 amostras do acelerometro MPU6050 e calcula a magnitude RMS resultante.
//  Aplica os offsets de calibracao em cada eixo antes do calculo.
//
//  O offsetAZ ja contem ~1g (gravidade), portanto ao subtrair o offset
//  a gravidade estatica e removida automaticamente — sem dupla subtracao.
//
//  Retorna: vibracao em unidades g (forca gravitacional)
// =============================================================================
float calcularVibracaoRMS() {
  const int NUM_AMOSTRAS  = 50;
  float     somaQuadrados = 0.0;
  sensors_event_t accel, gyro, temp;

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.getEvent(&accel, &gyro, &temp);

    // Converte de m/s² para g
    float ax = accel.acceleration.x / 9.81;
    float ay = accel.acceleration.y / 9.81;
    float az = accel.acceleration.z / 9.81;

    // Aplica offsets de calibracao — remove o erro estatico de cada eixo.
    // offsetAX/AY/AZ sao zero ate que calibrarPontoZero() seja chamada.
    ax = ax - offsetAX;
    ay = ay - offsetAY;
    // Eixo Z: subtrai o offset bruto (que ja contem ~1g) para remover gravidade.
    // offsetAZ ≈ 1g em repouso → apos subtracao az ≈ 0g em repouso.
    // A gravidade e removida AQUI, uma unica vez. NAO e removida em calibrarPontoZero().
    az = az - offsetAZ;

    somaQuadrados += (ax * ax) + (ay * ay) + (az * az);

    delay(2); // Aguarda nova amostra — evita leituras duplicadas em cache
  }

  return sqrt(somaQuadrados / NUM_AMOSTRAS);
}

// =============================================================================
//  FUNCAO: calcularHealthScore
//
//  Calcula a pontuacao de saude do motor de 0 a 100%.
//
//  Pesos:
//    - Temperatura : 40%
//    - Vibracao    : 40%
//    - Horimetro   : 20%
//
//  Retorna: healthScore entre 0.0 e 100.0
// =============================================================================
float calcularHealthScore(float temp, float vib, float horas) {
  // --- Componente de temperatura (0.0 a 1.0) ---
  // Abaixo de TEMP_AVISO_MAX → saude plena
  // Acima de TEMP_CRITICA    → saude zero neste componente
  float scoreTemp;
  if (temp <= TEMP_AVISO_MAX) {
    scoreTemp = 1.0;
  } else if (temp >= TEMP_CRITICA) {
    scoreTemp = 0.0;
  } else {
    // Degradacao linear entre TEMP_AVISO_MAX e TEMP_CRITICA
    scoreTemp = 1.0 - ((temp - TEMP_AVISO_MAX) / (TEMP_CRITICA - TEMP_AVISO_MAX));
  }

  // --- Componente de vibracao (0.0 a 1.0) ---
  // Abaixo de VIB_AVISO  → saude plena
  // Acima de VIB_CRITICA → saude zero neste componente
  float scoreVib;
  if (vib <= VIB_AVISO) {
    scoreVib = 1.0;
  } else if (vib >= VIB_CRITICA) {
    scoreVib = 0.0;
  } else {
    // Degradacao linear entre VIB_AVISO e VIB_CRITICA
    scoreVib = 1.0 - ((vib - VIB_AVISO) / (VIB_CRITICA - VIB_AVISO));
  }

  // --- Componente de horimetro (0.0 a 1.0) ---
  // Degrada nos ultimos 20% do intervalo de manutencao
  float limiteDegrade = INTERVALO_MANUTENCAO_H * 0.80; // 80% do intervalo = 400h
  float scoreHoras;
  if (horas <= limiteDegrade) {
    scoreHoras = 1.0;
  } else if (horas >= INTERVALO_MANUTENCAO_H) {
    scoreHoras = 0.0;
  } else {
    // Degradacao linear nos ultimos 20% do intervalo
    scoreHoras = 1.0 - ((horas - limiteDegrade) / (INTERVALO_MANUTENCAO_H - limiteDegrade));
  }

  // Score final ponderado: Temperatura 40% | Vibracao 40% | Horimetro 20%
  return (scoreTemp * 40.0) + (scoreVib * 40.0) + (scoreHoras * 20.0);
}

// =============================================================================
//  FUNCAO: classificarAlarme
//
//  Determina o nivel de alarme com base nos valores medidos.
//  Prioridade: CRITICO > AVISO > NORMAL
//
//  Retorna: String com o nivel de alarme
// =============================================================================
String classificarAlarme(float temp, float vib) {
  if (temp >= TEMP_CRITICA || vib >= VIB_CRITICA) {
    return "*** CRITICO ***";
  }
  if (temp >= TEMP_AVISO_MAX || vib >= VIB_AVISO) {
    return "    AVISO    ";
  }
  return "    NORMAL   ";
}

// =============================================================================
//  FUNCAO: obterClassificacaoHealth
//  Retorna string descritiva para o health score calculado.
// =============================================================================
String obterClassificacaoHealth(float score) {
  if (score >= 90.0) return "Excelente";
  if (score >= 70.0) return "Bom";
  if (score >= 50.0) return "Manutencao Recomendada";
  return "CONDICAO CRITICA";
}

// =============================================================================
//  FUNCAO: imprimirRelatorio
//  Formata e imprime no Monitor Serial o relatorio completo de status do motor.
// =============================================================================
void imprimirRelatorio() {
  float horasRestantes = INTERVALO_MANUTENCAO_H - horimetro;
  if (horasRestantes < 0) horasRestantes = 0;

  Serial.println(F("------------------------------------------------"));
  Serial.print(F("  Timestamp       : "));
  Serial.print(millis());
  Serial.println(F(" ms"));
  Serial.println(F("------------------------------------------------"));

  // Temperatura
  Serial.print(F("  Temperatura     : "));
  Serial.print(temperatura, 1);
  Serial.println(F(" oC"));

  // Vibracao
  Serial.print(F("  Vibracao        : "));
  Serial.print(vibracaoRMS, 3);
  Serial.println(F(" g"));

  // Horimetro
  Serial.print(F("  Horimetro       : "));
  Serial.print(horimetro, 4);
  Serial.println(F(" h"));

  // Proxima manutencao
  Serial.print(F("  Prox. Manut.    : "));
  Serial.print(horasRestantes, 1);
  Serial.println(F(" h restantes"));

  // Health Score
  Serial.print(F("  Health Score    : "));
  Serial.print(healthScore, 1);
  Serial.print(F(" %  ("));
  Serial.print(obterClassificacaoHealth(healthScore));
  Serial.println(F(")"));

  // Configuracao de sensibilidade ativa
  Serial.print(F("  Escala Acel.    : +/- "));
  Serial.print(escalaAtualG);
  Serial.print(F(" g  |  Aviso: "));
  Serial.print(VIB_AVISO, 2);
  Serial.print(F(" g  |  Critico: "));
  Serial.print(VIB_CRITICA, 2);
  Serial.println(F(" g"));

  // Alarme
  Serial.print(F("  Alarme          : "));
  Serial.println(statusAlarme);

  Serial.println(F("------------------------------------------------"));
  Serial.println();
}

// =============================================================================
//  SETUP — Executado uma unica vez na inicializacao
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("=============================================="));
  Serial.println(F("  Project SIGMA v0.1.2.1 - Iniciando Sistema..."));
  Serial.println(F("=============================================="));

  // ------------------------------------------
  //  Inicializacao do barramento I2C (MPU6050)
  //  Wire.setClock() obrigatorio — sem ele a Adafruit pode falhar no begin()
  //  delay(150) apos begin() — MPU6050 precisa de ao menos 100ms de boot interno
  // ------------------------------------------
  Wire.begin(PINO_MPU_SDA, PINO_MPU_SCL);
  Wire.setClock(400000); // 400 kHz — Fast Mode I2C
  delay(150);
  Serial.println(F("  [I2C] Barramento I2C iniciado (400 kHz Fast Mode)."));

  // Tenta conectar ao MPU6050 — ate 5 tentativas com 200ms de intervalo
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
    delay(200);
  }

  if (!mpuOk) {
    Serial.println(F(""));
    Serial.println(F("  [ERRO FATAL] MPU6050 nao encontrado!"));
    Serial.println(F("  Checklist de diagnostico:"));
    Serial.println(F("    1. Pull-up 4.7k no SDA (GPIO8) ligado ao 3.3V?"));
    Serial.println(F("    2. Pull-up 4.7k no SCL (GPIO9) ligado ao 3.3V?"));
    Serial.println(F("    3. VCC do MPU6050 ligado ao 3.3V?"));
    Serial.println(F("    4. GND do MPU6050 ligado ao GND?"));
    Serial.println(F("    5. Pino AD0 no GND = endereco 0x68 (padrao)"));
    Serial.println(F("    6. Pino AD0 no 3.3V = endereco 0x69"));
    Serial.println(F("  Sistema pausado. Corrija a fiacao e reinicie."));
    while (true) { delay(1000); }
  }
  Serial.println(F("  [OK] MPU6050 inicializado com sucesso."));

  // Configura escala do acelerometro para +/-8g (uso geral industrial)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println(F("  [MPU] Escala do acelerometro: +/- 8g"));

  // Configura escala do giroscopio para +/-500 deg/s
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.println(F("  [MPU] Escala do giroscopio: +/- 500 deg/s"));

  // Configura filtro passa-baixa para reduzir ruido (banda de 21 Hz)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println(F("  [MPU] Filtro passa-baixa: 21 Hz"));

  // ------------------------------------------
  //  Calibracao automatica de ponto zero do MPU6050
  //
  //  REGRA: no boot, a calibracao ocorre AUTOMATICAMENTE sem pedir confirmacao.
  //  A confirmacao (S/N) so e exibida quando o usuario envia 'C' manualmente.
  //
  //  O usuario pode cancelar a calibracao do boot enviando 'X' durante a coleta.
  // ------------------------------------------
  Serial.println(F(""));
  Serial.println(F("  [SIGMA] Calibracao automatica de ponto zero iniciando..."));
  Serial.println(F("  [SIGMA] Mantenha o sensor PARADO e NIVELADO!"));
  Serial.println(F("  [SIGMA] Envie 'X' a qualquer momento para cancelar."));
  delay(2000); // Pausa para o usuario posicionar o sensor antes de coletar

  emCalibracao = true;
  bool calOk = calibrarPontoZero(); // Calibra direto — sem confirmacao no boot

  if (!calOk) {
    // Usuario cancelou a calibracao durante o boot
    Serial.println(F("  [INFO] Calibracao cancelada no boot. Offsets = 0."));
    Serial.println(F("  [INFO] Envie 'C' a qualquer momento para calibrar."));
    Serial.println();
  }

  // ------------------------------------------
  //  Inicializacao do DS18B20 (temperatura)
  // ------------------------------------------
  sensorTemp.begin(); // Inicia o barramento 1-Wire e detecta sensores
  int qtdSensores = sensorTemp.getDeviceCount();
  Serial.print(F("  [DS18B20] Sensores encontrados: "));
  Serial.println(qtdSensores);

  if (qtdSensores == 0) {
    Serial.println(F("  [AVISO] Nenhum DS18B20 detectado! Verifique pull-up e fiacao."));
  } else {
    // Resolucao maxima de 12 bits (precisao de 0.0625 oC)
    sensorTemp.setResolution(12);
    Serial.println(F("  [DS18B20] Resolucao configurada: 12 bits (0.0625 oC)"));
  }

  // inicioSistema atribuido APOS toda a inicializacao e calibracao.
  // Garante que o horimetro conta apenas o tempo real de operacao do motor,
  // sem incluir o tempo de boot, inicializacao de sensores ou calibracao.
  inicioSistema = millis();
  ultimaLeitura = millis();

  Serial.println(F("=============================================="));
  Serial.println(F("  Sistema pronto. Iniciando monitoramento..."));
  Serial.println(F("  Comandos Serial disponiveis:"));
  Serial.println(F("    C = Recalibrar ponto zero do MPU6050"));
  Serial.println(F("    A = Menu de ajuste de sensibilidade"));
  Serial.println(F("=============================================="));
  Serial.println();
}

// =============================================================================
//  LOOP — Executado continuamente
// =============================================================================
void loop() {
  unsigned long agora = millis();

  // ------------------------------------------
  //  Verifica comandos do usuario via Serial
  //  Comandos disponiveis:
  //    C = Recalibrar ponto zero do MPU6050 (com confirmacao S/N)
  //    A = Abrir menu de ajuste de sensibilidade
  // ------------------------------------------
  if (!emCalibracao && Serial.available() > 0) {
    char cmd = Serial.read();

    // Comando 'A' — abre o menu de ajuste de sensibilidade do acelerometro
    // Permite selecionar escala (2g/4g/8g/16g) e ajustar limiares de alarme
    if (cmd == 'A' || cmd == 'a') {
      Serial.println(F(""));
      Serial.println(F("  [CMD] Menu de sensibilidade ativado!"));
      menuSensibilidade();
    }

    // Comando 'C' — inicia fluxo de recalibracao com confirmacao do usuario
    // DIFERENTE do boot: aqui o usuario precisa confirmar com S antes de calibrar
    if (cmd == 'C' || cmd == 'c') {
      Serial.println(F(""));
      Serial.println(F("  [CMD] Comando de recalibracao recebido!"));
      Serial.println(F("  [CMD] Monitoramento pausado durante o processo."));

      emCalibracao = true;

      // Exibe prompt de confirmacao — exclusivo do comando manual 'C'
      bool confirmado = aguardarConfirmacaoSerial(30000);

      if (confirmado) {
        bool calOk = calibrarPontoZero();
        if (!calOk) {
          Serial.println(F("  [INFO] Calibracao interrompida. Offsets anteriores mantidos."));
          Serial.println(F("  [INFO] Monitoramento retomado normalmente."));
          Serial.println();
        }
      } else {
        emCalibracao = false; // Cancela e retorna ao monitoramento
      }
    }
  }

  // ------------------------------------------
  //  Loop principal de monitoramento
  //  Bloqueado enquanto emCalibracao = true
  // ------------------------------------------
  if (!emCalibracao && agora - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora;

    // 1. Atualiza horimetro de operacao
    // Converte milissegundos para horas (1h = 3.600.000 ms)
    horimetro = (float)(agora - inicioSistema) / 3600000.0;

    // 2. Leitura de temperatura (DS18B20)
    sensorTemp.requestTemperatures();
    float leituraTemp = sensorTemp.getTempCByIndex(0);

    // -127 indica erro de comunicacao — mantem ultimo valor valido
    if (leituraTemp != DEVICE_DISCONNECTED_C) {
      temperatura = leituraTemp;
    } else {
      Serial.println(F("  [AVISO] Falha na leitura do DS18B20!"));
    }

    // 3. Leitura de vibracao (MPU6050) — RMS de 50 amostras com offsets aplicados
    vibracaoRMS = calcularVibracaoRMS();

    // 4. Calculo do Health Score e classificacao do alarme
    healthScore  = calcularHealthScore(temperatura, vibracaoRMS, horimetro);
    statusAlarme = classificarAlarme(temperatura, vibracaoRMS);

    // 5. Impressao do relatorio no Serial Monitor
    imprimirRelatorio();
  }
}