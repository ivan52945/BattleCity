# ADR-0007 — Память H743: ICache + стек в DTCM + MPU + _heap_end

Статус: принято. Дата: 2026-11-08 (вечер).

## Контекст

Фетч из flash на H743 — 4 WS; стек (вместе с `GameT`, 5 344 Б) лежал в AXI
RAM — CPU-only путь с задержками. Доступные ресурсы: 16 КБ I-cache, DTCM
128 КБ (CPU-only, DMA не достанет).

## Решение

- **ICache:** `SCB_EnableICache()` в `main.cpp` USER CODE Init — после
  `MPU_Config()`/`HAL_Init`, до `SystemClock_Config` (порядок ST AN: кэш
  после MPU). Coherency-обязанностей ноль: прошивка неизменяема, DMA кэш
  не использует. Откат — одна строка.
- **Стек → DTCM** (`STM32H743xx_FLASH.ld`):
  `_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM)` = 0x20020000, `_sstack` =
  0x2001FC00; растёт вниз; `GameT` уехал вместе со стеком — весь CPU-only
  сим-путь 0 WS. Вектор entry0 (`.word _estack`) не правился.
- **MPU region 2 (DTCM):** Device_NGNRE (TEX2) → Normal
  non-cacheable non-shareable (TEX1, C=0 B=0), XN. Device был легален, но
  Normal — по-канону (спекуляция/упорядочивание).
- **`_sbrk` фикс** (`sysmem.c`): граница кучи — новый символ
  `_heap_end = ORIGIN(RAM) + LENGTH(RAM)` = 0x24080000; со старым
  сравнением против `_estack` (в DTCM) любой malloc навсегда давал бы
  ENOMEM. malloc по-прежнему никто не зовёт — `--gc-sections` выкидывает
  `_sbrk`, но секция свяжется сама при появлении потребителей.

## Последствия

- FLASH 31 448 → 31 480 (+32: inline ICache + выписанный MPU-регион, −
  упрощённый `_sbrk`), RAM 1 904 const; DTCM/ITCM/SRAM в линкере — 0
  (стек не линкуется как данные).
- Правило `.ld`: DMA-секции держать в RAM/AXI — DMA в DTCM не достанет.
- Отложено: «всё в TCM» (код ~22.3 КБ < ITCM 64 КБ, данные ~6 КБ < DTCM
  128 КБ) — жизнеспособно, но ценой кастомного `.ld` вне Cube-шаблона +
  копии ~22 КБ в старте; ITCM для горячего кода — по цифрам DWT-зонда
  (актуальность снята wire-bound выводом ADR-0006).
- Живая проверка на плате (SP ≈ 0x2001FFxx при первом push, отсутствие
  странностей) — за пользователем.
