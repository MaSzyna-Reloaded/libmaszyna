---
layout: page
title: "libmaszyna"
---

**libmaszyna** is a port of [MaSzyna - Symulator Pojazdów Szynowych](https://eu07.pl) to Rails
Reloaded. It is a [plugin](../architecture#plugins) of the core, libreloaded, and is compiled into
the core's library. It adds MaSzyna's vehicles, sceneries, models, cabins and rendering.

libmaszyna provides:

* **Vehicle physics.** MaSzyna's original physics code (MOVER) is included unchanged and connected
  to the core's vehicle interfaces. See [Wrapping the MOVER](wrapping-mover).
* **File formats.** Loaders and importers for MaSzyna's **E3D**, **FIZ**, **MMD**, materials,
  sceneries and asset databanks.
* **Cabins.** The MMD cab logic and the Python screens of the cabs.
* **Sceneries.** Scenario events, memory cells and the original Lua API (`eu07.events`).
* **Rendering.** MaSzyna's models, tracks, roads and traction.

## Documentation

* [Architecture of the port](architecture) - what libmaszyna adds to the core and how it is built
* [Wrapping the MOVER](wrapping-mover) - how a MaSzyna vehicle runs on the core
* [Lua scenario scripts](lua-scripting) - memory cells, `eu07.events` and examples
* [API reference](../api/) - all classes of libreloaded and libmaszyna

## MaSzyna file formats

* [FIZ files](https://wiki.eu07.pl/index.php/Plik_charakterystyki)
* [E3D format](https://wiki.eu07.pl/index.php/Format_E3D)
* [E3D format - Rainsted Wiki](http://rainsted.com/pl/Format_binarny_modeli_-_E3D)
* [MMD format](https://wiki.eu07.pl/index.php/Plik_multimedi%C3%B3w_(mmd))
* [MAT format](https://wiki.eu07.pl/index.php/Materia%C5%82y_-_Pliki_.mat)
* [Scenery file format](https://wiki.eu07.pl/index.php/Plik_scenerii)
* [Node objects](https://wiki.eu07.pl/index.php/Obiekt_node)
