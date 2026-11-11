# Game_Core — платформенно-независимое ядро

Контракт использования ядра: подключение и инстанциация. Правила
изменения — `AGENTS.md` рядом; история решений — `docs/adr/`.

## Назначение

Вся игровая логика «Танчиков» без HAL: C++20, header-шаблоны над типом
панели `LcdT`, единственный `.cpp` — спрайты в flash
(`Game/Src/assets.cpp`). Ядро инстанцируется везде, где есть компилятор
C++20: MCU-платформа, хост-зритель, тестовая панель-двойник (fake).

Модули: `Game/` (сущности и кадр), `Utils/` (пул `Lobby<T, N>`),
`DeviceDrivers/` (драйверы ST7789/ST7735) — у каждого свой `README.md`.

За что НЕ отвечает: темп кадра (платформа), подключение дисплея
(wiring) и его HAL-инициализация (платформа), всё, кроме дисплея.

## Как подключить

Три CMake-цели (корневой `CMakeLists.txt` — только `add_subdirectory`):

| Цель | Тип | Даёт |
|---|---|---|
| `game_core` | STATIC (`assets.cpp`) | include `Game/Inc` + спрайты |
| `utils` | INTERFACE | include `Utils/Inc` |
| `display` | INTERFACE | include `DeviceDrivers/Display/Inc`; подключает `stm32cubemx` |

- MCU-платформа собирает связку `game_core display stm32cubemx`.
- Хосту (зрителю, тесту) `display` не нужен — он подключает HAL,
  который на хосте не собирается: `assets.cpp` компилируется напрямую
  с тремя include-путями ядра (`Game/Inc`, `Utils/Inc`,
  `DeviceDrivers/Display/Inc`).

## Как пользоваться

```cpp
#include "game.hpp"        // GameT + вся цепочка сущностей

Lcd lcd;                   // любой тип под concept Disp:
                           // ST7789<...>, ST7735<...>, SimLcd, двойник
GameT<Lcd> game(lcd);      // всё состояние (пулы + карта) — на стеке

game.init();               // boot-clear, карта, начальный спавн
for (;;) { game.loop(); }  // один вызов = один кадр; темп — платформа
```

## Предупреждения

- Геометрия поля — constexpr от `LcdT::w()/h()`, не ручка: другая
  панель = другая инстанциация (кроп карты сворачивается сам).
- Ручки геймплея — только `Game/Inc/config.hpp` (`cfg::`).
- `GameT` = 5 608 Б на стеке (ARM-ABI): рост ядра увеличивает
  stack-usage, а не .bss/.data (см. `docs/metrics.md`).
- Ядро не включает `main.h`/HAL: нарушение ловится на компиляции.
