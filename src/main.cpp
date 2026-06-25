// =============================================================================
//  Project SIGMA - Sistema de Monitoramento Preditivo de Motor Industrial
//  Codinome   : Project SIGMA
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Sensores   : DS18B20 (temperatura) + MPU6050 (vibração)
//  Protocolo  : Serial 115200 baud
//  Versao     : 0.1.1.4
// =============================================================================

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
bool calibrarPontoZero();          // Retorna true = concluída, false = abortada pelo usuário
bool aguardarConfirmacaoSerial(uint32_t timeoutMs);
bool confirmarSaidaCalibracao();    // Prompt de confirmação quando usuário pede para sair
void menuPosCalibracao();           // Menu pós-calibração: Sair (S) ou Recalibrar (R)

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
bool  emCalibracao = false; // Flag: bloqueia o loop de monitoramento durante calibração

// =============================================================================
//  FUNÇÃO: aguardarConfirmacaoSerial
//
//  Exibe um prompt interativo no Serial Monitor e aguarda resposta do usuário.
//  O sistema fica bloqueado até receber 'S' (confirmar) ou 'N' (cancelar),
//  ou até o tempo de timeout ser atingido.
//
//  Parâmetros:
//    timeoutMs : tempo máximo de espera em milissegundos
//
//  Retorna:
//    true  → usuário confirmou com 'S' ou 's'
//    false → usuário cancelou com 'N'/'n' ou timeout atingido
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

  uint32_t inicio = millis(); // Marca o início da espera
  uint32_t segundosRestantes = timeoutMs / 1000;
  uint32_t ultimoSegundo = millis();

  // Exibe o tempo restante no primeiro instante
  Serial.print(F("  Aguardando resposta... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  // Loop de espera — verifica Serial a cada ciclo
  while ((millis() - inicio) < timeoutMs) {

    // Atualiza contador regressivo a cada segundo sem travar o loop
    if ((millis() - ultimoSegundo) >= 1000) {
      ultimoSegundo = millis();
      segundosRestantes--;
      Serial.print(F("  Aguardando resposta... tempo restante: "));
      Serial.print(segundosRestantes);
      Serial.print(F(" s   \r")); // \r sobrescreve a mesma linha no terminal
    }

    // Verifica se chegou um caractere no buffer serial
    if (Serial.available() > 0) {
      char resposta = Serial.read();

      if (resposta == 'S' || resposta == 's') {
        // Usuário confirmou — prosseguir com a calibração
        Serial.println(F(""));
        Serial.println(F("  [OK] Calibracao CONFIRMADA pelo usuario."));
        return true;
      }

      if (resposta == 'N' || resposta == 'n') {
        // Usuário cancelou — retornar ao monitoramento
        Serial.println(F(""));
        Serial.println(F("  [INFO] Calibracao CANCELADA pelo usuario."));
        Serial.println(F("  [INFO] Retornando ao monitoramento normal..."));
        return false;
      }
      // Qualquer outro caractere é ignorado — aguarda S ou N
    }

    delay(50); // Pequena pausa para não sobrecarregar o loop de espera
  }

  // Timeout atingido — cancela automaticamente por segurança
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Nenhuma resposta recebida em 30 segundos."));
  Serial.println(F("  [TIMEOUT] Calibracao CANCELADA automaticamente."));
  Serial.println(F("  [INFO] Retornando ao monitoramento normal..."));
  return false;
}

// =============================================================================
//  FUNÇÃO: confirmarSaidaCalibracao
//
//  Exibida quando o usuário envia 'X' durante a coleta de amostras.
//  Pergunta se o usuário realmente quer abortar a calibração em andamento.
//
//  Retorna:
//    true  → usuário confirmou saída (S) — calibração será abortada
//    false → usuário cancelou saída (N) — calibração continua normalmente
// =============================================================================
bool confirmarSaidaCalibracao() {
  // Limpa buffer serial antes de aguardar a resposta
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

  // Timeout de 60 segundos: se nao responder, cancela a saida e CONTINUA a coleta.
  // Comportamento seguro: preferencia por NAO descartar amostras ja coletadas.
  const uint32_t TIMEOUT_SAIDA_MS  = 60000;
  uint32_t inicioEspera            = millis();
  uint32_t segundosRestantes       = TIMEOUT_SAIDA_MS / 1000;
  uint32_t ultimoSegundo           = millis();

  Serial.print(F("  Aguardando resposta... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  while ((millis() - inicioEspera) < TIMEOUT_SAIDA_MS) {

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
        // Usuario confirmou saida - abortar calibracao
        Serial.println(F(""));
        Serial.println(F("  [CAL] Saida CONFIRMADA. Calibracao abortada."));
        Serial.println(F("  [CAL] Amostras parciais descartadas."));
        return true;
      }

      if (resposta == 'N' || resposta == 'n') {
        // Usuario desistiu de sair - continuar calibracao
        Serial.println(F(""));
        Serial.println(F("  [CAL] Saida CANCELADA. Retomando coleta de amostras..."));
        return false;
      }
      // Qualquer outro caractere e ignorado - aguarda S ou N
    }
    delay(50);
  }

  // Timeout atingido - comportamento seguro: NAO abortar, continua a coleta
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 60s. Coleta retomada automaticamente."));
  return false; // false = nao sair, continuar calibrando
}

// =============================================================================
//  FUNÇÃO: menuPosCalibracao
//
//  Exibida imediatamente após a calibração ser concluída com sucesso.
//  Pergunta ao usuário se deseja sair ou realizar uma nova calibração.
//
//  Comandos aceitos:
//    S (ou s) → Sai do menu e retoma o monitoramento normal
//    R (ou r) → Reinicia uma nova calibração sem precisar reiniciar o sistema
//
//  Timeout de 60 segundos: se nao responder, sai automaticamente ao monitoramento.
// =============================================================================
void menuPosCalibracao() {
  // Limpa qualquer dado residual no buffer serial antes de exibir o menu
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

  // Timeout de 60 segundos: se nao responder, sai automaticamente.
  // Comportamento seguro: o monitoramento e retomado com os offsets aplicados.
  const uint32_t TIMEOUT_MENU_MS = 60000;
  uint32_t inicioMenu            = millis();
  uint32_t segundosRestantes     = TIMEOUT_MENU_MS / 1000;
  uint32_t ultimoSegundo         = millis();

  Serial.print(F("  Aguardando resposta... tempo restante: "));
  Serial.print(segundosRestantes);
  Serial.print(F(" s   \r"));

  while ((millis() - inicioMenu) < TIMEOUT_MENU_MS) {

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
        // Usuario escolheu sair - retoma o monitoramento
        Serial.println(F(""));
        Serial.println(F("  [MENU] Saindo do menu de calibracao."));
        Serial.println(F("  [MENU] Monitoramento retomado com os novos offsets."));
        Serial.println();
        emCalibracao = false; // Libera o loop de monitoramento
        return;
      }

      if (resposta == 'R' || resposta == 'r') {
        // Usuario escolheu recalibrar - inicia nova calibracao imediatamente
        Serial.println(F(""));
        Serial.println(F("  [MENU] Iniciando nova calibracao..."));
        Serial.println(F("  [MENU] Certifique-se que o sensor esta PARADO e NIVELADO!"));
        delay(1000); // Pequena pausa para o usuario posicionar o sensor

        // Chama calibracao diretamente (sem prompt de confirmacao novamente,
        // pois o usuario ja demonstrou intencao explicita ao pressionar R)
        bool calOk = calibrarPontoZero();

        if (!calOk) {
          // Nova calibracao foi abortada durante a coleta
          // emCalibracao ja foi liberado dentro de calibrarPontoZero()
          return;
        }
        // Se calOk == true: calibrarPontoZero() chamou menuPosCalibracao() novamente
        // (recursao controlada) - o retorno chega aqui e sai do loop atual
        return;
      }
      // Qualquer outro caractere e ignorado - aguarda S ou R
    }
    delay(50);
  }

  // Timeout atingido - sai automaticamente ao monitoramento
  Serial.println(F(""));
  Serial.println(F("  [TIMEOUT] Sem resposta em 60s. Retornando ao monitoramento."));
  Serial.println();
  emCalibracao = false; // Libera o loop de monitoramento
}

// =============================================================================
//  FUNÇÃO: calibrarPontoZero
//
//  Realiza a calibração estática do MPU6050 coletando 200 amostras em repouso.
//  O sensor DEVE estar parado e nivelado durante esta operação.
//
//  O que faz:
//    1. Coleta NUM_AMOSTRAS_CAL leituras brutas do acelerômetro
//    2. Verifica a cada amostra se o usuário enviou 'X' para sair
//    3. Se 'X' for recebido, pergunta se quer realmente sair (S/N)
//    4. Se confirmar saída: descarta amostras parciais e retorna false
//    5. Se concluída: calcula offsets, aplica e retorna true
//
//  Retorna:
//    true  → calibração concluída com sucesso, offsets atualizados
//    false → calibração abortada pelo usuário, offsets NÃO alterados
// =============================================================================
bool calibrarPontoZero() {
  const int NUM_AMOSTRAS_CAL = 200; // Número de amostras para calcular o offset

  Serial.println(F(""));
  Serial.println(F("  ==============================================="));
  Serial.println(F("  [CAL] INICIANDO CALIBRACAO DE PONTO ZERO"));
  Serial.println(F("  [CAL] Mantenha o sensor PARADO e NIVELADO!"));
  Serial.println(F("  [CAL] Dica: envie 'X' a qualquer momento para sair."));
  Serial.println(F("  [CAL] Coletando amostras..."));

  // Acumuladores temporários — só são transferidos para os offsets globais
  // se a calibração for CONCLUÍDA. Se abortada, os offsets anteriores são mantidos.
  float somaX = 0.0; // Acumulador do eixo X
  float somaY = 0.0; // Acumulador do eixo Y
  float somaZ = 0.0; // Acumulador do eixo Z
  int   amostrasColetadas = 0; // Contador de amostras válidas coletadas

  sensors_event_t accel, gyro, temp; // Estruturas de evento do sensor

  for (int i = 0; i < NUM_AMOSTRAS_CAL; i++) {
    mpu.getEvent(&accel, &gyro, &temp); // Lê os três eixos do acelerômetro

    // Converte de m/s² para g e acumula cada eixo
    somaX += accel.acceleration.x / 9.81;
    somaY += accel.acceleration.y / 9.81;
    somaZ += accel.acceleration.z / 9.81;
    amostrasColetadas++; // Contabiliza amostra válida

    delay(5); // Pequena pausa entre amostras para evitar leituras duplicadas

    // Imprime progresso a cada 50 amostras
    if ((i + 1) % 50 == 0) {
      Serial.print(F("  [CAL] "));
      Serial.print(i + 1);
      Serial.print(F(" / "));
      Serial.print(NUM_AMOSTRAS_CAL);
      Serial.println(F(" amostras coletadas... (envie 'X' para sair)"));
    }

    // ---------------------------------------------------------------
    //  Verifica se o usuário enviou 'X' para interromper a calibração
    //  Verificado a cada amostra para resposta imediata
    // ---------------------------------------------------------------
    if (Serial.available() > 0) {
      char tecla = Serial.read();

      if (tecla == 'X' || tecla == 'x') {
        Serial.println(F(""));
        Serial.print(F("  [CAL] Comando de saida recebido apos "));
        Serial.print(amostrasColetadas);
        Serial.println(F(" amostras."));

        // Pergunta se realmente quer sair
        bool sairConfirmado = confirmarSaidaCalibracao();

        if (sairConfirmado) {
          // Usuário confirmou saída — descarta tudo e retorna
          Serial.println(F("  [CAL] Calibracao ABORTADA pelo usuario."));
          Serial.println(F("  [CAL] Offsets anteriores mantidos sem alteraçao."));
          if (!calibrado) {
            // Nunca teve calibração antes — avisa sobre offsets zerados
            Serial.println(F("  [CAL] ATENCAO: Nenhuma calibracao ativa. Offsets = 0."));
            Serial.println(F("  [CAL] Leituras podem conter erro estatico do sensor."));
          }
          Serial.println(F("  [CAL] Envie 'C' para tentar calibrar novamente."));
          Serial.println(F("  ==============================================="));
          Serial.println();
          emCalibracao = false; // Libera o loop de monitoramento
          return false;         // Sinaliza que calibração não foi concluída
        }
        // Usuário escolheu continuar — o for prossegue normalmente
      }
      // Qualquer outro caractere durante a coleta é ignorado
    }
  }

  // ---------------------------------------------------------------
  //  Calibração concluída — calcula e aplica os offsets globais
  // ---------------------------------------------------------------

  // Calcula a média de cada eixo a partir dos acumuladores
  offsetAX = somaX / NUM_AMOSTRAS_CAL;
  offsetAY = somaY / NUM_AMOSTRAS_CAL;
  // No eixo Z: o sensor deve ler +1g em repouso (gravidade).
  // O offset é a média BRUTA do eixo Z — inclui o 1g da gravidade.
  // A subtração de 1g é feita UMA única vez dentro de calcularVibracaoRMS().
  // IMPORTANTE: NÃO subtrair 1g aqui para evitar dupla remoção de gravidade.
  offsetAZ = somaZ / NUM_AMOSTRAS_CAL;

  calibrado = true; // Marca calibração como concluída

  // Exibe resultado da calibração no Serial Monitor
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

  // ---------------------------------------------------------------
  //  Exibe menu pós-calibração:
  //  Usuário escolhe entre SAIR (S) ou RECALIBRAR (R)
  //  A função gerencia o flag emCalibracao internamente
  // ---------------------------------------------------------------
  menuPosCalibracao();

  return true; // Sinaliza calibração concluída com sucesso
}

// =============================================================================
//  FUNÇÃO: calcularVibracaoRMS
//  Lê 50 amostras do acelerômetro MPU6050 e calcula a magnitude RMS resultante.
//  Aplica os offsets de calibração em cada eixo antes do cálculo.
//  O offsetAZ já contém ~1g (gravidade), portanto ao subtrair o offset
//  a gravidade estática é removida automaticamente — sem dupla subtração.
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

    // Aplica offsets de calibração — remove o erro estático de cada eixo.
    // (offsetAX/AY/AZ são zero até que calibrarPontoZero() seja chamada)
    ax = ax - offsetAX;
    ay = ay - offsetAY;
    // Eixo Z: subtrai o offset bruto (que já contém ~1g) para remover gravidade.
    // offsetAZ ≈ 1g em repouso ideal → após a subtração az ≈ 0g em repouso.
    // A gravidade é removida AQUI, uma única vez. Não é removida no calibrar().
    az = az - offsetAZ;

    // Acumula o quadrado da magnitude resultante dos 3 eixos
    somaQuadrados += (ax * ax) + (ay * ay) + (az * az);

    delay(2); // Aguarda nova amostra do MPU6050 — evita leituras duplicadas em cache
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
  Serial.println(F("  Project SIGMA v0.1.1.4 - Iniciando Sistema..."));
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
  //  Pergunta ao usuário se deseja calibrar antes de prosseguir.
  //  Timeout de 30 segundos: se não responder, calibração é pulada.
  //  Os offsets permanecem em zero até que uma calibração seja realizada.
  // ------------------------------------------
  Serial.println(F(""));
  Serial.println(F("  [SIGMA] Calibracao de ponto zero disponivel."));

  // Chama confirmação interativa com timeout de 30 segundos
  emCalibracao = true;
  bool usuarioConfirmou = aguardarConfirmacaoSerial(30000);

  if (usuarioConfirmou) {
    // Executa calibração — retorna true se concluída, false se abortada pelo usuário
    bool calOk = calibrarPontoZero();
    if (!calOk) {
      // Usuário saiu durante a calibração no boot — offsets permanecem zerados
      Serial.println(F("  [INFO] Calibracao interrompida no boot. Offsets = 0."));
      Serial.println(F("  [INFO] Envie 'C' no Serial para calibrar quando pronto."));
      Serial.println();
    }
  } else {
    emCalibracao = false; // Garante liberação do flag mesmo sem calibrar
    Serial.println(F("  [INFO] Calibracao pulada. Offsets zerados — leituras podem ter erro estatico."));
    Serial.println(F("  [INFO] Envie 'C' a qualquer momento para calibrar."));
    Serial.println();
  }

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

  // Salva o momento de inicio APOS toda a inicializacao e calibracao.
  // IMPORTANTE: atribuir aqui garante que o horimetro nao conta o tempo
  // gasto no boot, na inicializacao dos sensores e na calibracao do MPU6050.
  // O horimetro reflete apenas o tempo real de operacao do motor.
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
  //  Verifica comando de recalibracao via Serial
  //  Envie o caractere 'C' (maiusculo ou minusculo) no Monitor Serial
  //  para iniciar o fluxo interativo de recalibracao.
  //  O sistema pergunta se o usuario deseja prosseguir antes de calibrar.
  //  Se nao responder em 30 segundos, o comando e cancelado automaticamente.
  // ------------------------------------------
  if (!emCalibracao && Serial.available() > 0) {
    char cmd = Serial.read(); // Lê o caractere enviado

    if (cmd == 'C' || cmd == 'c') {
      Serial.println(F(""));
      Serial.println(F("  [CMD] Comando de recalibracao recebido!"));
      Serial.println(F("  [CMD] Monitoramento pausado durante o processo."));

      emCalibracao = true; // Pausa o loop de leitura

      // Exibe prompt de confirmação interativa com timeout de 30 segundos
      bool confirmado = aguardarConfirmacaoSerial(30000);

      if (confirmado) {
        // Executa calibração — retorna true se concluída, false se abortada
        bool calOk = calibrarPontoZero();
        if (!calOk) {
          // Usuário saiu durante a coleta de amostras
          Serial.println(F("  [INFO] Calibracao interrompida. Offsets anteriores mantidos."));
          Serial.println(F("  [INFO] Monitoramento retomado normalmente."));
          Serial.println();
        }
      } else {
        emCalibracao = false; // Cancela e retorna ao monitoramento
      }
    }
  }

  // Verifica se o intervalo de leitura foi atingido
  // O bloco de monitoramento é ignorado enquanto o sistema estiver em calibração
  if (!emCalibracao && agora - ultimaLeitura >= INTERVALO_LEITURA_MS) {
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