# BattleCity

Портативные «Танчики» на STM32: типизированное C++20-ядро без HAL и без
vtable, header-шаблонные драйверы дисплея (wiring — non-type параметры),
спрайты и карта — constexpr в flash, DMA выводит напрямую. Кадр wire-bound:
время петли — это шина, не CPU.

## Платформы

| Платформа | Панель | Шина | Прошивка |
|---|---|---|---|
| STM32F401 (84 МГц) | ST7789 240×240 | SPI1, 42 Мбит | `build/Release/Tanks_F401.elf` |
| STM32H743 (240 МГц) | ST7735 160×80 | SPI4, 60 Мбит | `build/Release/Tanks_H743.elf` |

## Сборка

Нативно в Windows на CubeCLT 1.22.0 (WSL не нужен):

```
cd Platforms/STM32F401      # или Platforms/STM32H743
cmake --preset Release
cmake --build --preset Release
```

Размеры прошивок печатает линкер (`--print-memory-usage`). Актуальные
базлайны и методика замеров — `docs/metrics.md`.

Юнит-сетка ядра (`Game_Core/Tests/`) гоняется в CI на каждый push
(GitHub Actions: ubuntu + gcc + ctest под санитайзерами).

## Структура

```
Game_Core/            платформенно-независимое ядро (C++20, без HAL)
  Game/               Entity/Tank/Shell/GameT/Map/assets
  Utils/              пул Lobby<T, N>
  DeviceDrivers/      header-only драйверы ST7789/ST7735
Platforms/            CubeMX-проекты F401/H743 (+ рукописный main.cpp)
tmp/                  песочница; host-тест ядра (fake-драйвер)
docs/                 ADR (история решений), metrics.md (замеры)
```

## Документация

- `AGENTS.md` (корень и каталоги модулей) — правила и контракты для
  агентов и людей; каждый модуль описывает сам себя.
- `docs/adr/` — почему так решено (классы, NTP-wiring, typed-Game,
  ICache/DTCM, terrain…).
- `docs/metrics.md` — замеры, бюджеты и контро-пункты «не трогать».
