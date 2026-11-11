# Platforms/STM32H743 — плата STM32H743 + ST7735 160×80

Пользование платформой: сборка и настройки платы. Правила изменения
(память — не двигать, реген-факты CubeMX) — `AGENTS.md` рядом.

## Назначение

Порт ядра на STM32H743 (WeAct) с панелью ST7735 80×160 portrait:
ландшафтный кадр получается кропом центральной полосы авторской карты —
сама инстанциация `GameT` его и сворачивает.

## Сборка

```
cmake --preset Release
cmake --build --preset Release
```

- Тулчейн `cmake/starm-clang.cmake` (ST clang 21.1.1, `-Oz -g0`), Ninja.
- Артефакт — `build/Release/Tanks_H743.elf`; размеры печатает линкер.
  Базлайны — `docs/metrics.md`.

## Настройки платы

- Тактирование: 240 МГц (PLLN 60).
- ICache: `SCB_EnableICache()` в `main.cpp` USER CODE Init — после
  MPU/HAL_Init, до `SystemClock_Config` (порядок по ST AN); выборка из
  flash: 4 WS → 0 WS при попадании в кэш; откат — одна строка
  `SCB_DisableICache()`.
- Дисплей: ST7735 WeAct 80×160 portrait, SPI4 TX-only DMA, 60 Мбит;
  линия CS подключена (PE11), RST — согласно подключению платы;
  boot-clear — полоса `disp_clear_160x16`.
- Инстанс: `using Lcd = ST7735<&hspi4, ...>`; темп кадра —
  `HAL_Delay(15)`.
- Память: стек — верх DTCM (`_estack = 0x20020000`), `GameT` растёт
  вниз из DTCM (CPU-only путь 0 WS); куча — до конца AXI RAM
  (`_heap_end`); DMA-секции — в RAM/AXI (DMA в DTCM не достанет).
  Детали раскладки и «не двигать без обсуждения» — `AGENTS.md`.
