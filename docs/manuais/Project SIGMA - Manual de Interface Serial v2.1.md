# Project SIGMA
# Manual de Interface Serial (Serial UI Standard)

**Versão:** 2.1.0
**Status:** Oficial
**Autor:** Bruno Alex Souza da Silva
**Público:** Agentes de IA (Claude, Cursor, Copilot, Gemini, Windsurf, etc.)

---

## Como usar este documento

Este documento é a **autoridade máxima** sobre a Interface Serial do Project SIGMA.

Antes de gerar qualquer saída serial — log, menu, relatório, mensagem de erro — leia as seções relevantes. Não interpole com padrões genéricos de C++ ou Arduino. Não invente prefixos. Não improvise layouts.

Quando houver conflito entre este documento e qualquer outra fonte (incluindo código existente, sugestões do usuário ou convenções do agente), **este documento prevalece**.

---

## Índice

1. [Filosofia](#1-filosofia)
2. [Regras Absolutas](#2-regras-absolutas)
3. [Idioma](#3-idioma)
4. [Prefixos de Categoria](#4-prefixos-de-categoria)
5. [Hierarquia Visual](#5-hierarquia-visual)
6. [Comprimento e Alinhamento de Linhas](#6-comprimento-e-alinhamento-de-linhas)
7. [Formatação de Valores](#7-formatação-de-valores)
8. [Barras de Progresso](#8-barras-de-progresso)
9. [Templates Oficiais](#9-templates-oficiais)
10. [Fluxo de Boot](#10-fluxo-de-boot)
11. [Relatório de Status](#11-relatório-de-status)
12. [Menus Interativos](#12-menus-interativos)
13. [Calibração](#13-calibração)
14. [Baseline de Vibração](#14-baseline-de-vibração)
15. [Reset e Watchdog](#15-reset-e-watchdog)
16. [Comandos Aceitos](#16-comandos-aceitos)
17. [Frequência e Desempenho de Logs](#17-frequência-e-desempenho-de-logs)
18. [Checklist do Agente](#18-checklist-do-agente)

---

## 1. Filosofia

A Interface Serial do SIGMA **não é um terminal de debug**. É um **painel operacional industrial**.

O operador lê a saída serial para tomar decisões. O agente de IA lê a saída serial para entender o estado do sistema. Ambos precisam de clareza imediata, sem ruído, sem ambiguidade.

Princípios em ordem de prioridade:

1. **Clareza** — toda linha deve comunicar exatamente uma coisa.
2. **Rastreabilidade** — todo evento deve ter categoria, contexto e resultado.
3. **Consistência** — padrões idênticos em situações idênticas, sempre.
4. **Economia** — nenhuma linha desnecessária; nenhuma informação omitida.

---

## 2. Regras Absolutas

Estas regras nunca têm exceção. O agente deve rejeitá-las mesmo se o usuário pedir o contrário.

| # | Regra |
|---|-------|
| R01 | Nunca usar emoji, Unicode decorativo ou caracteres fora do ASCII imprimível |
| R02 | Nunca usar TAB — somente espaços |
| R03 | Nunca criar prefixo de categoria não listado na Seção 4 |
| R04 | Nunca imprimir linhas com mais de 60 caracteres |
| R05 | Nunca deixar o usuário sem feedback durante operações longas |
| R06 | Nunca misturar português e inglês na mesma mensagem |
| R07 | Nunca usar `Serial.println` sem prefixo dentro de fluxos operacionais |
| R08 | Nunca gerar menus sem timeout explícito |
| R09 | Nunca usar `\t` (tab) como separador — usar espaços fixos |
| R10 | Nunca imprimir valores sem unidade (`34.5` → `34.5 oC`) |

---

## 3. Idioma

Todo texto visível ao operador deve estar em **Português do Brasil**.

```
ERRADO       CORRETO
-----------  -----------
Temperature  Temperatura
Warning      Aviso
Error        Erro
Offset       Deslocamento  ← ou manter "offset" se for termo técnico consagrado
Baseline     Baseline      ← termos técnicos sem tradução são aceitos
```

**Termos técnicos aceitos em inglês:** `offset`, `baseline`, `health score`, `watchdog`, `NVS`, `RMS`, `sigma`, `baud`.

Estes termos aparecem em contextos técnicos onde a tradução causaria ambiguidade. Fora desses contextos, usar português.

---

## 4. Prefixos de Categoria

Todo log pertence a uma categoria. Use o prefixo correspondente. Nunca crie categorias novas.

| Prefixo | Uso |
|---------|-----|
| `[SIGMA]` | Mensagens gerais do sistema sem categoria específica |
| `[SYS]` | Inicialização, estado global, contadores |
| `[I2C]` | Barramento I2C |
| `[MPU]` | Sensor MPU6050 |
| `[DS18B20]` | Sensor de temperatura |
| `[CAL]` | Calibração de offset do acelerômetro |
| `[BAS]` | Baseline de vibração |
| `[NVS]` | Operações de leitura/escrita na flash NVS |
| `[WDT]` | Watchdog de hardware |
| `[MENU]` | Navegação de menus |
| `[CMD]` | Recepção e despacho de comandos seriais |
| `[SENS]` | Configuração de sensibilidade/escala |
| `[INFO]` | Informações operacionais sem urgência |
| `[WARN]` | Condição fora do esperado, ainda recuperável |
| `[ERRO]` | Falha que impede operação normal |
| `[TIMEOUT]` | Encerramento por ausência de resposta |
| `[STATUS]` | Linha de status avulsa fora do relatório |
| `[RESET]` | Operações de reset do sistema |

**Formato obrigatório:**
```
  [PREFIXO] Mensagem objetiva.
```

Dois espaços de indentação antes do prefixo. Ponto final na mensagem.

---

## 5. Hierarquia Visual

Quatro elementos estruturais, do mais para o menos importante:

### Título Principal
Usado em: boot, telas de módulo principal.
```
============================================================
```
60 caracteres exatos de `=`.

### Subtítulo / Separador de Seção
Usado em: divisões internas de uma tela, agrupamentos de dados.
```
------------------------------------------------------------
```
60 caracteres exatos de `-`.

### Caixa de Menu ou Confirmação
Usado em: menus interativos, confirmações, avisos críticos.
```
+----------------------------------------------------------+
|                                                          |
+----------------------------------------------------------+
```
Largura interna: 58 caracteres. Conteúdo centrado ou alinhado à esquerda com 2 espaços de margem.

### Linhas de Dados
Usadas em: relatórios, status, listas de parâmetros.
```
  Temperatura     : 34.5 oC
  Vibracao RMS    : 0.842 g
  Health Score    : 97.3 %
```
Dois espaços de indentação. Rótulo alinhado, dois pontos, valor com unidade.

---

## 6. Comprimento e Alinhamento de Linhas

**Limite absoluto: 60 caracteres por linha** (incluindo indentação).

Para verificar: conte os caracteres ou mantenha o `Serial.println(F("..."))` com string de no máximo 58 chars visíveis mais 2 de indentação.

**Alinhamento de rótulos em blocos de dados:**

Alinhe os dois-pontos na mesma coluna dentro do bloco. Use espaços, nunca TAB.

```
ERRADO:
  Temp: 34.5 oC
  Vibracao RMS: 0.842 g
  Health Score: 97.3 %

CORRETO:
  Temperatura     : 34.5 oC
  Vibracao RMS    : 0.842 g
  Health Score    : 97.3 %
```

---

## 7. Formatação de Valores

| Grandeza | Formato | Exemplo |
|----------|---------|---------|
| Temperatura | `XX.X oC` (1 decimal) | `34.5 oC` |
| Vibração RMS | `X.XXX g` (3 decimais) | `0.842 g` |
| Vibração offset | `X.XXXXX g` (5 decimais) | `0.00312 g` |
| Health Score | `XX.X %` (1 decimal) | `97.3 %` |
| Horímetro | `XXX.X h` (1 decimal) | `483.2 h` |
| Fator K | `X.XX` (2 decimais) | `2.00` |
| Desvio em sigma | `X.XX sigma` | `1.43 sigma` |
| Limiar calculado | `X.XXXX g` (4 decimais) | `1.0234 g` |
| Escala do acelerômetro | `+/- Xg` | `+/- 8g` |
| Contagem de amostras | inteiro simples | `300` |
| Tempo restante | `XX s` | `28 s` |
| Baud rate | inteiro simples | `115200` |

**Regras:**
- Sempre incluir unidade, sem exceção (R10).
- `oC` é o substituto ASCII para `°C`. Nunca usar `°` (Unicode).
- Separador decimal: ponto (`.`), nunca vírgula.

---

## 8. Barras de Progresso

Use barras ASCII para operações longas (calibração, coleta de baseline, etc.).

**Formato obrigatório:**
```
  [######################------] 73%
```

- Largura total da barra: 30 caracteres entre colchetes.
- `#` para progresso completo, `-` para restante.
- Percentual sem espaço antes de `%`.
- Atualizar na **mesma linha** com `\r`. Nunca quebrar linha durante o progresso.
- Ao concluir, imprimir `\n` para avançar a linha.

**Exemplo de implementação:**
```cpp
// Atualiza barra de progresso na mesma linha
void imprimirBarra(int atual, int total) {
  int pct   = (atual * 100) / total;
  int cheio = (atual * 30)  / total;
  Serial.print(F("  ["));
  for (int i = 0; i < 30; i++)
    Serial.print(i < cheio ? '#' : '-');
  Serial.print(F("] "));
  Serial.print(pct);
  Serial.print(F("%  "));
  Serial.print(atual);
  Serial.print(F(" / "));
  Serial.print(total);
  Serial.print(F(" \r"));
  esp_task_wdt_reset();
}
// Ao finalizar:
Serial.println();
```

---

## 9. Templates Oficiais

### 9.1 Template de Tela Genérica

```
============================================================
 [TITULO DA TELA EM MAIUSCULAS]
============================================================

  [CAT] Descricao da operacao em andamento.

------------------------------------------------------------

  Parametro 1     : valor
  Parametro 2     : valor

------------------------------------------------------------

  [CAT] Resultado da operacao.

============================================================
```

### 9.2 Template de Caixa de Confirmacao

```
+----------------------------------------------------------+
|  [TITULO DA ACAO]                                        |
+----------------------------------------------------------+
|                                                          |
|  Descricao clara do que sera feito.                      |
|                                                          |
|  [S] = CONFIRMAR  |  [N] = CANCELAR                     |
|                                                          |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s
```

### 9.3 Template de Confirmacao Dupla

Para ações irreversíveis (apagar NVS, reset de config):

```
+----------------------------------------------------------+
|  *** ATENCAO — ACAO IRREVERSIVEL ***                    |
|  [descricao do que sera apagado/resetado]                |
|  1a CONFIRMACAO: [S] = Continuar  |  [N] = Cancelar     |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s

[usuario digita S]

+----------------------------------------------------------+
|  *** CONFIRMACAO FINAL ***                               |
|  Digite  [RESET]  + Enter para confirmar                 |
|  Digite  [N]      para cancelar                          |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s
```

### 9.4 Template de Mensagem de Erro

```
  [ERRO] Descricao objetiva da falha.
  [ERRO] Causa provavel ou acao recomendada.
```

### 9.5 Template de Timeout

```
  [TIMEOUT] Sem resposta. [Acao tomada automaticamente].
```

---

## 10. Fluxo de Boot

O boot deve seguir exatamente esta sequência e este formato.

```
============================================================
 Project SIGMA vX.X.X - Iniciando...
============================================================

  [WDT]     Watchdog configurado (timeout: Xs).
  [SYS]     Resets: N | Ultimo: [motivo]
  [NVS]     Horimetro: XX.X h
  [NVS]     Configuracao carregada (sigma_cfg).
  [NVS]       Escala: +/- Xg  |  Aviso: X.XX g  |  Crit: X.XX g
  [NVS]     Baseline carregado (sigma_bas):
  [NVS]       Media  : X.XXXXX g
  [NVS]       Stddev : X.XXXXX g
  [NVS]       Fator K: X.XX
  [NVS]       Amostras: XXX
  [I2C]     Barramento iniciado (400 kHz).
  [I2C]     Tentativa N/5 MPU6050... ENCONTRADO!
  [OK]      MPU6050 inicializado.
  [MPU]     Escala: +/- Xg
  [MPU]     Giroscopio: 500deg/s | Filtro: 21Hz
  [NVS]     Offsets encontrados e carregados!
  [NVS]       AX: X.XXXXX  AY: X.XXXXX  AZ: X.XXXXX
  [DS18B20] Sensores: N
  [DS18B20] Resolucao: 12 bits

============================================================
  Sistema pronto. Monitoramento iniciado.
  [C]=Recalibrar    [A]=Sensibilidade
  [N]=Apagar NVS    [H]=Zerar Hor.
  [R]=Reset Config  [B]=Coletar Baseline
  [K]=Ajustar K     [Z]=Apagar Baseline
============================================================
```

**Regras do boot:**
- Cada subsistema tem exatamente uma linha de status ao inicializar.
- Falhas fatais (MPU não encontrado) interrompem o boot e imprimem instruções de diagnóstico.
- Nenhuma linha de debug temporária deve permanecer no código de boot.

---

## 11. Relatório de Status

Emitido a cada ciclo de leitura (`INTERVALO_LEITURA_MS`). Nunca emitir mais de um relatório por ciclo.

```
------------------------------------------------------------
  Timestamp       : XXXXXXX ms
------------------------------------------------------------
  Temperatura     : XX.X oC
  Vibracao RMS    : X.XXX g
  Baseline Vib.   : X.XXXX g  |  Stddev: X.XXXX g   <- se ativo
  Desvio Atual    : X.XX sigma  |  Limiar(K=X.X): X.XXXX g
  Horimetro       : XXX.XXXX h  (base: XXX.X h)
  Prox. Manut.    : XXX.X h restantes
  Health Score    : XX.X %  (Classificacao)
  Escala Acel.    : +/- Xg  |  Aviso: X.XX g  |  Critico: X.XX g
  Offsets NVS     : Salvos (calibracao ativa)
  Resets Sistema  : N  | Ultimo: [motivo]
  Alarme          : [NORMAL / AVISO / CRITICO]
------------------------------------------------------------

```

**Quando baseline inativo**, a linha `Baseline Vib.` deve ser:
```
  Baseline Vib.   : INATIVO — use [B] para coletar
```

**Classificações do Health Score:**

| Faixa | Texto |
|-------|-------|
| >= 90 | Excelente |
| >= 70 | Bom |
| >= 50 | Manutencao Recomendada |
| < 50  | CONDICAO CRITICA |

---

## 12. Menus Interativos

### Regras obrigatórias de todo menu

- Todo menu exibe **timeout decrescente** na mesma linha com `\r`.
- Todo menu tem uma saída via `S` ou `N` claramente indicada.
- Todo menu encerra automaticamente ao atingir o timeout, com mensagem `[TIMEOUT]`.
- **Todo input digitável pelo usuário aparece entre colchetes:** letras, números e palavras.
- Exemplos: `[S]`, `[N]`, `[R]`, `[1]`, `[2]`, `[RESET]`, `[2.50]`.
- Confirmações usam `[S] = CONFIRMAR  |  [N] = CANCELAR`.

### Menu de Pós-Calibração

```
+----------------------------------------------------------+
|  CALIBRACAO CONCLUIDA COM SUCESSO!                       |
|  [S] = SAIR ao monitoramento                             |
|  [R] = RECALIBRAR novamente                              |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s
```

### Menu de Sensibilidade

```
+----------------------------------------------------------+
|  MENU DE SENSIBILIDADE DO ACELEROMETRO                   |
+----------------------------------------------------------+
|                                                          |
|  Escala atual     : +/- Xg                              |
|  VIB_AVISO atual  : X.XX g                              |
|  VIB_CRITICA atual: X.XX g                              |
|                                                          |
|  [1] = +/-  2g  Alta sens.  - bancada / laboratorio      |
|  [2] = +/-  4g  Media sens. - motores pequenos           |
|  [3] = +/-  8g  Uso geral   - industrial medio           |
|  [4] = +/- 16g  Baixa sens. - motores pesados            |
|  [M] = Ajuste MANUAL dos limiares de alarme              |
|  [S] = SAIR sem alterar configuracao                     |
|                                                          |
+----------------------------------------------------------+
  Aguardando selecao... tempo restante: XX s
```

### Menu de Ajuste do Fator K

```
+----------------------------------------------------------+
|  AJUSTE DO FATOR K (BASELINE)                            |
+----------------------------------------------------------+
|                                                          |
|  Limiar = Media + K x Desvio Padrao                      |
|                                                          |
|  [K=1.0] -> muito sensivel (1-sigma)                    |
|  [K=2.0] -> sensibilidade padrao (2-sigma)              |
|  [K=3.0] -> pouco sensivel (3-sigma)                    |
|  [K=5.0] -> minima sensibilidade                        |
|                                                          |
|  Fator K atual : X.XX                                    |
|  Limiar atual  : X.XXXX g                                |
|                                                          |
|  Digite o novo valor (ex: [2.50]) + Enter                |
|  ou [N] para cancelar                                    |
|                                                          |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s
```

---

## 13. Calibração

### Modo Automático (boot)

- Sem confirmação, sem menus.
- Imprime apenas as linhas de progresso e resultado.
- Retorna imediatamente ao boot.

```
  [CAL] Iniciando calibracao de ponto zero.
  [CAL] Mantenha o sensor PARADO e NIVELADO!
  [CAL] Coletando amostras...
  [############################--] 93%  186 / 200
  [CAL] Calibracao concluida com sucesso!
  [CAL] Offset AX : X.XXXXX g
  [CAL] Offset AY : X.XXXXX g
  [CAL] Offset AZ : X.XXXXX g (inclui ~1g)
  [NVS] Offsets salvos (sigma_cal).
```

### Modo Manual (comando C)

1. Recebe confirmação de que o sensor está parado.
2. Executa coleta com barra de progresso.
3. Exibe resultado e menu pós-calibração.

```
  [CMD] Recalibracao solicitada. Monitoramento pausado.

+----------------------------------------------------------+
|  CONFIRMACAO NECESSARIA                                  |
|  Sensor deve estar PARADO e NIVELADO!                    |
|  [S] = CONFIRMAR  |  [N] = CANCELAR                     |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s

[... barra de progresso ...]

  [CAL] Calibracao concluida com sucesso!
  [CAL] ------- Offsets Calculados -------
  [CAL]   Offset AX : X.XXXXX g
  [CAL]   Offset AY : X.XXXXX g
  [CAL]   Offset AZ : X.XXXXX g (inclui ~1g)
  [NVS] Offsets salvos (sigma_cal).

+----------------------------------------------------------+
|  CALIBRACAO CONCLUIDA COM SUCESSO!                       |
|  [S] = SAIR ao monitoramento                             |
|  [R] = RECALIBRAR novamente                              |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s
```

### Cancelamento durante calibração

```
+----------------------------------------------------------+
|  CONFIRMAR SAIDA DA CALIBRACAO?                          |
|  Amostras serao DESCARTADAS.                             |
|  [S] = SAIR  |  [N] = CONTINUAR                         |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s
```

---

## 14. Baseline de Vibração

### Coleta (comando B)

```
+----------------------------------------------------------+
|  COLETA DE BASELINE DE VIBRACAO                          |
+----------------------------------------------------------+
|                                                          |
|  IMPORTANTE: O motor deve estar em operacao              |
|  NORMAL durante toda a coleta.                           |
|                                                          |
|  O sistema aprendera o padrao de vibracao                |
|  tipico e usara como referencia para alarmes.            |
|                                                          |
|  Amostras a coletar: XXX                                 |
|                                                          |
|  [S] = CONFIRMAR coleta de baseline                      |
|  [N] = CANCELAR                                          |
|                                                          |
+----------------------------------------------------------+
  Aguardando... tempo restante: XX s

[usuario confirma]

  [BAS] Iniciando coleta... Motor deve estar NORMAL!
  [BAS] Nao interrompa o motor durante a coleta.

  [################--------------] 53%  159 / 300

  [BAS]  50 / 300 amostras  |  RMS atual: X.XXXX g
  [BAS] 100 / 300 amostras  |  RMS atual: X.XXXX g
  [BAS] 150 / 300 amostras  |  RMS atual: X.XXXX g
  [BAS] 200 / 300 amostras  |  RMS atual: X.XXXX g
  [BAS] 250 / 300 amostras  |  RMS atual: X.XXXX g
  [BAS] 300 / 300 amostras  |  RMS atual: X.XXXX g

  [BAS] ======= BASELINE CALCULADO =======
  [BAS]   Amostras coletadas : XXX
  [BAS]   Media (normal)     : X.XXXXX g
  [BAS]   Desvio padrao      : X.XXXXX g
  [BAS]   Fator K atual      : X.XX
  [BAS]   Limiar alarme rel. : X.XXXXX g
  [BAS] ========================================
  [BAS] Baseline salvo na NVS e ativo.
  [BAS] Use [K] para ajustar o fator de sensibilidade.
  [BAS] Use [Z] para apagar o baseline.
```

### Invalidação automática (mudança de escala)

```
  [BAS] Mudanca de escala invalida o baseline.
  [BAS] Baseline apagado — colete novamente com [B].
```

### Apagar baseline (comando Z)

```
  [CMD] Apagando baseline...
  [NVS] Baseline apagado (sigma_bas). RAM zerada.
  [CMD] Baseline apagado. Use [B] para coletar novo baseline.
```

---

## 15. Reset e Watchdog

### Reset provocado por falha de sensor

```
  [WARN] Falha DS18B20! (N/5)
  [WARN] Falha DS18B20! (N/5)   <- repete até MAX_FALHAS

  ================================================
  [RESET] REINICIANDO SISTEMA...
  [RESET] Motivo: Falha consecutiva DS18B20 (5x)
  ================================================
  [RESET] NVS salva. Reiniciando em 2 segundos...
```

### Watchdog configurado no boot

```
  [WDT] Watchdog configurado (timeout: Xs).
```

---

## 16. Comandos Aceitos

| Tecla | Ação | Categoria do log |
|-------|------|-----------------|
| `C` | Recalibrar acelerômetro | `[CMD]` / `[CAL]` |
| `A` | Menu de sensibilidade/escala | `[CMD]` / `[SENS]` |
| `N` | Apagar offsets da NVS e recalibrar | `[CMD]` / `[NVS]` |
| `H` | Zerar horímetro | `[NVS]` |
| `R` | Reset de configuração (confirmação dupla) | `[RESET]` |
| `B` | Coletar baseline de vibração | `[CMD]` / `[BAS]` |
| `K` | Ajustar fator K do baseline | `[CMD]` / `[BAS]` |
| `Z` | Apagar baseline da NVS | `[CMD]` / `[NVS]` |

**Comportamento padrão ao receber comando durante calibração:**

O comando é ignorado. Nenhuma mensagem é emitida. O fluxo de calibração continua normalmente.

---

## 17. Frequência e Desempenho de Logs

- **Relatório de status:** uma vez por ciclo (`INTERVALO_LEITURA_MS`). Nunca duplicar.
- **Falhas de sensor:** uma linha por falha, com contador.
- **Eventos de NVS:** uma linha por operação (salvar, carregar, apagar).
- **Progresso:** atualiza na mesma linha com `\r`. Nunca imprimir centenas de linhas.
- **Mensagens de debug temporárias:** proibidas no código de produção. Devem ser removidas antes de commitar.
- **Nenhuma mensagem deve ser emitida dentro de `calcularVibracaoRMS()`** ou outras funções chamadas a cada ciclo, exceto em condição de erro.

---

## 18. Checklist do Agente

Antes de submeter qualquer modificação, verificar:

```
[ ] Todas as strings usam F() macro para PROGMEM?
[ ] Nenhuma linha ultrapassa 60 caracteres?
[ ] Todos os logs usam prefixo da Seção 4?
[ ] Todos os valores têm unidade (Seção 7)?
[ ] Menus têm timeout explícito e mensagem [TIMEOUT]?
[ ] Barras de progresso usam \r (Seção 8)?
[ ] Nenhum emoji, Unicode ou TAB foi introduzido?
[ ] Nenhum prefixo novo foi criado?
[ ] Nenhuma mensagem de debug temporária permaneceu?
[ ] O idioma é exclusivamente português (exceto termos técnicos da Seção 3)?
[ ] Confirmações duplas aplicadas onde a ação é irreversível?
[ ] O baseline é apagado automaticamente ao mudar de escala?
[ ] esp_task_wdt_reset() chamado dentro de todos os loops de espera?
```

Se qualquer item falhar, corrigir antes de finalizar.

---

*Fim do documento. Versão 2.1.0 — Project SIGMA Serial UI Standard.*
