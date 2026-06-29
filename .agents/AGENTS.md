# Project SIGMA — Agent Guide

ESP32-S3 embedded firmware (Arduino via PlatformIO) for predictive motor monitoring with MPU6050 (vibration) and DS18B20 (temperature).

## Quick commands

```powershell
pio run                      # build all
pio run --target upload      # build + flash via USB
pio device monitor -b 115200 # open serial monitor (SUS protocol output)
pio test                     # run unit tests (none exist yet; test/ dir is empty)
```

Pre-build hook: `update_headers.py` auto-injects the current version from `src/hal/Version.h` into every `.h/.cpp` file header. No manual version bumps in source files.

## Architecture (5 layers)

Camada 5 (Application) — `src/main.cpp` (orchestrator, globals, setup/loop)
Camada 4 (Services)    — `src/services/` (health, alarm, calibration, analytics)
Camada 3 (Storage)     — `src/storage/` (NVS wrappers: sigma_cal, sigma_sys, sigma_cfg, sigma_bas)
Camada 2 (Drivers)     — `src/drivers/` (Mpu6050Driver, Ds18b20Driver)
Camada 1 (HAL)         — `src/hal/` (PinConfig, Version)

Dependencies flow downward only. No upper-layer includes in lower layers.

## Mandatory workflow for every change

Before making ANY code changes, YOU MUST:

1. **Bump version and codename** in `src/hal/Version.h`:
   - Update `SIGMA_FIRMWARE_VERSION` (semantic versioning)
   - Update `SIGMA_CODENAME` (short description of the feature)

2. **Run `py -3 update_headers.py`** to propagate version + codename
   to all `.h/.cpp` file headers.

3. **Commit with `git add -A && git commit -m "v<versao> - <codename>"`**

4. **Tag with `git tag v<versao> && git push origin v<versao>`**

Only then proceed with the actual code changes. This ensures every
commit is traceable to a specific versioned release.

## Key facts an agent will miss

- **Version source of truth:** `SIGMA_FIRMWARE_VERSION` and `SIGMA_CODENAME`
  in `src/hal/Version.h`. Never edit version strings in individual source
  files — `update_headers.py` propagates them on every change.
- **Pin mapping:** defined in `src/hal/PinConfig.h` (DS18B20=GPIO4, MPU SDA=GPIO8, SCL=GPIO9, INT=GPIO14).
- **NVS namespaces:** `sigma_cal` (calibration offsets), `sigma_sys` (horimeter/resets), `sigma_cfg` (escala/vibAviso/vibCritica), `sigma_bas` (baseline data).
- **Watchdog:** 10 s timeout, `esp_task_wdt_reset()` required in every blocking loop/delay. Fatal if missed.
- **Memory constraints (forbidden in runtime):** `new`, `delete`, `malloc`, `free`, `std::vector`, `std::list`, `std::string`, Arduino `String` in critical paths — use fixed `char[]`. No recursion. No exceptions/RTTI.
- **Serial output MUST follow SUS** (Serial UI Standard): 60-char max per line, ASCII only, category prefixes (`[INFO]`, `[WARN]`, `[ERRO]`, `[CMD]`, `[CAL]`, `[BAS]`, `[NVS]`, etc.), `oC` for °C, `F()` macro for all string literals.
- **Code comments MUST follow CCS** (Code Comment Standard): Portuguese (Brazil), 60-char line limit, specific header/function/section/group formats (all documented with examples in the old `.agents/AGENTS.md` which this replaces — the four full manuals there are now authoritative sources but this guide is the compact reference).
- **Comment language:** Portuguese (Brazil). Technical terms in English allowed: offset, baseline, health score, watchdog, NVS, RMS, sigma, buffer, callback, etc.
- **Serial commands:** `C`=calibrate, `A`=sensitivity menu, `N`=clear NVS cal, `H`=reset horimeter, `R`=factory reset config, `B`=collect baseline, `K`=adjust K-factor, `Z`=delete baseline.
- **Analytics buffer:** Circular FIFO, size 300 (set by `ANALYTICS_BUFFER_SIZE` in Version.h), ~7.2 KB SRAM.
- **Empty stubs exist at `src/monitor/analytics/`** — future module location. Currently unused; real analytics are in `src/services/analytics/`.

## Style conventions

- All serial output in Portuguese (Brazil), ASCII only, prefix-bracketed categories.
- All comments in Portuguese (Brazil), 60-char max per line.
- Structs for data crossing layers (`*Data`, `*Context`, `*Result`). No abstract base classes or virtual dispatch.
- `enum class AlarmLevel : uint8_t { NORMAL, WARNING, CRITICAL }` for typed alarm state.
- I2C at 400 kHz, Wire pins configured in PinConfig.h.
- Loop cycle: 500 ms (`INTERVALO_LEITURA_MS`).
- Health thresholds: >55 °C = AVISO, >65 °C = CRITICO; vibration limiar = basMedia + K * basStddev.

## Canonical sources

- **SUS (Serial UI Standard):** `docs/manuais/Project SIGMA - Manual de Interface Serial v2.1.md`
- **CCS (Code Comment Standard):** `docs/manuais/Project SIGMA - Manual de Comentários de Código v1.0.md`
- **DAM (Documentation Agent Manual):** `docs/manuais/Project SIGMA - Manual do Agente de Documentação Técnica v1.0.md`
- **PSS (Directory Structure & Naming):** `docs/manuais/Project SIGMA - Manual de Estrutura de Diretórios e Padrão de Nomenclatura v1.0.md`

These are the authoritative definitions. This agent guide is a compact reference; consult the manuals above when full rule detail is needed.
