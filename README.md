# Project SIGMA

Project SIGMA é um sistema embarcado para monitoramento preditivo de motores industriais, desenvolvido utilizando um ESP32-S3 e sensores de temperatura e vibração.

O objetivo do projeto é monitorar continuamente as condições de operação de um motor, detectar anomalias e fornecer indicadores que auxiliem na manutenção preditiva, reduzindo falhas e aumentando a confiabilidade do equipamento.

O firmware está sendo desenvolvido em C++ utilizando o Framework Arduino através do PlatformIO, com foco em organização, escalabilidade e documentação do código.

## Hardware

- ESP32-S3-DevKitC-1
- MPU6050 (Vibração)
- DS18B20 (Temperatura)

## Tecnologias

- C++
- Arduino Framework
- PlatformIO

## Status

O projeto encontra-se em desenvolvimento ativo.

A versão atual implementa o núcleo do sistema de monitoramento, incluindo aquisição de dados dos sensores, calibração do acelerômetro, configuração de sensibilidade via interface Serial, cálculo de indicadores do equipamento e geração de alarmes.

Novas funcionalidades serão adicionadas gradualmente conforme a evolução do projeto.

## Licença

Copyright (c) 2026 Bruno Alex Souza da Silva

All Rights Reserved.

This software and associated documentation files are proprietary and confidential.

No part of this software may be copied, modified, distributed, published, sublicensed, sold, or used in any form without prior written permission from the copyright holder.
