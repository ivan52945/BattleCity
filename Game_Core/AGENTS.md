# Game_Core — платформенно-независимое ядро

Вся игровая логика живёт здесь и не знает о HAL/CubeMX: ядро собирается теми
же заголовками и на хосте (host-тест `tmp/host_test/`). Один .cpp на всё
ядро — `Game/Src/assets.cpp` (спрайты в flash), остальное — header-шаблоны
над типом панели `LcdT`.

Разделение ролей: контракт использования (подключение, инстанциация,
предупреждения) — `README.md` рядом; здесь — правила изменения.

## Состав и цели CMake

- `Game/` → target `game_core`: библиотека из одного `assets.cpp`,
  include-путь `Inc` PUBLIC.
- `Utils/` → target `utils` (INTERFACE, `Inc`).
- `DeviceDrivers/` → target `display` (INTERFACE, `Inc` + link `stm32cubemx` —
  заголовки драйверов включают `main.h`).
- Связка: `game_core` PUBLIC `utils`, PRIVATE `display`; платформа линкует
  `game_core display stm32cubemx` (см. `Platforms/*/CMakeLists.txt`).
- В корне `Game_Core/CMakeLists.txt` — только `add_subdirectory` всех трёх.

## Инварианты

- C++20; стыковка драйвера — только через concept `Disp`
  (`DeviceDrivers/Display/Inc/disp.hpp`), нарушение формы ловится в точке
  инстанциации `GameT`, а не в трёх call-sites.
- Ядро не включает `main.h`/HAL: панельная геометрия приходит из
  constexpr `LcdT::w()/h()`, поля концы сворачиваются в иммедиаты.
- Игровой путь без косвенностей: blit — статический вызов драйвера,
  dispatch-записей нет по построению (ADR-0004).
- Всё состояние `GameT` (пулы + карта) — один объект на стеке `main()`:
  рост ядра двигает stack-usage, а не .bss/.data (см. metrics.md).
- Ручки геймплея — только `Game/Inc/config.hpp` (`cfg::`); верхняя
  геометрия (концы поля, лестница спавна) — не ручка, а константа
  инстанциации драйвера.

## Контроль

- Регресс-сетка ядра: юнит-тесты `Tests/` (doctest, автопрогон после
  сборки; контракты — `Utils/README.md`) + host-тест `tmp/host_test/`.
- FLASH/RAM — `--print-memory-usage` против базлайнов `docs/metrics.md`.
