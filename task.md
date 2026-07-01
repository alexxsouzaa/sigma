# Project SIGMA — Plano Mestre de Implementação

Versão: 1.0.0

Status: Oficial

---

# Objetivo

Este documento define a ordem oficial de desenvolvimento do Project SIGMA.

Cada tarefa representa um passo específico da evolução do firmware.

Nenhuma tarefa deve iniciar antes da conclusão da anterior, salvo quando explicitamente indicado.

Cada etapa deve preservar compatibilidade com versões anteriores.

---

# FASE 1 — Foundation (Firmware Core)

Objetivo:

Concluir toda infraestrutura necessária para futuras funcionalidades.

---

## T001 — Arquitetura Base

Status: Concluído

Objetivo

Consolidar a arquitetura em camadas.

Entregas

* HAL
* Drivers
* Storage
* Services
* Application

---

## T002 — Sistema de Configuração

Status: Concluído

Objetivo

Centralizar todas as constantes globais.

Entregas

* Version.h
* PinConfig.h
* Configurações globais

---

## T003 — Drivers

Status: Concluído

Objetivo

Isolar completamente o hardware.

Entregas

* DS18B20 Driver
* MPU6050 Driver

---

## T004 — Persistência NVS

Status: Concluído

Objetivo

Persistir todos os dados importantes.

Entregas

* Calibração
* Horímetro
* Configuração
* Baseline

---

## T005 — Serviços

Status: Concluído

Objetivo

Separar regras de negócio.

Entregas

* AlarmService
* HealthService

---

## T006 — Interface Serial

Status: Concluído

Objetivo

Padronizar toda interface SUS.

---

## T007 — Analytics Buffer

Status: Concluído

Objetivo

Implementar histórico circular em RAM.

---

## T008 — Rolling Statistics

Status: Concluído

Objetivo

Calcular estatísticas continuamente.

---

## T009 — Trend Analysis

Status: Concluído

Objetivo

Detectar tendências.

---

## T010 — Analytics API

Status: Concluído

Objetivo

Criar API única para acesso aos dados analíticos.

Entregas

* AnalyticsEngine
* AnalyticsResult
* Métodos públicos
* Interface única para futuras integrações

---

# FASE 2 — Data Quality

Objetivo

Garantir confiabilidade dos dados.

---

## T011 — Filtro Digital

Status: Concluído

Implementar filtros para redução de ruído.

Entregas

* Média móvel
* EMA
* Configuração por firmware

---

## T012 — Detecção de Outliers

Status: Concluído

Objetivo

Eliminar leituras inválidas.

Entregas

* Limites físicos (temperatura -55..125°C, vibração 0..escalaG)
* Z-Score com algoritmo online de Welford (O(1) por amostra)
* Rejeição automática com fallback para 0.0f
* Limiar Z-Score configurável (padrão 3.5 sigma)
* Bootstrap de 10 amostras antes de ativar Z-Score

---

## T013 — Qualidade dos Sensores

Status: Concluído

Objetivo

Criar índice de confiança da leitura.

Entregas

* Sensor Health
* Sensor Quality Score

---

# FASE 3 — Event Engine

Objetivo

Transformar leituras em eventos.

---

## T014 — Event Manager

Status: Concluído

Objetivo

Criar sistema de eventos internos.

Entregas

* EventTypes.h: 10 tipos de evento (leituras, alarmes, outliers, calibracao)
* EventManager.h/.cpp: barramento pub/sub estatico sem alocacao dinamica
* Disparo sincrono (disparar) e assincrono (enfileirar + processarFila)
* MAX_ASSINANTES = 16, MAX_EVENTOS_FILA = 8
* Callback de log serial via SIGMA_DEBUG_EVENTOS
* Integrado em main.cpp: eventos disparados a cada leitura/processamento

---

## T015 — Historico de Eventos

Status: Concluido

Objetivo

Registrar eventos em memoria.

Entregas

* EventHistory.h/.cpp: buffer circular de 40 eventos em RAM
* Registro automatico via callback do EventManager em main.cpp
* Consulta temporal com obter() e visualizacao com imprimir()
* Integrado: cada evento do barramento e persistido no historico

---

## T016 — Sistema de Alertas

Status: Concluido

Gerenciar alarmes persistentes.

---

# FASE 4 — Diagnóstico

Objetivo

Autodiagnóstico completo.

---

## T017 — Boot Diagnostics

Verificação completa durante inicialização.

---

## T018 — Runtime Diagnostics

Monitoramento interno contínuo.

---

## T019 — Self Test

Testes automáticos dos módulos.

---

# FASE 5 — Comunicação

Objetivo

Conectar o firmware ao mundo externo.

---

## T020 — Communication Layer

Criar camada de comunicação.

---

## T021 — Wi-Fi Manager

Gerenciamento da rede.

---

## T022 — MQTT Client

Cliente MQTT.

---

## T023 — Payload Builder

Construção dos pacotes MQTT.

---

## T024 — Reconexão Inteligente

Reconexão automática.

---

# FASE 6 — Dashboard

Objetivo

Visualização dos dados.

---

## T025 — Node-RED

Dashboard inicial.

---

## T026 — Banco de Dados

Persistência histórica.

---

## T027 — API REST

Interface HTTP.

---

## T028 — Dashboard Web

Painel próprio.

---

# FASE 7 — Inteligência Analítica

Objetivo

Extrair conhecimento dos dados.

---

## T029 — Indicadores Estatísticos

KPIs completos.

---

## T030 — Tendências Avançadas

Modelos estatísticos.

---

## T031 — Detecção de Anomalias

Identificar comportamentos incomuns.

---

## T032 — Assinatura da Máquina

Criar perfil individual de cada motor.

---

# FASE 8 — Inteligência Artificial

Objetivo

Criar IA embarcada.

---

## T033 — Dataset Builder

Gerar dataset automaticamente.

---

## T034 — Feature Engineering

Extrair características.

---

## T035 — Modelo de IA

Treinamento.

---

## T036 — Inferência Local

Executar IA no ESP32.

---

# FASE 9 — Gestão Industrial

Objetivo

Transformar o firmware em plataforma IIoT.

---

## T037 — Cadastro de Máquinas

Identificação única.

---

## T038 — Gestão de Ativos

Inventário.

---

## T039 — Histórico Completo

Linha do tempo.

---

## T040 — Relatórios

Relatórios automáticos.

---

# FASE 10 — Produto Final

Objetivo

Preparação para produção.

---

## T041 — Otimização de Memória

Redução de RAM.

---

## T042 — Otimização de Flash

Redução do firmware.

---

## T043 — Testes de Estresse

Execução contínua.

---

## T044 — Testes de Longa Duração

72 h

168 h

720 h

---

## T045 — Documentação Final

Atualização completa dos manuais.

---

## T046 — Release Candidate

Preparação para RC.

---

## T047 — Versão 1.0.0

Primeira versão industrial estável.

---

# Regras Gerais

Toda tarefa deverá possuir:

* Objetivo
* Escopo
* Critérios de aceite
* Dependências
* Arquivos afetados
* Fluxo de execução
* Compatibilidade
* Impactos
* Melhorias futuras

Nenhuma tarefa poderá violar:

* SUS
* CCS
* RAM
* PSS

Todos os módulos deverão permanecer desacoplados.

Toda implementação deverá ser preparada para futuras expansões.

Fim do documento.
