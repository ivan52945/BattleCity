# Game — игровой слой ядра

Сущности и кадр: `entity.hpp` (база, side_t/act_t), `tank.hpp`/`shell.hpp`
(финальные наследники), `game.hpp` (`GameT<LcdT>` — кадр, пулы, коллизии),
`map.hpp` (terrain), `assets.hpp`/`Src/assets.cpp` (единственный .cpp —
flash-спрайты), `config.hpp` (`cfg::` ручки), `point.hpp`.

Разделение ролей: контракт использования (вход/выход, пример, ловушки
потребителя) — `README.md` рядом; здесь — правила изменения.

## Контракты

- Вся цепочка — шаблоны над `LcdT`: `GameT<LcdT>` → `Lobby<Tank<LcdT>>` /
  `Lobby<Shell<LcdT>>` → `Entity<LcdT>`; несколько инстанциаций = несколько
  панелей в одной прошивке. Docking — concept `Disp`.
- Blit-путь — статические вызовы `LcdT::print(...)`: `Entity::print()/clear()`
  не знают объектов драйвера; `GameT` держит единственную ссылку `disp`
  для lifecycle.
- Пулы: танковый = `2 * cfg::units_per_side` (квота — единственный источник),
  шеловый = 96 (хардкод, пункт P бэклога).
- Геометрия: `fld_x_end/fld_y_end` и лестница спавна — constexpr
  инстанциации; карта `map.hpp` — авторский массив строк → constexpr-сетка
  в flash (легенда `.`/`#`/`X`/`~`/`*`), панель = окно-кроп или wrap.
- Ручки `cfg::`: `spawn_margin_px=30`, `spawn_slots=7`, `units_per_side=4`,
  `map_cell_px=8` (static_assert'ы внутри config.hpp/map.hpp).

## Порядок кадра (`GameT::loop`) — не менять без чтения комментариев

1. `spawn()` — пересчёт живых по side-тегам, букинг через `is_booked`.
2. Танковая фаза (без отрисовки): `update` → танк-танк `resolve_collision`
   (O(n²), самопара пропущена) → `shot` через `shells.activate()`.
3. `disp.select()` — открытие CS-окна. МАИНТЕНАНС: новые blit'ы выше этой
   строки нельзя — на общей шине печать до пика игнорируется (MAINTENANCE-
   комментарий в game.hpp повторяет это правило).
4. Снарядная фаза: `Shell::update` (снос кирпича — битсеты `destroyed` +
   `dirty`) → shell-tank resolve → book умерших.
5. `map.flush_destroyed()` — repaint снесённого кирпича, до любых print.
6. Lull-проходы (shells, затем tanks) — трупы выталкиваются, пулы сжимаются.
7. Draw-проходы: print'ы танков и снарядов (+ `tile_touch` каждого).
8. `map.tile_flush()` — тайловая маска последним blit кадра; затем
   `disp.unselect()`.

## Инварианты и ловушки

- Контракт BLOCK у print/clear держится на всех путях (коллизии откатывают
  шаг — ghost-полос нет); `Tank::back()` принимает map и не толкает в стену.
- Спавн-дисциплина compile-time: `MapT::spawn_free` + `static_assert
  ladder_free()` — правка карты, перекрывшая спавн, валит сборку платформы.
- `print_img` драйвера обязан быть noinline — иначе тело SetWindow+DMA
  реплицируется в каждый сайт (+1–3 КБ flash, ADR-0004).
- `using`-реэкспорт зависимой базы — по цензу: ≥3 голых сайта на имя,
  остальное через `this->` (Tank 6, Shell 3 канализации).
- Stack: `GameT` = 5 608 Б (ARM-ABI; пулы 716 + 4 620 + карта/прочее) —
  см. metrics.md; деструкторы Tank/Shell не существуют (пул живёт в GameT,
  teardown на MCU недостижим).
- Случайность: `rand()` (три ролла Tank) — libc-машина; замена на 32-битный
  xorshift — пункт V бэклога, не мелочь.

## Не трогать без обсуждения

- Порядок кадра и CS-окно (п. 3–8), контракт BLOCK, спавн-static_assert'ы.
- Пул шелов 96 и `rand()` — есть оформленные пункты P и V.
