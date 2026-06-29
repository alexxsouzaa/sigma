// =============================================================
//  Project SIGMA
//  Arquivo    : CommandHandler.cpp
//  Descricao  : Implementacao do despachante de comandos.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.7.0
//  Codename   : Filtro Digital
//  Data       : 2026-06-29
// =============================================================
//
//  HISTORICO DE VERSOES
// =============================================================
//
//  [v0.1.6.3] - 2026-06-29
//  Autor  : Bruno Alex Souza da Silva
//  Status : Substituido
//  Resumo :
//    - Revisao completa de conformidade com o SUS v2.1.0.
//    - Correcao de larguras de caixa, countdowns, sequencias
//      canonicas de Z, C, R, B, A e K.
//
//  [v0.1.6.4] - 2026-06-29                           [CURRENT]
//  Autor  : Bruno Alex Souza da Silva
//  Status : Estavel
//  Resumo :
//    - lerLinhaSerial() reescrita com char[] (sem String).
//    - Limite de 3 tentativas no loop de recalibracao [C].
//    - Validacao de calibracao antes de iniciar coleta [B].
//    - Apagar baseline automaticamente no reset de config [R].
//    - Corrigida logica de cancelamento rapido no case [K].
//    - Feedback para comandos desconhecidos no default.
//    - Comentarios de secao padronizados conforme CCS v1.0.
//
// =============================================================

#include "CommandHandler.h"
#include <Arduino.h>
#include <ctype.h>
#include <math.h>
#include <esp_task_wdt.h>

// =============================================================
//  CONSTANTES INTERNAS
// =============================================================

// Limite de recalibrações consecutivas no case [C].
// Protege contra loop indefinido por pressão repetida de [R].
static const uint8_t MAX_RECALIBRACOES = 3;

// Tamanho maximo do buffer de leitura serial.
// Suficiente para "RESET" (5) e valores float como "5.00" (4).
static const uint8_t SERIAL_BUF_SIZE = 8;

// =============================================================
//  FUNCAO ESTATICA: cbProgresso
//  Adaptador de callback para injetar a UI na amostragem.
//  Chamado pelo CalibrationService a cada amostra coletada.
// -------------------------------------------------------------
//  Parametros:
//    context : Ponteiro opaco para CommandContext.
//    atual   : Numero da amostra atual.
//    total   : Total de amostras previstas.
// =============================================================
static void cbProgresso(void* context, int atual, int total) {
  CommandContext* ctx = static_cast<CommandContext*>(context);
  ctx->ui.imprimirBarraProgresso(atual, total);
}

// =============================================================
//  FUNCAO ESTATICA: lerLinhaSerial
//  Captura uma linha da Serial em buffer de tamanho fixo.
//  Nao usa String do Arduino — sem alocacao dinamica em runtime.
//  Descarta quebras de linha residuais antes de iniciar leitura.
// -------------------------------------------------------------
//  Parametros:
//    outBuf    : Buffer de saida. Sempre terminado com '\0'.
//    bufSize   : Tamanho maximo do buffer (incluindo '\0').
//    timeoutMs : Tempo maximo de espera em milissegundos.
//  Retorno   : true se linha valida recebida antes do timeout.
//  Observ.   : Caracteres alem de (bufSize - 1) sao descartados.
//              Chamador deve garantir bufSize >= 2.
// =============================================================
static bool lerLinhaSerial(char* outBuf,
                           uint8_t bufSize,
                           uint32_t timeoutMs) {
  uint8_t  pos   = 0;
  uint32_t inicio = millis();

  // Zera o buffer antes de comecar
  for (uint8_t i = 0; i < bufSize; i++) outBuf[i] = '\0';

  // Descarta quebras de linha residuais do buffer Serial
  while (Serial.available() > 0 &&
         (Serial.peek() == '\n' || Serial.peek() == '\r')) {
    Serial.read();
  }

  while ((millis() - inicio) < timeoutMs) {
    esp_task_wdt_reset();

    if (Serial.available() > 0) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        // Fim de linha — retorna true se capturou algo
        if (pos > 0) return true;
        continue; // Ignora linhas em branco e tenta novamente
      }

      // Aceita caractere apenas se ha espaco no buffer
      if (pos < (bufSize - 1)) {
        outBuf[pos++] = c;
        outBuf[pos]   = '\0';
      }
      // Caracteres excedentes sao silenciosamente descartados
    }
    delay(10);
  }
  return false; // Timeout esgotado sem linha valida
}

// =============================================================
//  FUNCAO ESTATICA: strIgual
//  Compara buffer com string literal de forma case-sensitive.
//  Substitui String::equals() sem alocacao dinamica.
// -------------------------------------------------------------
//  Parametros:
//    buf      : Buffer lido por lerLinhaSerial.
//    literal  : String literal para comparacao.
//  Retorno  : true se identicos, false caso contrario.
// =============================================================
static bool strIgual(const char* buf, const char* literal) {
  uint8_t i = 0;
  while (buf[i] != '\0' && literal[i] != '\0') {
    if (buf[i] != literal[i]) return false;
    i++;
  }
  return (buf[i] == '\0' && literal[i] == '\0');
}

// =============================================================
//  FUNCAO ESTATICA: strParaFloat
//  Converte buffer ASCII para float sem usar String::toFloat().
//  Suporta sinal negativo e um ponto decimal.
// -------------------------------------------------------------
//  Parametros:
//    buf : Buffer lido por lerLinhaSerial.
//  Retorno : Valor float convertido. 0.0f se invalido.
// =============================================================
static float strParaFloat(const char* buf) {
  float  result  = 0.0f;
  float  decimal = 0.0f;
  bool   negativo  = false;
  bool   emDecimal = false;
  float  divisor   = 10.0f;
  uint8_t i = 0;

  if (buf[0] == '-') { negativo = true; i = 1; }

  for (; buf[i] != '\0'; i++) {
    char c = buf[i];
    if (c == '.') {
      emDecimal = true;
      continue;
    }
    if (c < '0' || c > '9') return 0.0f; // Caracter invalido
    if (!emDecimal) {
      result = result * 10.0f + (c - '0');
    } else {
      decimal += (c - '0') / divisor;
      divisor *= 10.0f;
    }
  }

  result += decimal;
  return negativo ? -result : result;
}

// =============================================================
//  FUNCAO: processar
//  Le a Serial de forma nao bloqueante e executa acoes de UI.
//  Deve ser chamada a cada ciclo do loop() principal.
//  Nao deve ser chamada enquanto emCalibracao for true.
// =============================================================
void CommandHandler::processar(CommandContext& ctx) {
  if (Serial.available() == 0) return;

  char cmd = toupper(Serial.read());

  switch (cmd) {

    // ---------------------------------------------------------
    //  [H] Zerar Horimetro
    // ---------------------------------------------------------
    case 'H': {
      ctx.nvsHor.apagar();
      ctx.horData.horimetro  = 0.0f;
      ctx.horData.resetCount = 0;
      ctx.inicioSistemaMs    = millis();
      Serial.println(F(""));
      Serial.println(F(
        "  [NVS] Horimetro e contadores zerados."));
      Serial.println(F(
        "  [NVS] Contagem de horas reiniciada."));
      Serial.println(F(""));
      break;
    }

    // ---------------------------------------------------------
    //  [Z] Apagar Baseline da NVS
    //  Sequencia canonica definida no SUS, Secao 14.
    // ---------------------------------------------------------
    case 'Z': {
      Serial.println(F(""));
      Serial.println(F("  [CMD] Apagando baseline..."));
      ctx.nvsBas.apagar();
      ctx.basData.basAtivo = false;
      Serial.println(F(
        "  [NVS] Baseline apagado (sigma_bas). RAM zerada."));
      Serial.println(F(
        "  [CMD] Use [B] para coletar novo baseline."));
      Serial.println(F(""));
      break;
    }

    // ---------------------------------------------------------
    //  [N] Apagar Offsets de Calibracao da NVS
    // ---------------------------------------------------------
    case 'N': {
      Serial.println(F(""));
      Serial.println(F("  [CMD] Apagando NVS cal..."));
      ctx.nvsCal.apagar();
      ctx.calData.calibrado = false;
      ctx.calData.offsetAX  = 0.0f;
      ctx.calData.offsetAY  = 0.0f;
      ctx.calData.offsetAZ  = 0.0f;
      Serial.println(F(
        "  [NVS] Offsets apagados (sigma_cal). RAM zerada."));
      Serial.println(F(""));
      break;
    }

    // ---------------------------------------------------------
    //  [C] Recalibrar Acelerometro (Ponto Zero)
    //  Fluxo completo definido no SUS, Secao 13.
    //  Limite de MAX_RECALIBRACOES tentativas consecutivas para
    //  evitar que o sistema fique indefinidamente fora do
    //  monitoramento por pressão repetida de [R].
    // ---------------------------------------------------------
    case 'C': {
      uint8_t tentativas = 0;
      bool    recalibrar = true;

      while (recalibrar && tentativas < MAX_RECALIBRACOES) {
        recalibrar = false;
        tentativas++;

        // --- Aviso de tentativa quando nao e a primeira ---
        if (tentativas > 1) {
          Serial.print(F("  [CAL] Tentativa "));
          Serial.print(tentativas);
          Serial.print(F(" de "));
          Serial.print(MAX_RECALIBRACOES);
          Serial.println(F("."));
        }

        // --- Confirmacao inicial ---
        Serial.println(F(""));
        Serial.println(F(
          "  [CMD] Recalibracao solicitada. Monitoramento pausado."));
        Serial.println(F(""));
        Serial.println(F(
          "+----------------------------------------------------------+"));
        Serial.println(F(
          "|  CONFIRMACAO NECESSARIA                                  |"));
        Serial.println(F(
          "|  Sensor deve estar PARADO e NIVELADO!                    |"));
        Serial.println(F(
          "|  [S] = CONFIRMAR  |  [N] = CANCELAR                     |"));
        Serial.println(F(
          "+----------------------------------------------------------+"));

        if (!ctx.ui.aguardarConfirmacao(
              "Calibracao confirmada.",
              "Calibracao CANCELADA.",
              30000)) {
          break;
        }

        // --- Coleta de amostras ---
        Serial.println(F(""));
        Serial.println(F(
          "  [CAL] Iniciando calibracao de ponto zero."));
        Serial.println(F(
          "  [CAL] Mantenha o sensor PARADO e NIVELADO!"));
        Serial.println(F(
          "  [CAL] Coletando amostras..."));

        CalibrationResult res =
          ctx.srvCal.executar(ctx.driverVib, cbProgresso, &ctx);

        Serial.println(F(""));

        if (!res.sucesso) {
          Serial.println(F(
            "  [ERRO] Falha na leitura do sensor durante calibracao."));
          Serial.println(F(""));
          break;
        }

        // --- Aplica e persiste os offsets ---
        ctx.calData.offsetAX  = res.offsetAX;
        ctx.calData.offsetAY  = res.offsetAY;
        ctx.calData.offsetAZ  = res.offsetAZ;
        ctx.calData.calibrado = true;
        ctx.nvsCal.salvar(ctx.calData);

        // --- Resultado conforme SUS, Secao 13 ---
        Serial.println(F(
          "  [CAL] Calibracao concluida com sucesso!"));
        Serial.println(F(
          "  [CAL] ------- Offsets Calculados -------"));
        Serial.print(F("  [CAL]   Offset AX : "));
        Serial.print(res.offsetAX, 5);
        Serial.println(F(" g"));
        Serial.print(F("  [CAL]   Offset AY : "));
        Serial.print(res.offsetAY, 5);
        Serial.println(F(" g"));
        Serial.print(F("  [CAL]   Offset AZ : "));
        Serial.print(res.offsetAZ, 5);
        Serial.println(F(" g (inclui ~1g)"));
        Serial.println(F(
          "  [NVS] Offsets salvos (sigma_cal)."));
        Serial.println(F(""));

        // --- Menu pos-calibracao conforme SUS, Secao 13 ---
        while (Serial.available()) Serial.read();
        Serial.println(F(
          "+----------------------------------------------------------+"));
        Serial.println(F(
          "|  CALIBRACAO CONCLUIDA COM SUCESSO!                       |"));
        Serial.println(F(
          "|  [S] = SAIR ao monitoramento                             |"));

        // Exibe opcao [R] apenas se ainda ha tentativas disponiveis
        if (tentativas < MAX_RECALIBRACOES) {
          Serial.println(F(
            "|  [R] = RECALIBRAR novamente                              |"));
        }
        Serial.println(F(
          "+----------------------------------------------------------+"));

        {
          uint32_t inicio   = millis();
          uint32_t segs     = 60;
          uint32_t ultSeg   = millis();
          bool     aguardando = true;

          Serial.print(F(
            "  Aguardando... tempo restante: 60 s\r"));

          while (aguardando && (millis() - inicio) < 60000) {
            esp_task_wdt_reset();

            if ((millis() - ultSeg) >= 1000) {
              ultSeg = millis();
              segs--;
              Serial.print(F(
                "  Aguardando... tempo restante: "));
              Serial.print(segs);
              Serial.print(F(" s   \r"));
            }

            if (Serial.available() > 0) {
              char r = toupper(Serial.read());
              Serial.println(F(""));

              if (r == 'S') {
                Serial.println(F(
                  "  [MENU] Monitoramento retomado."));
                Serial.println(F(""));
                aguardando = false;
              } else if (r == 'R' &&
                         tentativas < MAX_RECALIBRACOES) {
                Serial.println(F(
                  "  [MENU] Iniciando nova calibracao..."));
                Serial.println(F(""));
                aguardando = false;
                recalibrar = true;
              }
            }
            delay(50);
          }

          if (aguardando) {
            Serial.println(F(""));
            Serial.println(F(
              "  [TIMEOUT] Sem resposta. Monitoramento retomado."));
            Serial.println(F(""));
          }
        }
      }

      // Limite de tentativas atingido
      if (recalibrar && tentativas >= MAX_RECALIBRACOES) {
        Serial.println(F(""));
        Serial.println(F(
          "  [WARN] Limite de recalibrações atingido."));
        Serial.println(F(
          "  [MENU] Monitoramento retomado automaticamente."));
        Serial.println(F(""));
      }
      break;
    }

    // ---------------------------------------------------------
    //  [R] Reset de Configuracao
    //  Confirmacao dupla obrigatoria conforme SUS, Secao 9.3.
    //  Apaga baseline automaticamente pois a escala e restaurada
    //  para 8g — baseline coletado em outra escala fica invalido.
    // ---------------------------------------------------------
    case 'R': {
      Serial.println(F(""));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|  *** ATENCAO - ACAO IRREVERSIVEL ***                     |"));
      Serial.println(F(
        "|  Sera apagado permanentemente:                           |"));
      Serial.println(F(
        "|    - Escala do acelerometro                              |"));
      Serial.println(F(
        "|    - Limiar VIB_AVISO                                    |"));
      Serial.println(F(
        "|    - Limiar VIB_CRITICA                                  |"));
      Serial.println(F(
        "|    - Baseline de vibracao (escala sera restaurada)       |"));
      Serial.println(F(
        "|  Padroes: Escala=8g | Aviso=2.00g | Crit=4.00g          |"));
      Serial.println(F(
        "|  1a CONFIRMACAO: [S] = Continuar  |  [N] = Cancelar     |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));

      if (!ctx.ui.aguardarConfirmacao(
            "Primeira confirmacao recebida.",
            "Reset cancelado.",
            30000)) {
        Serial.println(F(""));
        break;
      }

      // 2a confirmacao — digitar RESET
      while (Serial.available()) Serial.read();
      Serial.println(F(""));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|  *** CONFIRMACAO FINAL ***                               |"));
      Serial.println(F(
        "|  Digite  [RESET]  + Enter para confirmar                 |"));
      Serial.println(F(
        "|  Digite  [N]      para cancelar                          |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));

      {
        char buf[SERIAL_BUF_SIZE];
        if (lerLinhaSerial(buf, SERIAL_BUF_SIZE, 30000)) {
          if (strIgual(buf, "RESET")) {

            // Restaura configuracao
            ctx.nvsCfg.apagar();
            ctx.cfgData.escalaG    = 8;
            ctx.cfgData.vibAviso   = 2.0f;
            ctx.cfgData.vibCritica = 4.0f;
            ctx.driverVib.configurarEscala(ctx.cfgData.escalaG);
            ctx.nvsCfg.salvar(ctx.cfgData);

            // Apaga baseline — escala restaurada invalida dados
            // coletados com qualquer outra escala (SUS, Secao 14)
            ctx.nvsBas.apagar();
            ctx.basData.basAtivo = false;

            Serial.println(F(""));
            Serial.println(F(
              "  [RESET] Configuracoes restauradas ao padrao fabrica."));
            Serial.print(F("  [RESET]   Escala: +/- "));
            Serial.print(ctx.cfgData.escalaG);
            Serial.println(F("g"));
            Serial.print(F("  [RESET]   Aviso : "));
            Serial.print(ctx.cfgData.vibAviso, 2);
            Serial.println(F(" g"));
            Serial.print(F("  [RESET]   Crit. : "));
            Serial.print(ctx.cfgData.vibCritica, 2);
            Serial.println(F(" g"));
            Serial.println(F(
              "  [BAS] Baseline apagado - colete novamente com [B]."));
            Serial.println(F(""));

          } else {
            Serial.println(F(""));
            Serial.println(F(
              "  [RESET] Entrada invalida. Reset CANCELADO."));
            Serial.println(F(""));
          }
        } else {
          Serial.println(F(""));
          Serial.println(F(
            "  [TIMEOUT] Sem resposta. Reset CANCELADO."));
          Serial.println(F(""));
        }
      }
      break;
    }

    // ---------------------------------------------------------
    //  [K] Ajustar Fator K do Baseline
    //  Menu e confirmacao conforme SUS, Secao 12.
    //  Fluxo de cancelamento rapido ([N] sem Enter) tratado
    //  antes da chamada a lerLinhaSerial para evitar espera
    //  desnecessaria de 10s apos consumir o caracter 'N'.
    // ---------------------------------------------------------
    case 'K': {
      while (Serial.available()) Serial.read();

      Serial.println(F(""));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|  AJUSTE DO FATOR K (BASELINE)                            |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  Limiar = Media + K x Desvio Padrao                      |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  [K=1.0] -> muito sensivel (1-sigma)                    |"));
      Serial.println(F(
        "|  [K=2.0] -> sensibilidade padrao (2-sigma)              |"));
      Serial.println(F(
        "|  [K=3.0] -> pouco sensivel (3-sigma)                    |"));
      Serial.println(F(
        "|  [K=5.0] -> minima sensibilidade                        |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.print(F(
        "|  Fator K atual : "));
      Serial.print(ctx.basData.fatorK, 2);
      Serial.println(F("                              |"));

      if (ctx.basData.basAtivo) {
        float limiarAtual =
          ctx.basData.basMedia +
          (ctx.basData.fatorK * ctx.basData.basStddev);
        Serial.print(F(
          "|  Limiar atual  : "));
        Serial.print(limiarAtual, 4);
        Serial.println(F(" g                          |"));
      } else {
        Serial.println(F(
          "|  Baseline: INATIVO (colete com [B])                      |"));
      }

      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  Digite o novo valor (ex: [2.50]) + Enter                |"));
      Serial.println(F(
        "|  ou [N] + Enter para cancelar                            |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));

      {
        char     buf[SERIAL_BUF_SIZE];
        uint32_t inicio   = millis();
        uint32_t segs     = 30;
        uint32_t ultSeg   = millis();
        bool     recebido = false;

        Serial.print(F(
          "  Aguardando... tempo restante: 30 s\r"));

        // Loop de countdown — aguarda linha completa do operador.
        // Nao usa peek() para cancelamento rapido pois isso criava
        // um caminho onde o 'N' era consumido e lerLinhaSerial
        // esperava mais 10s sem nada para ler.
        // A solucao correta e tratar [N] como qualquer outra
        // entrada apos o Enter — verificado em strIgual() abaixo.
        while (!recebido && (millis() - inicio) < 30000) {
          esp_task_wdt_reset();

          if ((millis() - ultSeg) >= 1000) {
            ultSeg = millis();
            segs--;
            Serial.print(F(
              "  Aguardando... tempo restante: "));
            Serial.print(segs);
            Serial.print(F(" s   \r"));
          }

          if (Serial.available() > 0) {
            // Tenta capturar linha com timeout curto de 5s
            // (operador ja comeecou a digitar — ha dados disponiveis)
            if (lerLinhaSerial(buf, SERIAL_BUF_SIZE, 5000)) {
              recebido = true;
            }
          }
          delay(50);
        }

        if (!recebido) {
          Serial.println(F(""));
          Serial.println(F(
            "  [TIMEOUT] Sem resposta. Fator K mantido."));
          Serial.println(F(""));
          break;
        }

        // Cancelamento via [N] + Enter
        if (buf[0] == 'N' || buf[0] == 'n') {
          Serial.println(F(""));
          Serial.println(F(
            "  [INFO] Operacao cancelada pelo operador."));
          Serial.println(F(""));
          break;
        }

        float novoK = strParaFloat(buf);

        if (novoK < 1.0f || novoK > 5.0f) {
          Serial.println(F(""));
          Serial.println(F(
            "  [ERRO] Valor fora do range permitido (1.0 a 5.0)."));
          Serial.println(F(
            "  [ERRO] Fator K NAO alterado."));
          Serial.println(F(""));
          break;
        }

        // --- Previa da nova configuracao ---
        Serial.println(F(""));
        Serial.println(F(
          "------------------------------------------------------------"));
        Serial.print(F("  Novo Fator K : "));
        Serial.println(novoK, 2);

        if (ctx.basData.basAtivo) {
          float novoLimiar =
            ctx.basData.basMedia +
            (novoK * ctx.basData.basStddev);
          Serial.print(F("  Novo Limiar  : "));
          Serial.print(novoLimiar, 4);
          Serial.println(F(" g"));
        } else {
          Serial.println(F(
            "  [WARN] Baseline inativo. Limiar calculado apos coleta."));
        }

        Serial.println(F(
          "------------------------------------------------------------"));
        Serial.println(F(""));

        // --- Confirmacao do novo valor ---
        while (Serial.available()) Serial.read();
        Serial.println(F(
          "+----------------------------------------------------------+"));
        Serial.println(F(
          "|  CONFIRMAR NOVO FATOR K?                                 |"));
        Serial.println(F(
          "|  [S] = CONFIRMAR  |  [N] = CANCELAR                     |"));
        Serial.println(F(
          "+----------------------------------------------------------+"));

        if (ctx.ui.aguardarConfirmacao(
              "Fator K atualizado com sucesso.",
              "Alteracao cancelada.",
              15000)) {
          ctx.basData.fatorK = novoK;

          if (ctx.basData.basAtivo) {
            ctx.nvsBas.salvar(ctx.basData);
            float limiarNovo =
              ctx.basData.basMedia +
              (ctx.basData.fatorK * ctx.basData.basStddev);
            Serial.print(F("  [BAS] Novo limiar de alarme : "));
            Serial.print(limiarNovo, 4);
            Serial.println(F(" g"));
          }

          Serial.print(F("  [BAS] Fator K atualizado para: "));
          Serial.println(ctx.basData.fatorK, 2);
          Serial.println(F(""));
        }
      }
      break;
    }

    // ---------------------------------------------------------
    //  [B] Coletar Baseline de Vibracao
    //  Fluxo completo definido no SUS, Secao 14.
    //  Valida calibracao antes de iniciar — offsets zerados
    //  produziriam um baseline incorreto sem nenhum aviso.
    // ---------------------------------------------------------
    case 'B': {
      while (Serial.available()) Serial.read();

      // --- Guarda de calibracao ---
      // Baseline coletado sem calibracao aprende ruido de offset
      // em vez de vibracao real do motor. Bloqueia a coleta e
      // orienta o operador a calibrar primeiro.
      if (!ctx.calData.calibrado) {
        Serial.println(F(""));
        Serial.println(F(
          "  [WARN] Calibracao nao realizada."));
        Serial.println(F(
          "  [WARN] Baseline requer offsets validos."));
        Serial.println(F(
          "  [INFO] Pressione [C] para calibrar e tente novamente."));
        Serial.println(F(""));
        break;
      }

      Serial.println(F(""));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|  COLETA DE BASELINE DE VIBRACAO                          |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  IMPORTANTE: O motor deve estar em operacao              |"));
      Serial.println(F(
        "|  NORMAL durante toda a coleta.                           |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  O sistema aprendera o padrao de vibracao                |"));
      Serial.println(F(
        "|  tipico e usara como referencia para alarmes.            |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  Amostras a coletar: 300                                 |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  [S] = CONFIRMAR coleta de baseline                      |"));
      Serial.println(F(
        "|  [N] = CANCELAR                                          |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));

      if (!ctx.ui.aguardarConfirmacao(
            "Coleta de baseline iniciada.",
            "Coleta cancelada.",
            30000)) {
        Serial.println(F(""));
        break;
      }

      // --- Inicio da coleta ---
      Serial.println(F(""));
      Serial.println(F(
        "  [BAS] Iniciando coleta... Motor deve estar NORMAL!"));
      Serial.println(F(
        "  [BAS] Nao interrompa o motor durante a coleta."));
      Serial.println(F(""));

      float soma      = 0.0f;
      float somaQ     = 0.0f;
      int   coletadas = 0;
      bool  abortado  = false;
      float ultimoRMS = 0.0f;

      for (int i = 0; i < 300; i++) {
        esp_task_wdt_reset();

        // Media de 10 leituras rapidas por amostra do baseline
        float somaRapida = 0.0f;
        for (int j = 0; j < 10; j++) {
          Mpu6050Data bruto = ctx.driverVib.lerAceleracao(
            ctx.calData.offsetAX,
            ctx.calData.offsetAY,
            ctx.calData.offsetAZ);
          if (bruto.valido) {
            somaRapida += sqrt(
              (bruto.ax * bruto.ax) +
              (bruto.ay * bruto.ay) +
              (bruto.az * bruto.az));
          }
          delay(2);
        }
        ultimoRMS = somaRapida / 10.0f;

        soma  += ultimoRMS;
        somaQ += (ultimoRMS * ultimoRMS);
        coletadas++;

        ctx.ui.imprimirBarraProgresso(coletadas, 300);

        // Milestone a cada 50 amostras — avanca linha da barra
        if (coletadas % 50 == 0) {
          Serial.println(F(""));
          Serial.print(F("  [BAS] "));
          if (coletadas < 100) Serial.print(F(" "));
          Serial.print(coletadas);
          Serial.print(F(" / 300 amostras  |  RMS atual: "));
          Serial.print(ultimoRMS, 4);
          Serial.println(F(" g"));
        }

        // Abortar com [X]
        if (Serial.available() > 0) {
          if (toupper(Serial.read()) == 'X') {
            abortado = true;
            break;
          }
        }
        delay(10);
      }

      Serial.println(F(""));

      if (abortado || coletadas == 0) {
        Serial.println(F(
          "  [WARN] Coleta interrompida. Dados descartados."));
        Serial.println(F(""));
        break;
      }

      // --- Calculo do baseline ---
      ctx.basData.basMedia = soma / coletadas;
      float variancia =
        (somaQ / coletadas) -
        (ctx.basData.basMedia * ctx.basData.basMedia);

      // Protege contra variancia negativa por erro de ponto flutuante
      if (variancia < 0.0f) variancia = 0.0f;

      ctx.basData.basStddev = sqrt(variancia);

      // Stddev minimo para evitar divisao por zero no Health Score
      if (ctx.basData.basStddev < 0.001f)
        ctx.basData.basStddev = 0.001f;

      ctx.basData.basAtivo = true;
      ctx.basData.basN     = coletadas;

      ctx.nvsBas.salvar(ctx.basData);

      // --- Resultado conforme SUS, Secao 14 ---
      float limiarAlarme =
        ctx.basData.basMedia +
        (ctx.basData.fatorK * ctx.basData.basStddev);

      Serial.println(F(""));
      Serial.println(F(
        "  [BAS] ======= BASELINE CALCULADO ======="));
      Serial.print(F(
        "  [BAS]   Amostras coletadas : "));
      Serial.println(ctx.basData.basN);
      Serial.print(F(
        "  [BAS]   Media (normal)     : "));
      Serial.print(ctx.basData.basMedia, 5);
      Serial.println(F(" g"));
      Serial.print(F(
        "  [BAS]   Desvio padrao      : "));
      Serial.print(ctx.basData.basStddev, 5);
      Serial.println(F(" g"));
      Serial.print(F(
        "  [BAS]   Fator K atual      : "));
      Serial.println(ctx.basData.fatorK, 2);
      Serial.print(F(
        "  [BAS]   Limiar alarme rel. : "));
      Serial.print(limiarAlarme, 5);
      Serial.println(F(" g"));
      Serial.println(F(
        "  [BAS] ========================================"));
      Serial.println(F(
        "  [BAS] Baseline salvo na NVS e ativo."));
      Serial.println(F(
        "  [BAS] Use [K] para ajustar o fator de sensibilidade."));
      Serial.println(F(
        "  [BAS] Use [Z] para apagar o baseline."));
      Serial.println(F(""));
      break;
    }

    // ---------------------------------------------------------
    //  [A] Menu de Sensibilidade e Escala do Acelerometro
    //  Fluxo conforme SUS, Secao 12.
    // ---------------------------------------------------------
    case 'A': {
      while (Serial.available()) Serial.read();

      Serial.println(F(""));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|  MENU DE SENSIBILIDADE DO ACELEROMETRO                   |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));
      Serial.println(F(
        "|                                                          |"));
      Serial.print(F(
        "|  Escala atual     : +/- "));
      Serial.print(ctx.cfgData.escalaG);
      Serial.println(F("g                            |"));
      Serial.print(F(
        "|  VIB_AVISO atual  : "));
      Serial.print(ctx.cfgData.vibAviso, 2);
      Serial.println(F(" g                             |"));
      Serial.print(F(
        "|  VIB_CRITICA atual: "));
      Serial.print(ctx.cfgData.vibCritica, 2);
      Serial.println(F(" g                             |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "|  [1] = +/-  2g Alta sens. - bancada / lab                |"));
      Serial.println(F(
        "|  [2] = +/-  4g Media sens.- motores pequenos             |"));
      Serial.println(F(
        "|  [3] = +/-  8g Uso geral  - industrial medio             |"));
      Serial.println(F(
        "|  [4] = +/- 16g Baixa sens.- motores pesados              |"));
      Serial.println(F(
        "|  [S] = SAIR sem alterar configuracao                     |"));
      Serial.println(F(
        "|                                                          |"));
      Serial.println(F(
        "+----------------------------------------------------------+"));

      {
        uint32_t inicio    = millis();
        uint32_t segs      = 60;
        uint32_t ultSeg    = millis();
        bool     menuAtivo = true;

        Serial.print(F(
          "  Aguardando selecao... tempo restante: 60 s\r"));

        while (menuAtivo && (millis() - inicio) < 60000) {
          esp_task_wdt_reset();

          if ((millis() - ultSeg) >= 1000) {
            ultSeg = millis();
            segs--;
            Serial.print(F(
              "  Aguardando selecao... tempo restante: "));
            Serial.print(segs);
            Serial.print(F(" s   \r"));
          }

          if (Serial.available() > 0) {
            char opcao = toupper(Serial.read());
            Serial.println(F(""));

            if (opcao == 'S') {
              Serial.println(F(
                "  [SENS] Configuracao mantida. Saindo..."));
              Serial.println(F(""));
              menuAtivo = false;
              break;
            }

            if (opcao < '1' || opcao > '4') {
              Serial.println(F(
                "  [SENS] Opcao invalida. "
                "Digite [1],[2],[3],[4] ou [S]."));
              continue;
            }

            int   novaG = 8;
            float sAv   = 2.0f;
            float sCr   = 4.0f;

            if (opcao == '1') { novaG = 2;  sAv = 0.5f; sCr = 1.0f; }
            if (opcao == '2') { novaG = 4;  sAv = 1.0f; sCr = 2.0f; }
            if (opcao == '3') { novaG = 8;  sAv = 2.0f; sCr = 4.0f; }
            if (opcao == '4') { novaG = 16; sAv = 4.0f; sCr = 8.0f; }

            // --- Previa da nova configuracao ---
            Serial.println(F(""));
            Serial.println(F(
              "------------------------------------------------------------"));
            Serial.print(F("  Nova Escala : +/- "));
            Serial.print(novaG);
            Serial.println(F("g"));
            Serial.print(F("  Novo Aviso  : "));
            Serial.print(sAv, 2);
            Serial.println(F(" g"));
            Serial.print(F("  Novo Critico: "));
            Serial.print(sCr, 2);
            Serial.println(F(" g"));
            Serial.println(F(
              "------------------------------------------------------------"));
            Serial.println(F(""));

            // --- Confirmacao da nova escala ---
            while (Serial.available()) Serial.read();
            Serial.println(F(
              "+----------------------------------------------------------+"));
            Serial.println(F(
              "|  CONFIRMAR NOVA ESCALA E LIMIARES?                       |"));
            Serial.println(F(
              "|  [S] = CONFIRMAR  |  [N] = CANCELAR                     |"));
            Serial.println(F(
              "+----------------------------------------------------------+"));

            if (ctx.ui.aguardarConfirmacao(
                  "Nova sensibilidade aplicada.",
                  "Alteracao cancelada.",
                  15000)) {
              ctx.cfgData.escalaG    = novaG;
              ctx.cfgData.vibAviso   = sAv;
              ctx.cfgData.vibCritica = sCr;

              ctx.driverVib.configurarEscala(novaG);
              ctx.nvsCfg.salvar(ctx.cfgData);

              // Mudanca de escala invalida o baseline (SUS, Secao 14)
              ctx.nvsBas.apagar();
              ctx.basData.basAtivo = false;

              Serial.println(F(""));
              Serial.println(F(
                "  [SENS] Nova escala de hardware configurada."));
              Serial.print(F("  [SENS]   Escala      : +/- "));
              Serial.print(ctx.cfgData.escalaG);
              Serial.println(F("g"));
              Serial.print(F("  [SENS]   VIB_AVISO   : "));
              Serial.print(ctx.cfgData.vibAviso, 2);
              Serial.println(F(" g"));
              Serial.print(F("  [SENS]   VIB_CRITICA : "));
              Serial.print(ctx.cfgData.vibCritica, 2);
              Serial.println(F(" g"));
              Serial.println(F(
                "  [BAS] Mudanca de escala invalida o baseline."));
              Serial.println(F(
                "  [BAS] Baseline apagado - colete novamente com [B]."));
              Serial.println(F(
                "  [SENS] Monitoramento retomado."));
              Serial.println(F(""));
            }

            menuAtivo = false;
          }
          delay(50);
        }

        if (menuAtivo) {
          Serial.println(F(""));
          Serial.println(F(
            "  [TIMEOUT] Sem resposta. Configuracao mantida."));
          Serial.println(F(""));
        }
      }
      break;
    }

    // ---------------------------------------------------------
    //  default — Caracteres nao reconhecidos
    //  Quebras de linha (\n, \r) e espacos sao ignorados
    //  silenciosamente pois chegam como artefato do terminal.
    //  Letras e digitos desconhecidos recebem feedback ao
    //  operador conforme definido no ABC, Secao 7.2.
    // ---------------------------------------------------------
    default: {
      // Filtra silenciosamente artefatos comuns de terminal
      if (cmd == '\n' || cmd == '\r' || cmd == ' ') break;

      // Qualquer outro caracter imprimivel recebe feedback
      Serial.println(F(""));
      Serial.print(F("  [CMD] Comando desconhecido: ["));
      Serial.print(cmd);
      Serial.println(F("]"));
      Serial.println(F(
        "  [INFO] Comandos validos: C A B K R H N Z X."));
      Serial.println(F(""));
      break;
    }
  }
}