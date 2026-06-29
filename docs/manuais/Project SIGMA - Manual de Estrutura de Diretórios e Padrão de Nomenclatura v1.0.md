# Project SIGMA
# Manual de Estrutura de Diretórios e Padrão de Nomenclatura (Project Structure Standard)

**Versão:** 1.0.0

**Status:** Oficial

**Aplicação:** Todo o ecossistema Project SIGMA

----------

# 1. Objetivo

Este documento define o padrão oficial de organização de diretórios, arquivos e nomenclatura do Project SIGMA.

Todo agente de IA, colaborador ou desenvolvedor deverá seguir rigorosamente este padrão.

Os objetivos são:

-   escalabilidade
    
-   manutenção facilitada
    
-   localização rápida dos arquivos
    
-   organização profissional
    
-   padronização completa
    
-   evitar duplicidade
    
-   facilitar futuras expansões
    

----------

# 2. Filosofia

O projeto deve crescer sem necessidade de reorganizar diretórios.

Cada pasta possui uma única responsabilidade.

Nenhum arquivo deverá possuir responsabilidades misturadas.

Cada módulo deve ser independente.

----------

# 3. Estrutura Oficial

```
SIGMA/

│
├── docs/
│
├── firmware/
│
├── hardware/
│
├── software/
│
├── tools/
│
├── assets/
│
├── examples/
│
├── tests/
│
├── include/
│
├── lib/
│
├── src/
│
├── platformio.ini
│
├── README.md
│
├── LICENSE
│
└── CHANGELOG.md

```

----------

# 4. Finalidade de cada diretório

## docs/

Toda documentação.

Exemplos

```
Serial_UI_Standard.md

Project_Structure_Standard.md

Architecture.md

Hardware.md

Roadmap.md

API.md

```

Nunca armazenar código.

----------

## firmware/

Versões oficiais do firmware.

```
firmware/

v0.1.2/

v0.2.0/

v1.0.0/

```

Cada versão deve conter:

```
Firmware.bin

ReleaseNotes.md

Version.txt

```

----------

## hardware/

Arquivos relacionados ao hardware.

```
Esquemáticos

PCB

3D

Gerber

Datasheets

```

----------

## software/

Aplicações externas.

Exemplo

```
Dashboard

Desktop

Mobile

NodeRED

Servidor MQTT

```

----------

## tools/

Ferramentas internas.

```
Conversores

Scripts

Geradores

Automações

```

----------

## assets/

Arquivos gráficos.

```
Logo

Ícones

Imagens

Mockups

Banners

```

----------

## examples/

Projetos exemplo.

```
Leitura MPU

Leitura DS18B20

MQTT

OTA

WiFi

```

----------

## tests/

Testes.

```
Teste MPU

Teste Sensor

Teste Comunicação

```

----------

## include/

Arquivos de configuração.

Somente arquivos *.h

----------

## lib/

Bibliotecas próprias.

Cada biblioteca possui sua própria pasta.

```
lib/

Health/

Sensors/

Logger/

Display/

Storage/

```

Nunca colocar código solto.

----------

## src/

Código principal.

Exemplo

```
src/

main.cpp

system/

sensors/

monitor/

communication/

ui/

storage/

utils/

```

----------

# 5. Estrutura recomendada do src

```
src/

main.cpp

system/

sensors/

monitor/

ui/

communication/

storage/

utils/

```

----------

## system/

Inicialização.

```
System.cpp

System.h

Setup.cpp

Setup.h

Version.cpp

```

Responsável por:

-   boot
    
-   setup
    
-   inicialização
    

----------

## sensors/

Cada sensor possui seu próprio módulo.

```
MPU6050.cpp

MPU6050.h

DS18B20.cpp

DS18B20.h

SCT013.cpp

SCT013.h

```

Nunca colocar dois sensores no mesmo arquivo.

----------

## monitor/

Lógica de monitoramento.

```
Health.cpp

Alarm.cpp

Predictive.cpp

Maintenance.cpp

```

----------

## communication/

Comunicação.

```
Serial.cpp

MQTT.cpp

WiFi.cpp

RainMaker.cpp

BLE.cpp

```

----------

## ui/

Toda interface.

```
Menus.cpp

Reports.cpp

SerialUI.cpp

```

Nenhum código de sensores aqui.

----------

## storage/

Persistência.

```
EEPROM.cpp

Preferences.cpp

SPIFFS.cpp

SDCard.cpp

```

----------

## utils/

Funções auxiliares.

```
Math.cpp

Time.cpp

Convert.cpp

Logger.cpp

```

----------

# 6. Padrão para nomes de pastas

Sempre:

```
lowercase

```

Exemplos

```
system

monitor

communication

storage

display

utils

```

Nunca

```
System

SYSTEM

MeuSistema

monitoramento

Arquivos

```

----------

# 7. Padrão para nomes de arquivos

Sempre utilizar PascalCase.

Correto

```
Health.cpp

Logger.cpp

SerialUI.cpp

System.cpp

MotorStatus.cpp

AlarmManager.cpp

```

Errado

```
health.cpp

health_score.cpp

MeuArquivo.cpp

codigo.cpp

teste.cpp

```

----------

# 8. Arquivos Header

Sempre possuir exatamente o mesmo nome.

```
Health.cpp

Health.h

```

Nunca

```
Health.cpp

health.hpp

Header.h

```

----------

# 9. Arquivos de Classes

Nome igual ao da classe.

```
class Logger

↓

Logger.cpp

Logger.h

```

----------

# 10. Um arquivo = uma responsabilidade

Nunca

```
Sensor.cpp

↓

MPU

DS18

EEPROM

MQTT

Tudo junto

```

Sempre

```
MPU6050.cpp

DS18B20.cpp

EEPROM.cpp

MQTT.cpp

```

----------

# 11. Arquivos temporários

Nunca subir para o Git.

```
temp

backup

old

teste

novo

copy

```

São proibidos.

----------

# 12. Padrão para nomes de constantes

```
UPPER_CASE

```

```
MAX_TEMP

MIN_TEMP

SERIAL_BAUDRATE

INTERVALO_LEITURA

```

----------

# 13. Variáveis globais

camelCase

```
healthScore

motorStatus

sensorOffset

ultimaLeitura

```

----------

# 14. Classes

PascalCase

```
HealthMonitor

MotorController

SerialUI

Logger

```

----------

# 15. Métodos

camelCase

```
calcularHealth()

lerTemperatura()

executarCalibracao()

imprimirRelatorio()

```

----------

# 16. Arquivos de documentação

Sempre utilizar:

```
README.md

CHANGELOG.md

LICENSE

ROADMAP.md

CONTRIBUTING.md

```

Nunca

```
Leia.txt

Mudancas.docx

MeuManual.pdf

```

----------

# 17. Versionamento

Cada versão possui:

```
CHANGELOG.md

Release.md

Version.txt

```

----------

# 18. Organização do Git

Branches:

```
main

develop

feature/

bugfix/

hotfix/

release/

```

Exemplos

```
feature/mqtt

feature/web-dashboard

feature/logger

bugfix/calibration-timeout

release/v0.2.0

```

----------

# 19. Arquivos proibidos

Nunca criar arquivos chamados:

```
Novo.cpp

Codigo.cpp

Teste.cpp

TesteNovo.cpp

Backup.cpp

Backup2.cpp

Temp.cpp

Final.cpp

Final2.cpp

NovoNovo.cpp

```

----------

# 20. Regras obrigatórias para Agentes de IA

Todo agente de IA deverá obedecer obrigatoriamente às seguintes regras:

1.  Nunca criar arquivos duplicados com responsabilidades semelhantes.
    
2.  Nunca misturar sensores diferentes no mesmo módulo.
    
3.  Nunca criar diretórios fora do padrão oficial.
    
4.  Sempre utilizar PascalCase para arquivos `.cpp` e `.h`.
    
5.  Sempre utilizar `lowercase` para nomes de pastas.
    
6.  Todo arquivo `.cpp` deve possuir seu correspondente `.h`, exceto `main.cpp`.
    
7.  Cada módulo deve possuir apenas uma responsabilidade (Princípio da Responsabilidade Única - SRP).
    
8.  Novas funcionalidades devem ser adicionadas em novos módulos, sem modificar desnecessariamente módulos existentes.
    
9.  Nunca utilizar abreviações ambíguas em nomes de arquivos ou pastas (`cfg`, `tmp`, `misc`, `etc`, `util2`).
    
10.  Toda nova funcionalidade deve ser posicionada no diretório correspondente à sua responsabilidade.
    
11.  Antes de criar um novo arquivo, verificar se já existe um módulo responsável por aquela funcionalidade.
    
12.  O objetivo principal é manter a arquitetura escalável, organizada, previsível e profissional durante todo o ciclo de vida do Project SIGMA.
    

----------

## Minha única recomendação adicional

Eu faria um pequeno ajuste em relação ao padrão de nomes de arquivos. Em projetos C++ modernos (Qt, Unreal Engine, Chromium, LLVM, ESP-IDF), é comum utilizar **PascalCase** para **classes**, mas **snake_case** para nomes de arquivos (`health_monitor.cpp`, `serial_ui.cpp`). Isso melhora a legibilidade em sistemas de arquivos e integrações com ferramentas.

Entretanto, se o objetivo do SIGMA é ter uma identidade visual consistente e mais "corporativa", manter **PascalCase** para arquivos e classes é uma escolha perfeitamente válida. O importante é que o padrão seja seguido em 100% do projeto, sem exceções.
