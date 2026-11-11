# Platforms/STM32F401 — плата STM32F401 + ST7789 240×240

Пользование платформой: сборка и настройки платы. Правила изменения
(CubeMX-дисциплина, бюджеты памяти) — `AGENTS.md` рядом.

## Назначение

Порт ядра на STM32F401 с панелью ST7789 240×240: CubeMX-проект
(`Tanks_F401.ioc`) + рукописный `Core/Src/main.cpp` — точка
инстанциации драйвера и `GameT`.

## Сборка

```
cmake --preset Release
cmake --build --preset Release
```

- Тулчейн `cmake/gcc-arm-none-eabi.cmake` (`-Os -g0`), Ninja.
- Артефакт — `build/Release/Tanks_F401.elf`; размеры печатает линкер
  (`--print-memory-usage`). Базлайны — `docs/metrics.md`.

## Настройки платы

- Тактирование: 84 МГц (HSI → PLLM 16 / PLLN 168), flash 2 WS;
  prefetch включён строкой `__HAL_FLASH_PREFETCH_BUFFER_ENABLE()` в
  `main.cpp` USER CODE Init — на кадр не влияет (A/B-замер, ADR-0006).
- Дисплей: ST7789 240×240 (`st_240x240`), SPI1 TX-only DMA, 42 Мбит
  (prescaler 2 — минимальный делитель, потолок скорости); boot-clear —
  полоса `disp_clear_240x16`.
- Подсветка: PWM TIM1_CH1.
- Инстанс: `using Lc = ST7789<&hspi1, ...>` в `main.cpp`; объект
  драйвера и `GameT<Lc> game(lcd)` — на стеке; темп кадра —
  `HAL_Delay(15)`.
- Память: стек-резерв `0x1800` (6 144 Б), heap `0x200`; `GameT` =
  5 608 Б на стеке — резерв покрывает с запасом ~536 Б.
