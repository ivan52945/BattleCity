# ADR-0002 — NTP-wiring драйверов: wiring как non-type параметры шаблона

Статус: принято. Дата: ≈2026-11-03 (экспы в `tmp/inline_const_exp/`).

## Контекст

Wiring-поля драйверов (порты, пины, хэндлы) были константными членами.
Замер «const-поля ничего не дают»: RT == CST байт-в-байт, заголовки
создаёт outliner (пункт K). Цель — чтобы компилятор штамповал wiring в
тела функций, а объект не таскал полей.

## Решение

- `st7735.hpp`/`st7789.hpp` переписаны в header-шаблоны; wiring — целиком
  non-type параметры: `<&hspiN, DC_ADDR/DC_PIN, CS_ADDR/CS_PIN/SINGLE,
  RST_ADDR/RST_PIN, SIZES, Y_INV/X_INV/XY_CHG/COL_BGR>`.
- Порты — **целые `GPIOx_BASE`**, не указатели: Clang-21 отвергает
  round-trip `(std::uintptr_t)GPIOE` как NTP (reinterpret_cast в constant
  expression); материализация указателя — внутри `port()`-функции.
- SPI-хэндл — pointer-NTP (`&hspi4`/`&hspi1`, адрес линкуется);
  незаведённые линии — `0`, гварды вычисляются компилятором, ветки не
  генерируются.
- `sizes`/`single_device`/биты MADCTL — параметры; геометрия и байт MADCTL
  — constexpr; init-таблица — `static constexpr` во flash; поля wiring+
  geometry удалены; `st7735.cpp`/`st7789.cpp` удалены; HAL-SPI принимает
  `const uint8_t*` — const_cast не нужен нигде.

## Последствия (замеры, Release)

- Объект драйвера = vptr + scratch: **−68 Б RAM/объект** (ST7735 320 Б /
  ST7789 416 Б); `select/unselect` без CS = `bx lr`; `TrnsmtCMD`/`SetWindow`
  — imm/pool-константы; `init()` — константный цикл по 21 строке таблицы.
- H743 text −660 Б (27 600 → 26 940), с LTO −784 Б (26 684 → 25 900);
  F401: 18 116 text / 1 816 bss.
- Ключевой вывод (дампы `dump_new_lto.txt`/`dump_old_ltorel.txt`): LTO **не
  девиртуализирует** вызовы `Disp_Iface` из ядра — все blx vtable остались;
  −784 Б целиком изнутри драйвера. Это закрыло иллюзию «LTO решит
  виртуальность» и подготовило ADR-0004.
