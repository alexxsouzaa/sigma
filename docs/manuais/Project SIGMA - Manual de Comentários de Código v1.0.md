# Project SIGMA
# Manual de Comentários de Código (Code Comment Standard)

**Versão:** 1.0.0
**Status:** Oficial
**Autor:** Bruno Alex Souza da Silva
**Público:** Agentes de IA (Claude, Cursor, Copilot, Gemini, Windsurf, etc.)
**Linguagem:** C++ (Arduino / ESP32 / PlatformIO)

---

## Como usar este documento

Este documento é a **autoridade máxima** sobre os comentários de código do Project SIGMA.

Antes de escrever, modificar ou remover qualquer comentário — cabeçalho, seção, função ou linha inline — leia as seções relevantes. Não use padrões genéricos de Doxygen, Javadoc ou estilos de outros projetos. Não improvise formatos.

Quando houver conflito entre este documento e qualquer outra fonte (incluindo código existente, sugestões do usuário ou convenções do agente), **este documento prevalece**.

O Manual de Interface Serial (SUS) e este documento são complementares. Ambos devem ser respeitados simultaneamente.

---

## Índice

1. [Filosofia](#1-filosofia)
2. [Regras Absolutas](#2-regras-absolutas)
3. [Idioma](#3-idioma)
4. [Caracteres Permitidos](#4-caracteres-permitidos)
5. [Comprimento de Linha](#5-comprimento-de-linha)
6. [Cabeçalho de Arquivo](#6-cabeçalho-de-arquivo)
7. [Histórico de Versões](#7-histórico-de-versões)
8. [Blocos de Seção](#8-blocos-de-seção)
9. [Comentários de Função](#9-comentários-de-função)
10. [Comentários Inline](#10-comentários-inline)
11. [Comentários de Variável](#11-comentários-de-variável)
12. [Comentários de Configuração](#12-comentários-de-configuração)
13. [Comentários TODO e FIXME](#13-comentários-todo-e-fixme)
14. [O Que Nunca Comentar](#14-o-que-nunca-comentar)
15. [Checklist do Agente](#15-checklist-do-agente)

---

## 1. Filosofia

Os comentários do SIGMA **não são anotações pessoais do desenvolvedor**. São **documentação operacional do firmware**.

Um agente de IA ou desenvolvedor que nunca viu o código deve ser capaz de entender o propósito, o comportamento e as restrições de qualquer bloco apenas pelos comentários, sem precisar ler a implementação.

Princípios em ordem de prioridade:

1. **Precisão** — o comentário descreve exatamente o que o código faz, não o que se pretendia que fizesse.
2. **Utilidade** — todo comentário justifica sua existência. Se não acrescenta informação, não existe.
3. **Consistência** — o mesmo tipo de elemento sempre recebe o mesmo formato de comentário.
4. **Manutenibilidade** — comentários desatualizados são piores que a ausência de comentários.

---

## 2. Regras Absolutas

Estas regras nunca têm exceção.

| # | Regra |
|---|-------|
| R01 | Nunca usar emoji, Unicode decorativo ou caracteres fora do ASCII imprimível |
| R02 | Nunca usar TAB dentro de comentários — somente espaços |
| R03 | Nunca ultrapassar 60 caracteres por linha de comentário |
| R04 | Nunca deixar código comentado ("dead code") no arquivo final |
| R05 | Nunca escrever comentários que apenas repetem o código em palavras |
| R06 | Nunca usar `//` no final de linhas longas de código (comentário inline) se ultrapassar 60 chars |
| R07 | Nunca criar seções sem o separador padrão definido na Seção 8 |
| R08 | Nunca modificar o cabeçalho de arquivo sem atualizar Versao e data |
| R09 | Nunca misturar português e inglês no mesmo comentário |
| R10 | Todo comentário de função deve existir — nenhuma função pública sem comentário |

---

## 3. Idioma

Todo comentário deve estar em **Português do Brasil**.

```
ERRADO               CORRETO
-------------------  -------------------
// Read sensor       // Le o sensor
// Calculate offset  // Calcula o offset
// Save to NVS       // Salva na NVS
// Warning threshold // Limiar de aviso
```

**Termos técnicos aceitos em inglês nos comentários:**
`offset`, `baseline`, `health score`, `watchdog`, `NVS`, `RMS`, `sigma`, `baud`,
`setup`, `loop`, `callback`, `buffer`, `stack`, `heap`, `tick`, `flag`, `struct`,
`namespace`, `task`, `core`, `timer`.

Nomes de funções, variáveis e constantes do código podem aparecer em inglês dentro
de comentários quando referenciados diretamente (ex: `// Chama calcularVibracaoRMS()`).

---

## 4. Caracteres Permitidos

Somente ASCII imprimível. Os mesmos caracteres estruturais do SUS se aplicam aqui:

```
Permitidos em bordas e separadores:
  =   -   |   +   /   *   #   >   <   [   ]   (   )
```

O marcador de comentário C++ é `//` para linhas simples e `/* */` para blocos
estruturais (cabeçalho de arquivo e histórico de versões apenas).

---

## 5. Comprimento de Linha

**Limite absoluto: 60 caracteres por linha**, contando o marcador `//` e o espaço
após ele.

```
// Verificando se o sensor esta respondendo.   <- 47 chars OK
// Verifica se o sensor DS18B20 esta presente e respondendo ao barramento. <- ERRADO
```

Quando a descrição for longa, quebre em múltiplas linhas:

```cpp
// Verifica se o sensor DS18B20 esta presente
// e respondendo ao barramento 1-Wire.
```

---

## 6. Cabeçalho de Arquivo

Todo arquivo `.cpp`, `.h` ou `.ino` do projeto começa com este bloco.
Nenhum arquivo sem cabeçalho.

### Formato obrigatório

```cpp
// =============================================================
//  Project SIGMA
//  Arquivo    : nome_do_arquivo.cpp
//  Descricao  : Descricao resumida em uma linha.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : X.X.X
//  Data       : AAAA-MM-DD
// =============================================================
```

### Regras do cabeçalho

- O separador usa `=` com exatamente 61 caracteres (incluindo `//` e espaço).
- Todos os campos são obrigatórios.
- `Descricao` deve caber em uma linha. Se precisar de mais, use duas linhas com
  o rótulo vazio na segunda: `//             continuacao da descricao.`
- `Versao` deve coincidir com a versão do firmware declarada no ROADMAP.
- `Data` no formato `AAAA-MM-DD`.
- Nenhum campo pode ficar em branco. Se não aplicável, usar `N/A`.

### Exemplo real

```cpp
// =============================================================
//  Project SIGMA
//  Arquivo    : sigma_webserver.h
//  Descricao  : Modulo WebServer — endpoints /data e /cmd.
//  Autor      : Bruno Alex Souza da Silva
//  Plataforma : ESP32-S3-DevKitC-1
//  Framework  : Arduino via PlatformIO
//  Versao     : 0.1.4.1
//  Data       : 2026-06-26
// =============================================================
```

---

## 7. Histórico de Versões

O histórico de versões fica no arquivo principal do firmware (`.ino` ou `main.cpp`),
logo abaixo do cabeçalho de arquivo. Arquivos auxiliares (`.h`, módulos) não
repetem o histórico completo — apenas indicam sua própria versão no cabeçalho.

### Formato obrigatório

```cpp
// =============================================================
//  ROADMAP
// -------------------------------------------------------------
//  vX.X.x  Nome da serie          [STATUS]
//  vX.X.x  Nome da serie          [STATUS]
// =============================================================
//
//  HISTORICO DE VERSOES
// =============================================================
//
//  SERIE vX.X.x — Nome da Serie
//  [vX.X.X]  Titulo resumido da versao            [STATUS]
//  Autor  : Nome
//  Data   : AAAA-MM-DD
//  Status : Estavel | Em desenvolvimento | Descontinuado
//  Resumo :
//    - Item 1 do que foi feito nesta versao.
//    - Item 2 do que foi feito nesta versao.
//
```

### Status permitidos no ROADMAP

| Status | Significado |
|--------|-------------|
| `[EM ANDAMENTO]` | Serie atual em desenvolvimento |
| `[PLANNED]` | Planejado, nao iniciado |
| `[RELEASED]` | Versao publicada e estavel |
| `[CLOSED]` | Serie encerrada |
| `[CURRENT]` | Versao atualmente em uso |
| `[DEPRECATED]` | Descontinuado |

### Regras do histórico

- Versões mais recentes ficam no final da série, não no topo.
- Cada versão tem obrigatoriamente: tag `[vX.X.X]`, Autor, Data, Status e Resumo.
- O Resumo usa marcadores `- ` (traço + espaço). Máximo 55 chars por item.
- Nunca apagar entradas antigas — o histórico é cumulativo.
- A versão `[CURRENT]` é sempre a última entrada do arquivo.

---

## 8. Blocos de Seção

Blocos de seção separam grupos de funções relacionadas dentro de um arquivo.
Toda seção tem um separador superior, um título e um separador inferior.

### Formato obrigatório

```cpp
// =============================================================
//  NOME DA SECAO EM MAIUSCULAS
// =============================================================
```

### Variante com descrição

Quando a seção precisa de contexto adicional:

```cpp
// =============================================================
//  NVS — CALIBRACAO (sigma_cal)
// -------------------------------------------------------------
//  Namespace: sigma_cal
//  Chaves   : offsetAX, offsetAY, offsetAZ, calibrado
// =============================================================
```

### Regras de seção

- Título sempre em MAIUSCULAS.
- Separador usa `=` com 61 caracteres (incluindo `//` e espaço).
- Descrição opcional usa `-` no separador intermediário.
- Uma linha em branco antes e depois do bloco de seção.
- Seções são obrigatórias quando o arquivo tem mais de 3 grupos funcionais distintos.

### Seções padrão do firmware SIGMA

Use exatamente estes nomes para as seções já existentes:

```
DEFINICAO DE PINOS
CONFIGURACOES DO SISTEMA
FORWARD DECLARATIONS
INSTANCIAS
VARIAVEIS DE CONTROLE
OFFSETS DE CALIBRACAO
CONFIGURACAO DE ESCALA
WATCHDOG E RESET
BASELINE DE VIBRACAO
NVS — CALIBRACAO (sigma_cal)
NVS — HORIMETRO (sigma_sys)
NVS — CONFIGURACAO (sigma_cfg)
NVS — BASELINE (sigma_bas)
SETUP
LOOP
```

---

## 9. Comentários de Função

Toda função não trivial recebe um bloco de comentário imediatamente acima de
sua assinatura. Funções triviais (getters, setters de uma linha) podem omitir
o bloco, mas nunca funções públicas ou de mais de 10 linhas.

### Formato obrigatório — função simples

```cpp
// =============================================================
//  FUNCAO: nomeDaFuncao
//  Descricao resumida do proposito da funcao.
// =============================================================
void nomeDaFuncao() {
```

### Formato completo — função com parâmetros, retorno e observações

```cpp
// =============================================================
//  FUNCAO: nomeDaFuncao
//  Descricao resumida do proposito da funcao.
// -------------------------------------------------------------
//  Parametros:
//    param1  : Descricao do parametro 1.
//    param2  : Descricao do parametro 2.
//  Retorno  : Descricao do valor retornado.
//             false se falhar, true se OK.
//  Observ.  : Informacao importante sobre uso ou restricao.
//  Chama    : outraFuncao(), Serial.println()
//  Chamado  : loop(), menuCalibracaoSetup()
// =============================================================
```

### Campos do comentário de função

| Campo | Obrigatorio | Uso |
|-------|-------------|-----|
| `FUNCAO:` | Sim | Nome exato da função |
| Descrição | Sim | Uma a três linhas explicando o propósito |
| `Parametros:` | Se houver | Um por linha, alinhados |
| `Retorno:` | Se nao for void | O que e retornado e quando |
| `Observ.:` | Se necessario | Restricoes, efeitos colaterais, dependencias |
| `Chama:` | Opcional | Funcoes relevantes que esta chama |
| `Chamado:` | Opcional | De onde esta funcao e chamada |

### Regras do comentário de função

- `FUNCAO:` sempre em maiúsculas.
- O nome da função deve ser idêntico ao da assinatura C++.
- Nenhuma linha ultrapassa 60 caracteres.
- O separador usa `=` com 61 caracteres.
- Se houver seção adicional (Parametros, Retorno), usar `-` no separador intermediário.
- Não duplicar informação óbvia: `// Retorno: void` é proibido.

### Exemplos reais

```cpp
// =============================================================
//  FUNCAO: calcularVibracaoRMS
//  Coleta 50 leituras do MPU6050 e retorna o valor RMS
//  da aceleracao resultante em g, com offsets aplicados.
// -------------------------------------------------------------
//  Retorno  : Valor RMS em g (float).
//  Observ.  : Bloqueia por ~100 ms (50 x delay(2)).
//             Nao emite logs — chamada a cada ciclo do loop.
// =============================================================
float calcularVibracaoRMS() {
```

```cpp
// =============================================================
//  FUNCAO: reiniciarSistema
//  Salva horimetro e motivo na NVS, aguarda 2s e executa
//  esp_restart() para reinicio limpo do firmware.
// -------------------------------------------------------------
//  Parametros:
//    motivo  : String descrevendo a causa do reset.
//  Observ.  : Nao retorna. Execucao encerrada apos chamada.
//  Chama    : salvarHorimetroNVS(), salvarResetNVS()
// =============================================================
void reiniciarSistema(String motivo) {
```

```cpp
// =============================================================
//  FUNCAO: carregarBaselineNVS
//  Le baseline de vibracao do namespace sigma_bas na NVS.
//  Preenche basMedia, basStddev, fatorK, basAtivo, basN.
// -------------------------------------------------------------
//  Retorno  : true  — baseline valido encontrado e carregado.
//             false — nenhum baseline na NVS.
// =============================================================
bool carregarBaselineNVS() {
```

---

## 10. Comentários Inline

Comentários inline ficam na mesma linha do código ou na linha imediatamente
acima, dependendo do contexto.

### Quando usar comentário na mesma linha

Use `//` no final da linha quando o comentário é curto e a linha de código
cabe dentro do limite de 60 caracteres total.

```cpp
falhasDS18B20 = 0;            // Reseta contador apos leitura OK
esp_task_wdt_reset();         // Alimenta o watchdog de hardware
calibrado = true;             // Marca calibracao como concluida
```

### Quando usar comentário na linha acima

Use quando a linha de código já é longa, ou quando o comentário precisa de
mais de uma linha.

```cpp
// Protege contra variancia negativa por erro de ponto flutuante.
if (variancia < 0.0) variancia = 0.0;

// Calcula desvio atual em numero de sigmas acima do baseline.
// Usado no relatorio e no Health Score.
vibDesvio = (vib - basMedia) / basStddev;
```

### Alinhamento de comentários inline em blocos

Quando múltiplas linhas consecutivas têm comentários inline, alinhe os `//`
na mesma coluna:

```cpp
float offsetAX = 0.0;   // Offset eixo X em g
float offsetAY = 0.0;   // Offset eixo Y em g
float offsetAZ = 0.0;   // Offset eixo Z em g (inclui ~1g da gravidade)
bool  calibrado = false; // true quando calibracao foi executada
```

### O que o comentário inline deve dizer

O comentário inline explica o **porquê**, não o **o quê**. O código já diz
o que está sendo feito; o comentário acrescenta contexto.

```cpp
ERRADO:
  delay(2000);  // Aguarda 2 segundos

CORRETO:
  delay(2000);  // Tempo para flush da Serial antes do restart
```

```cpp
ERRADO:
  horasBase = 0.0;  // Zera horasBase

CORRETO:
  horasBase = 0.0;  // Reinicia contagem a partir do novo boot
```

---

## 11. Comentários de Variável

Grupos de variáveis relacionadas recebem um comentário de grupo acima do bloco,
e comentários individuais inline quando o nome não é autoexplicativo.

### Formato de grupo

```cpp
// -------------------------
//  Baseline de vibracao
// -------------------------
float basMedia  = 0.0;   // Media RMS do baseline (g)
float basStddev = 0.0;   // Desvio padrao do baseline (g)
float fatorK    = FATOR_K_PADRAO; // Multiplicador de sensibilidade
bool  basAtivo  = false; // true quando baseline valido esta ativo
int   basN      = 0;     // Numero de amostras do baseline
float vibDesvio = 0.0;   // Desvio atual em sigmas acima do baseline
```

### Regras de variável

- O separador de grupo usa `-` com 27 caracteres (incluindo `//` e espaço).
- Comentários inline de variável explicam a unidade e o significado, não o tipo.
- Variáveis com nomes autoexplicativos e tipo óbvio podem omitir comentário inline.
- Sempre indicar a unidade quando aplicável: `// (g)`, `// (ms)`, `// (h)`.

---

## 12. Comentários de Configuração

Constantes e defines de configuração recebem comentário inline explicando o
impacto da mudança, não apenas o significado.

### Formato

```cpp
// -------------------------
//  Configuracoes do sistema
// -------------------------
#define INTERVALO_LEITURA_MS        500    // Ciclo do loop principal
#define INTERVALO_MANUTENCAO_H      500.0  // Horas ate proxima manutencao
#define INTERVALO_SAVE_HORIMETRO_MS 300000UL // Salva NVS a cada 5 min
#define TEMP_AVISO_MIN              25.0   // Abaixo: motor frio
#define TEMP_AVISO_MAX              55.0   // Acima: entra em AVISO
#define TEMP_CRITICA                65.0   // Acima: entra em CRITICO
#define WDT_TIMEOUT_MS              10000  // Reset se loop travar
#define MAX_FALHAS_DS18B20          5      // Resets apos N falhas
#define NUM_AMOSTRAS_BASELINE       300    // Amostras para baseline
#define FATOR_K_PADRAO              2.0    // Sensibilidade: 2-sigma
#define FATOR_K_MIN                 1.0    // Limite inferior do fator K
#define FATOR_K_MAX                 5.0    // Limite superior do fator K
```

### Regras de configuração

- Toda constante de configuração tem comentário inline. Sem exceção.
- O comentário explica o **efeito** de mudar o valor, não o nome.
- Unidades sempre presentes: `ms`, `h`, `g`, `oC`.

---

## 13. Comentários TODO e FIXME

Usados apenas durante desenvolvimento ativo. **Proibidos em versões marcadas
como `[RELEASED]` ou `[CURRENT]`.**

### Formato obrigatório

```cpp
// TODO  [vX.X.x] : Descricao clara do que precisa ser feito.
// FIXME [vX.X.x] : Descricao do problema e comportamento esperado.
```

### Regras de TODO/FIXME

- Sempre incluir a versão alvo entre colchetes.
- Sempre descrever a ação necessária, não apenas o problema.
- Remover antes de marcar a versão como `[RELEASED]`.
- Nunca usar `// HACK`, `// WORKAROUND`, `// TEMP` sem prazo e versão.

### Exemplos

```cpp
// TODO  [v0.2.0] : Substituir Serial por publicacao MQTT.
// FIXME [v0.1.5] : menuSensibilidade() nao reseta basAtivo
//                  ao receber escala identica a atual.
```

---

## 14. O Que Nunca Comentar

O agente não deve criar comentários para os seguintes casos:

| Situação | Motivo |
|----------|--------|
| Código autoexplicativo | `i++; // incrementa i` é ruído |
| Cabeçalhos de bibliotecas externas | Não pertencem ao projeto |
| Código comentado (`// float x = ...`) | Proibido pela R04 |
| Explicações do compilador ou sintaxe C++ | Não é tutorial |
| Comentários de fechamento de bloco (`} // fim do if`) | Indica função grande demais |
| Assinaturas ou créditos dentro de funções | Pertencem ao cabeçalho |
| Comentários de data de modificação inline | Pertencem ao histórico de versões |

---

## 15. Checklist do Agente

Antes de submeter qualquer modificação de código, verificar:

```
[ ] Todo arquivo tem cabeçalho completo com Versao e Data atualizados?
[ ] O histórico de versões foi atualizado com a nova entrada?
[ ] Toda função nao trivial tem bloco de comentário no formato correto?
[ ] Nenhum comentário ultrapassa 60 caracteres por linha?
[ ] Nenhum TAB foi usado dentro de comentários?
[ ] Nenhum emoji ou Unicode foi introduzido?
[ ] O idioma é exclusivamente português (exceto termos da Seção 3)?
[ ] Comentários inline explicam o PORQUÊ, nao o QUÊ?
[ ] Variáveis de configuração têm comentário inline com unidade?
[ ] Nenhum código comentado ("dead code") permaneceu?
[ ] TODO/FIXME têm versão alvo e foram removidos se a versão é RELEASED?
[ ] Separadores de seção usam o formato exato da Seção 8?
[ ] Nomes de seção coincidem com os nomes padrão da Seção 8?
```

Se qualquer item falhar, corrigir antes de finalizar.

---

*Fim do documento. Versão 1.0.0 — Project SIGMA Code Comment Standard.*
