# Project SIGMA
# Manual do Agente de Documentação Técnica

**Versão:** 1.0.0
**Status:** Oficial
**Autor:** Bruno Alex Souza da Silva
**Público:** Agentes de IA (Claude, Cursor, Copilot, Gemini, Windsurf, etc.)
**Escopo:** Documentação técnica do firmware — arquitetura, módulos e fluxos.
**Formato de saída:** Markdown com exemplos de código C++.

---

## Como usar este documento

Este documento define como o agente deve criar, estruturar e manter toda
a documentação técnica do Project SIGMA.

O agente atua em dois cenários distintos:

- **Cenário A** — Documentar código existente: analisar o firmware e
  produzir documentação fiel ao que o código realmente faz.
- **Cenário B** — Documentar junto com o código: criar a documentação
  simultaneamente à entrega do código novo.

Em ambos os cenários, as mesmas regras e padrões se aplicam.

**Hierarquia de autoridade — ordem decrescente de prioridade:**

1. Manual de Interface Serial (SUS)
2. Manual de Comentários de Código (CCS)
3. Manual de Refatoração e Modularização (RAM)
4. Este documento

Quando houver conflito entre este documento e qualquer outra fonte,
este documento prevalece sobre fontes externas, mas cede aos três
documentos acima.

---

## Índice

1. [Filosofia da Documentação](#1-filosofia-da-documentação)
2. [Regras Absolutas](#2-regras-absolutas)
3. [Tipos de Documento](#3-tipos-de-documento)
4. [Estrutura de Arquivos de Documentação](#4-estrutura-de-arquivos-de-documentação)
5. [Padrões de Escrita](#5-padrões-de-escrita)
6. [Formatação Markdown](#6-formatação-markdown)
7. [Exemplos de Código C++](#7-exemplos-de-código-c)
8. [Template — Visão Geral do Sistema](#8-template--visão-geral-do-sistema)
9. [Template — Documento de Módulo](#9-template--documento-de-módulo)
10. [Template — Documento de Fluxo](#10-template--documento-de-fluxo)
11. [Template — Documento de Interface](#11-template--documento-de-interface)
12. [Template — Documento de Configuração](#12-template--documento-de-configuração)
13. [Cenário A — Documentar Código Existente](#13-cenário-a--documentar-código-existente)
14. [Cenário B — Documentar Junto com o Código](#14-cenário-b--documentar-junto-com-o-código)
15. [Manutenção e Versionamento](#15-manutenção-e-versionamento)
16. [O Que Nunca Documentar](#16-o-que-nunca-documentar)
17. [Checklist do Agente](#17-checklist-do-agente)

---

## 1. Filosofia da Documentação

A documentação do SIGMA não é um comentário expandido.
É um **artefato de engenharia independente**.

Um engenheiro que nunca viu o código deve ser capaz de:

1. Entender o propósito de cada módulo.
2. Entender como os módulos se comunicam.
3. Entender o comportamento do sistema em condição normal e em falha.
4. Integrar uma nova funcionalidade sem quebrar o que existe.
5. Diagnosticar um problema apenas lendo a documentação.

Se a documentação não permite isso, ela está incompleta.

### 1.1 Documentação como contrato

Toda documentação de módulo é um **contrato**:

- Define o que o módulo faz.
- Define o que o módulo não faz.
- Define o que o módulo espera receber.
- Define o que o módulo promete entregar.
- Define o que acontece quando algo falha.

O código implementa o contrato. A documentação o define.

### 1.2 Precisão acima de volume

Um documento de 300 linhas preciso e completo é superior a um de
1000 linhas com redundâncias, suposições e imprecisões.

Nunca preencher espaço com texto vago.
Nunca documentar o óbvio.
Sempre documentar o não óbvio.

### 1.3 Sincronismo obrigatório

Documentação desatualizada é pior que ausência de documentação.
Toda alteração de comportamento no código exige atualização imediata
do documento correspondente.

---

## 2. Regras Absolutas

| # | Regra |
|---|-------|
| R01 | Nunca documentar intenção — documentar comportamento real |
| R02 | Nunca usar linguagem vaga: "pode", "talvez", "geralmente" |
| R03 | Nunca omitir o comportamento em falha de nenhum módulo |
| R04 | Nunca criar documento sem seção de Versão e Data |
| R05 | Nunca usar inglês fora dos termos técnicos definidos na Seção 5.2 |
| R06 | Nunca documentar código comentado ou funcionalidade removida |
| R07 | Nunca usar capturas de tela como substituto de texto estruturado |
| R08 | Nunca deixar seção com conteúdo "a definir" ou "em breve" |
| R09 | Nunca criar documento sem sumário quando tiver mais de 5 seções |
| R10 | Todo exemplo de código deve compilar e representar o uso real |

---

## 3. Tipos de Documento

O agente produz cinco tipos de documento técnico. Cada tipo tem
estrutura, propósito e template próprios.

| Tipo | Arquivo | Propósito |
|------|---------|-----------|
| **Visão Geral** | `OVERVIEW.md` | Descreve o sistema inteiro: arquitetura, camadas, fluxo de boot, dependências externas. |
| **Módulo** | `docs/modulo/NomeModulo.md` | Descreve um único módulo: responsabilidade, interface pública, fluxos internos, falhas. |
| **Fluxo** | `docs/fluxos/NomeFluxo.md` | Descreve um fluxo de ponta a ponta: calibração, baseline, alarme, boot, reset. |
| **Interface** | `docs/interfaces/NomeInterface.md` | Descreve uma interface de comunicação: Serial (SUS), WebServer (endpoints, JSON). |
| **Configuração** | `docs/config/NomeConfig.md` | Descreve parâmetros configuráveis: constantes, NVS, limiares, seus impactos e faixas. |

---

## 4. Estrutura de Arquivos de Documentação

Toda documentação reside em `docs/` na raiz do projeto.
Nenhum documento de referência fica fora desta estrutura.

```
ProjectSIGMA/
│
├── README.md                        <- Entrada do projeto (nao tecnico)
├── OVERVIEW.md                      <- Visao geral tecnica do sistema
│
└── docs/
    │
    ├── modulos/                     <- Um arquivo por modulo
    │   ├── Mpu6050Driver.md
    │   ├── Ds18b20Driver.md
    │   ├── NvsCalibration.md
    │   ├── NvsHorimeter.md
    │   ├── NvsConfig.md
    │   ├── NvsBaseline.md
    │   ├── CalibrationService.md
    │   ├── BaselineService.md
    │   ├── HealthService.md
    │   ├── AlarmService.md
    │   ├── HorimeterService.md
    │   ├── SerialReport.md
    │   ├── CommandHandler.md
    │   └── SigmaWebServer.md
    │
    ├── fluxos/                      <- Um arquivo por fluxo completo
    │   ├── FluxoBoot.md
    │   ├── FluxoCalibracao.md
    │   ├── FluxoBaseline.md
    │   ├── FluxoAlarme.md
    │   ├── FluxoReset.md
    │   └── FluxoManutencao.md
    │
    ├── interfaces/                  <- Interfaces de comunicacao
    │   ├── InterfaceSerial.md
    │   └── InterfaceWebServer.md
    │
    └── config/                      <- Parametros e configuracoes
        ├── ConstantesDoSistema.md
        └── NamespacesNVS.md
```

---

## 5. Padrões de Escrita

### 5.1 Voz e tom

- Voz ativa, presente do indicativo.
- Direta ao ponto. Sem introduções longas.
- Orientada ao comportamento, não à intenção.

```
ERRADO:
  "Este módulo foi criado para facilitar a leitura do sensor."

CORRETO:
  "Este módulo lê temperatura do DS18B20 via barramento 1-Wire
   e retorna um struct Ds18b20Data com o valor em °C e flag
   de validade."
```

### 5.2 Idioma

Todo texto em **Português do Brasil**.

Termos técnicos aceitos em inglês (mesma lista do CCS e SUS):
`offset`, `baseline`, `health score`, `watchdog`, `NVS`, `RMS`,
`sigma`, `baud`, `driver`, `struct`, `namespace`, `buffer`,
`callback`, `stack`, `heap`, `flag`, `firmware`, `hardware`.

Nomes de funções, variáveis, arquivos e constantes do código aparecem
em inglês quando referenciados diretamente — sempre em formatação
`inline code`.

### 5.3 Referências cruzadas

Ao mencionar outro módulo, fluxo ou documento, sempre referenciar
com link Markdown relativo:

```markdown
Ver [`BaselineService`](../modulos/BaselineService.md) para detalhes
do cálculo estatístico.
```

### 5.4 Precisão de valores

Toda grandeza documentada inclui unidade e faixa válida:

```
ERRADO:
  "A temperatura é monitorada continuamente."

CORRETO:
  "A temperatura é lida a cada 500 ms. Faixa operacional:
   25,0 °C a 55,0 °C. Aviso acima de 55,0 °C.
   Crítico acima de 65,0 °C."
```

---

## 6. Formatação Markdown

### 6.1 Hierarquia de títulos

```markdown
# Título do Documento          <- Apenas um por arquivo
## Seção Principal             <- Divisões de primeiro nível
### Subseção                   <- Divisões de segundo nível
#### Detalhe                   <- Usar com moderação
```

Nunca usar `#####` ou mais níveis — indica estrutura mal planejada.

### 6.2 Tabelas

Usar tabelas para: parâmetros, campos de struct, comandos, constantes,
mapeamentos de falha. Nunca usar tabela para texto corrido.

```markdown
| Campo | Tipo | Unidade | Faixa | Descrição |
|-------|------|---------|-------|-----------|
| `temperaturaCelsius` | `float` | °C | -55 a 125 | Temperatura lida |
| `valido` | `bool` | — | true/false | false se leitura falhou |
```

### 6.3 Listas

Usar listas para: enumerações sem ordem, passos com ordem.

Lista sem ordem — comportamentos, características:
```markdown
- Inicializa o barramento 1-Wire no pino definido em `PinConfig.h`.
- Configura resolução de 12 bits (0,0625 °C por passo).
- Retorna `valido = false` se nenhum sensor for detectado.
```

Lista numerada — sequências obrigatórias:
```markdown
1. Verificar se `basAtivo == true`.
2. Calcular desvio: `(vibAtual - basMedia) / basStddev`.
3. Comparar com limiar: `basMedia + fatorK * basStddev`.
4. Classificar alarme conforme resultado.
```

### 6.4 Blocos de destaque

```markdown
> **IMPORTANTE:** Texto de atenção operacional — comportamento
> não óbvio que pode causar problema se ignorado.
```

```markdown
> **RESTRICAO:** Limitação técnica do módulo ou do hardware.
```

```markdown
> **DEPENDENCIA:** Este módulo requer que X esteja inicializado
> antes de ser chamado.
```

### 6.5 Diagramas de fluxo em ASCII

Para fluxos simples, usar ASCII dentro de bloco de código:

```
Boot
 |
 +-- Carregar NVS
 |    |
 |    +-- [offset encontrado] --> Aplicar offsets --> Pular calibracao
 |    |
 |    +-- [sem offset]        --> Calibrar automaticamente
 |
 +-- Inicializar sensores
 |
 +-- Iniciar loop principal
```

Para fluxos complexos com muitas ramificações, descrever em texto
estruturado com lista numerada + subníveis.

---

## 7. Exemplos de Código C++

Todo exemplo de código deve:

- Ser compilável no contexto do projeto.
- Representar uso real, não pseudocódigo.
- Ter comentário de contexto quando não for autoexplicativo.
- Usar as convenções de nomenclatura do RAM (Seção 7 do RAM).

### 7.1 Formato obrigatório

````markdown
```cpp
// Contexto: chamado em loop() a cada INTERVALO_LEITURA_MS.
Ds18b20Data dado = ds18b20.ler();
if (!dado.valido) {
  _falhasConsecutivas++;
  // [WARN] registrado pelo CommandHandler
  return;
}
temperatura = dado.temperaturaCelsius;
_falhasConsecutivas = 0;
```
````

### 7.2 Exemplos de uso correto vs. incorreto

Quando o objetivo for mostrar o padrão correto, sempre apresentar
os dois lados:

````markdown
```cpp
// ERRADO — retorna 0.0 em caso de falha, valor ambiguo
float lerTemperatura() {
  float t = sensorTemp.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) return 0.0;
  return t;
}

// CORRETO — struct com flag de validade
Ds18b20Data lerTemperatura() {
  Ds18b20Data dado = { 0.0f, false };
  float t = sensorTemp.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) return dado;
  dado.temperaturaCelsius = t;
  dado.valido = true;
  return dado;
}
```
````

### 7.3 Exemplos de interface de módulo

Ao documentar a interface pública de um módulo, mostrar a assinatura
completa com comentário de retorno:

````markdown
```cpp
// Inicializa o sensor. Deve ser chamado uma vez no setup().
// Retorna false se o sensor nao for detectado no barramento.
bool begin();

// Le a temperatura atual.
// Retorna struct com valido=false se sensor desconectado.
Ds18b20Data ler();

// Retorna o numero de falhas consecutivas desde o ultimo sucesso.
int falhasConsecutivas() const;
```
````

---

## 8. Template — Visão Geral do Sistema

Arquivo: `OVERVIEW.md` na raiz do projeto.
Produzido uma vez. Atualizado a cada mudança arquitetural.

```markdown
# Project SIGMA — Visão Geral Técnica

**Versão do firmware:** X.X.X
**Data:** AAAA-MM-DD
**Plataforma:** ESP32-S3-DevKitC-1
**Framework:** Arduino via PlatformIO

---

## 1. Propósito do Sistema

[Duas a quatro frases descrevendo o que o sistema faz, para quem
e em que contexto industrial é aplicado.]

---

## 2. Hardware

| Componente | Modelo | Interface | Pino(s) |
|------------|--------|-----------|---------|
| Microcontrolador | ESP32-S3 | — | — |
| Acelerômetro | MPU6050 | I2C (400 kHz) | SDA=8, SCL=9 |
| Temperatura | DS18B20 | 1-Wire | GPIO 4 |

---

## 3. Arquitetura em Camadas

[Diagrama ASCII das cinco camadas com descrição de cada uma.]

---

## 4. Estrutura de Diretórios

[Árvore completa do projeto com responsabilidade de cada pasta.]

---

## 5. Fluxo de Boot

[Sequência de inicialização do sistema do power-on até loop().]

---

## 6. Fluxo Principal (loop)

[O que acontece a cada ciclo: leitura, cálculo, alarme, relatório.]

---

## 7. Persistência de Dados (NVS)

| Namespace | Módulo responsável | Dados persistidos |
|-----------|-------------------|-------------------|
| `sigma_cal` | `NvsCalibration` | offsetAX, offsetAY, offsetAZ, calibrado |
| `sigma_sys` | `NvsHorimeter` | horimetro, resetCount, resetMotivo |
| `sigma_cfg` | `NvsConfig` | escalaG, vibAviso, vibCritica |
| `sigma_bas` | `NvsBaseline` | basMedia, basStddev, fatorK, basAtivo, basN |

---

## 8. Comandos Disponíveis

| Comando | Ação | Módulo responsável |
|---------|------|--------------------|
| `C` | Recalibrar acelerômetro | `CalibrationService` |
| `A` | Menu de sensibilidade | `MenuSensitivity` |
| `B` | Coletar baseline | `BaselineService` |
| `K` | Ajustar fator K | `BaselineService` |
| `Z` | Apagar baseline | `NvsBaseline` |
| `H` | Zerar horímetro | `NvsHorimeter` |
| `R` | Reset de configuração | `NvsConfig` |
| `N` | Apagar calibração | `NvsCalibration` |

---

## 9. Dependências Externas

| Biblioteca | Versão | Uso |
|------------|--------|-----|
| Adafruit MPU6050 | X.X.X | Driver do acelerômetro |
| DallasTemperature | X.X.X | Driver do DS18B20 |
| ArduinoJson | X.X.X | Serialização JSON para WebServer |
| ESPAsyncWebServer | X.X.X | HTTP assíncrono |

---

## 10. Roadmap

[Tabela de versões planejadas com status.]
```

---

## 9. Template — Documento de Módulo

Arquivo: `docs/modulos/NomeModulo.md`
Um arquivo por módulo. Produzido junto com o módulo ou ao documentar
código existente.

```markdown
# NomeModulo

**Camada:** [HAL / Driver / Storage / Service / UI / Web]
**Arquivo:** `src/camada/NomeModulo.h` / `NomeModulo.cpp`
**Versão:** X.X.X
**Data:** AAAA-MM-DD

---

## 1. Responsabilidade

[Uma a três frases. O que este módulo faz e — igualmente importante
— o que ele NÃO faz.]

---

## 2. Dependências

| Depende de | Tipo | Motivo |
|------------|------|--------|
| `PinConfig.h` | HAL | Definição dos pinos de hardware |
| `Preferences.h` | Biblioteca ESP | Acesso à NVS |

> **DEPENDENCIA:** Este módulo requer que `Wire.begin()` tenha sido
> chamado antes de `begin()`.

---

## 3. Interface Pública

### 3.1 Structs e tipos

```cpp
// Dado retornado pela leitura do sensor.
// Sempre verificar o campo `valido` antes de usar os dados.
struct NomeData {
  float campo1;  // Descricao e unidade
  float campo2;  // Descricao e unidade
  bool  valido;  // false se leitura falhou
};
```

### 3.2 Métodos

```cpp
// Inicializa o modulo. Chamar uma vez no setup().
// Retorna false se a inicializacao falhar.
bool begin();

// Executa leitura. Retorna struct com valido=false em falha.
NomeData ler();
```

---

## 4. Comportamento Normal

[Descreve o que o módulo faz em condição normal de operação.
Sequência de operações, valores esperados, tempo de execução.]

---

## 5. Comportamento em Falha

| Falha | Classe | Comportamento do módulo |
|-------|--------|------------------------|
| [Descrição da falha] | FATAL / CRITICA / RECUPERAVEL | [O que o módulo faz] |

---

## 6. Configuração

| Parâmetro | Definido em | Valor padrão | Impacto |
|-----------|-------------|-------------|---------|
| `PARAM` | `PinConfig.h` | `valor` | [Efeito de alterar] |

---

## 7. Exemplos de Uso

```cpp
// Inicializacao no setup()
if (!modulo.begin()) {
  // tratar falha fatal
}

// Uso no loop()
NomeData dado = modulo.ler();
if (!dado.valido) {
  // tratar falha de leitura
  return;
}
// usar dado.campo1, dado.campo2
```

---

## 8. Integração com Outros Módulos

[Quem chama este módulo e como. Quem este módulo chama e por quê.]

---

## 9. Limitações Conhecidas

[Restrições técnicas do módulo — não são bugs, são características
do hardware ou da implementação que o integrador precisa saber.]

---

## 10. Histórico de Alterações

| Versão | Data | Alteração |
|--------|------|-----------|
| X.X.X | AAAA-MM-DD | Criação do módulo. |
```

---

## 10. Template — Documento de Fluxo

Arquivo: `docs/fluxos/NomeFluxo.md`
Descreve um fluxo de ponta a ponta que atravessa múltiplos módulos.

```markdown
# Fluxo: [Nome do Fluxo]

**Versão:** X.X.X
**Data:** AAAA-MM-DD
**Gatilho:** [O que inicia este fluxo — boot, comando, evento]
**Módulos envolvidos:** `ModuloA`, `ModuloB`, `ModuloC`

---

## 1. Visão Geral

[Duas a quatro frases descrevendo o fluxo completo do início ao fim.]

---

## 2. Pré-condições

[O que deve ser verdadeiro antes do fluxo começar.]

- `ModuloA` inicializado com sucesso.
- NVS namespace `sigma_xxx` acessível.
- Sensor físico conectado e respondendo.

---

## 3. Sequência de Execução

```
[Gatilho]
    |
    +-- Passo 1: [descricao] — ModuloA
    |       |
    |       +-- [condicao OK]  --> Passo 2
    |       +-- [condicao NOK] --> Fluxo de Falha (ver Secao 5)
    |
    +-- Passo 2: [descricao] — ModuloB
    |
    +-- Passo N: [descricao] — ModuloC
    |
    +-- [Resultado final]
```

---

## 4. Detalhamento por Passo

### Passo 1 — [Nome]

**Módulo:** `NomeModulo`
**Função:** `nomeFuncao()`

[Descrição detalhada do que acontece neste passo, quais dados são
produzidos, quais decisões são tomadas.]

```cpp
// Exemplo real do passo
resultado = modulo.executar(parametro);
```

### Passo 2 — [Nome]

[idem]

---

## 5. Fluxos de Falha

| Passo | Falha | Classe | Ação |
|-------|-------|--------|------|
| Passo 1 | [descrição] | CRITICA | [o que acontece] |
| Passo 2 | [descrição] | FATAL | `reiniciarSistema()` |

---

## 6. Pós-condições

[O que é verdadeiro quando o fluxo termina com sucesso.]

- NVS atualizada com novo valor.
- Flag `xxx = true` em RAM.
- Relatório emitido via Serial.

---

## 7. Saída Serial Esperada

[Transcrição exata da saída serial deste fluxo, conforme o SUS.]

```
  [CMD] Recalibracao solicitada. Monitoramento pausado.

+----------------------------------------------------------+
|  CONFIRMACAO NECESSARIA                                  |
|  [S] = CONFIRMAR  |  [N] = CANCELAR                     |
+----------------------------------------------------------+
```

---

## 8. Histórico de Alterações

| Versão | Data | Alteração |
|--------|------|-----------|
| X.X.X | AAAA-MM-DD | Criação do documento. |
```

---

## 11. Template — Documento de Interface

Arquivo: `docs/interfaces/NomeInterface.md`
Descreve uma interface de comunicação externa do sistema.

```markdown
# Interface: [Nome da Interface]

**Versão:** X.X.X
**Data:** AAAA-MM-DD
**Protocolo:** [Serial / HTTP / MQTT]
**Módulo responsável:** `NomeModulo`

---

## 1. Visão Geral

[O que esta interface expõe, quem a consome e em que contexto.]

---

## 2. Endpoints / Comandos

### [Endpoint ou Comando]

**Tipo:** GET / POST / Comando serial
**Caminho / Tecla:** `/data` ou `C`
**Descrição:** [O que faz]

**Parâmetros de entrada:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| `cmd` | string | Sim | Letra do comando |

**Resposta / Saída:**

```json
{
  "temperatura": 34.5,
  "vibracaoRMS": 0.842,
  "healthScore": 97.3,
  "statusAlarme": "NORMAL",
  "basAtivo": true
}
```

| Campo | Tipo | Unidade | Descrição |
|-------|------|---------|-----------|
| `temperatura` | float | °C | Temperatura atual (1 decimal) |
| `vibracaoRMS` | float | g | Vibração RMS (3 decimais) |
| `healthScore` | float | % | Health Score (1 decimal) |
| `statusAlarme` | string | — | "NORMAL", "AVISO" ou "CRITICO" |
| `basAtivo` | bool | — | true se baseline válido e ativo |

**Códigos de resposta HTTP:**

| Código | Situação |
|--------|----------|
| 200 | Sucesso |
| 400 | JSON inválido ou campo ausente |
| 409 | Sistema em calibração — comando rejeitado |

---

## 3. Limitações

[Taxa máxima de chamadas, tamanho máximo de payload, etc.]

---

## 4. Histórico de Alterações

| Versão | Data | Alteração |
|--------|------|-----------|
| X.X.X | AAAA-MM-DD | Criação do documento. |
```

---

## 12. Template — Documento de Configuração

Arquivo: `docs/config/NomeConfig.md`
Descreve constantes e parâmetros configuráveis do sistema.

```markdown
# Configuração: [Nome]

**Versão:** X.X.X
**Data:** AAAA-MM-DD
**Arquivo de definição:** `src/hal/PinConfig.h` ou `main.cpp`

---

## 1. Constantes do Sistema

| Constante | Valor padrão | Unidade | Faixa válida | Impacto de alterar |
|-----------|-------------|---------|-------------|-------------------|
| `INTERVALO_LEITURA_MS` | 500 | ms | 100–5000 | Frequência do relatório serial |
| `TEMP_AVISO_MAX` | 55,0 | °C | > TEMP_AVISO_MIN | Limiar de alarme AVISO |
| `TEMP_CRITICA` | 65,0 | °C | > TEMP_AVISO_MAX | Limiar de alarme CRITICO |
| `WDT_TIMEOUT_MS` | 10000 | ms | 5000–30000 | Tempo até reset por watchdog |
| `FATOR_K_PADRAO` | 2,0 | — | 1,0–5,0 | Sensibilidade do baseline |

---

## 2. Parâmetros NVS por Namespace

### Namespace `sigma_cfg`

| Chave | Tipo | Valor padrão | Descrição |
|-------|------|-------------|-----------|
| `escalaG` | int | 8 | Escala do acelerômetro em g |
| `vibAviso` | float | 2,0 | Limiar de vibração para AVISO |
| `vibCritica` | float | 4,0 | Limiar de vibração para CRITICO |

---

## 3. Dependências entre Parâmetros

[Restrições de consistência entre parâmetros que o agente deve
respeitar ao alterar valores.]

- `TEMP_AVISO_MIN` < `TEMP_AVISO_MAX` < `TEMP_CRITICA`
- `VIB_AVISO` < `VIB_CRITICA`
- `FATOR_K_MIN` <= `fatorK` <= `FATOR_K_MAX`

---

## 4. Histórico de Alterações

| Versão | Data | Alteração |
|--------|------|-----------|
| X.X.X | AAAA-MM-DD | Criação do documento. |
```

---

## 13. Cenário A — Documentar Código Existente

Quando o código já existe e precisa ser documentado, seguir esta
sequência obrigatória.

### Etapa 1 — Leitura e mapeamento

Antes de escrever uma linha de documentação:

1. Ler o código inteiro do arquivo a documentar.
2. Identificar todas as funções públicas e privadas.
3. Identificar todas as variáveis globais e seus módulos responsáveis.
4. Mapear todas as dependências (includes, chamadas externas).
5. Identificar todos os fluxos de falha tratados e não tratados.

Nunca documentar sem completar esta etapa.

### Etapa 2 — Verificação de comportamento real

Para cada função documentada:

- Ler a implementação completa, não apenas a assinatura.
- Documentar o que o código **faz**, não o que o nome sugere que faz.
- Se o comportamento real diferir do nome, registrar a divergência
  como `> **RESTRICAO:**` no documento.

```
ERRADO:
  "Esta função salva os offsets na NVS."
  [quando o código só salva se calibrado == true]

CORRETO:
  "Salva offsetAX, offsetAY, offsetAZ e o flag `calibrado`
   no namespace `sigma_cal`. Executada sempre que a calibração
   é concluída com sucesso."
```

### Etapa 3 — Ordem de produção dos documentos

Documentar nesta ordem (da camada mais baixa para a mais alta):

1. `OVERVIEW.md` — esqueleto inicial.
2. Módulos HAL.
3. Drivers.
4. Módulos NVS (Storage).
5. Services.
6. UI e Web.
7. Fluxos (após todos os módulos documentados).
8. Interfaces.
9. Configuração.
10. Completar `OVERVIEW.md`.

### Etapa 4 — Validação

Após cada documento produzido:

- Toda função pública está documentada?
- O comportamento em falha está descrito?
- Os exemplos de código são fiéis ao código real?
- As referências cruzadas estão corretas?

---

## 14. Cenário B — Documentar Junto com o Código

Quando o código e a documentação são criados simultaneamente, a
documentação precede o código — ela é o contrato que o código
implementa.

### Ordem obrigatória

```
1. Escrever o documento do módulo (interface, contratos, falhas).
2. Escrever o header (.h) baseado no documento.
3. Escrever a implementação (.cpp) baseada no header.
4. Revisar o documento se a implementação revelou necessidade.
```

Nunca escrever `.cpp` antes do documento do módulo estar completo.

### O documento define a interface

O documento de módulo escrito no Cenário B deve conter a assinatura
completa de todos os métodos públicos antes da implementação começar:

```cpp
// Interface definida no documento ANTES da implementacao:

bool begin();
Ds18b20Data ler();
int falhasConsecutivas() const;
void resetarContadorFalhas();
```

### Atualização após implementação

Se durante a implementação surgirem funções auxiliares privadas
relevantes, parâmetros adicionais ou comportamentos não previstos
no documento original, atualizar o documento imediatamente — antes
de prosseguir com a próxima função.

---

## 15. Manutenção e Versionamento

### 15.1 Quando atualizar um documento

| Evento no código | Documento a atualizar |
|------------------|-----------------------|
| Nova função pública adicionada | `docs/modulos/NomeModulo.md` |
| Comportamento de falha alterado | `docs/modulos/NomeModulo.md` + fluxo afetado |
| Nova constante de configuração | `docs/config/ConstantesDoSistema.md` |
| Novo namespace NVS | `docs/config/NamespacesNVS.md` + `OVERVIEW.md` |
| Novo endpoint WebServer | `docs/interfaces/InterfaceWebServer.md` |
| Novo comando serial | `docs/interfaces/InterfaceSerial.md` + `OVERVIEW.md` |
| Novo fluxo de comportamento | `docs/fluxos/NomeFluxo.md` |
| Mudança arquitetural | `OVERVIEW.md` + módulos afetados |

### 15.2 Controle de versão do documento

Cada documento tem versão própria, independente da versão do firmware.
A versão do documento segue o firmware que o originou, mas pode ter
revisões independentes.

```markdown
| Versão | Data | Alteração |
|--------|------|-----------|
| 0.1.4.1 | 2026-06-26 | Criação do documento. |
| 0.1.4.1-r1 | 2026-06-27 | Corrigida descrição do comportamento em falha. |
```

### 15.3 Documento obsoleto

Se um módulo for removido ou renomeado, seu documento não é apagado.
Recebe no topo:

```markdown
> **OBSOLETO desde vX.X.X — AAAA-MM-DD**
> Este módulo foi substituído por [`NomeNovo`](../modulos/NomeNovo.md).
> Mantido para referência histórica.
```

---

## 16. O Que Nunca Documentar

| Situação | Motivo |
|----------|--------|
| Código comentado (`// float x = ...`) | Não existe no sistema |
| Intenções futuras não implementadas | Pertencem ao roadmap, não à doc técnica |
| Funcionamento interno de bibliotecas externas | Não é responsabilidade deste projeto |
| Sintaxe C++ genérica | Não é tutorial de linguagem |
| Decisões revertidas | Causam confusão — remover sem registro |
| Comportamento suposto sem verificação no código | Viola R01 |

---

## 17. Checklist do Agente

Antes de entregar qualquer documento, verificar cada item.
Se qualquer resposta for "não", corrigir antes de finalizar.

```
ESTRUTURA
[ ] O arquivo está no caminho correto conforme Seção 4?
[ ] O template correto foi usado conforme o tipo (Seção 3)?
[ ] O documento tem seção de Versão e Data?
[ ] Documentos com mais de 5 seções têm índice/sumário?
[ ] Todas as referências cruzadas são links relativos válidos?

CONTEÚDO
[ ] A responsabilidade do módulo está em uma a três frases?
[ ] O que o módulo NÃO faz está explícito?
[ ] Toda função pública está documentada?
[ ] O comportamento em falha está descrito para cada módulo?
[ ] Toda grandeza tem unidade e faixa válida?
[ ] Nenhuma frase usa "pode", "talvez" ou "geralmente"?

EXEMPLOS DE CÓDIGO
[ ] Todo exemplo compila no contexto do projeto?
[ ] Exemplos mostram uso real, não pseudocódigo?
[ ] Pares correto/errado usados onde o padrão não é óbvio?
[ ] Nomes seguem as convenções do RAM (Seção 7)?

IDIOMA E FORMATAÇÃO
[ ] Todo texto em português (exceto termos da Seção 5.2)?
[ ] Nomes de funções/variáveis em inline code (`nome`)?
[ ] Tabelas usadas para parâmetros, nunca para texto corrido?
[ ] Blocos de destaque usados para restrições e dependências?

CENÁRIO A (código existente)
[ ] O comportamento documentado foi verificado no código real?
[ ] Divergências entre nome e comportamento estão anotadas?
[ ] A ordem de produção da Seção 13 foi seguida?

CENÁRIO B (código novo)
[ ] O documento foi escrito antes da implementação?
[ ] A interface pública está completamente definida no documento?
[ ] O documento foi revisado após a implementação?

MANUTENÇÃO
[ ] A tabela de histórico de alterações foi atualizada?
[ ] Módulos removidos têm marcação de obsoleto?
[ ] Todos os documentos afetados pela mudança foram atualizados?
```

---

*Fim do documento. Versão 1.0.0 — Project SIGMA Documentation Agent Manual.*
