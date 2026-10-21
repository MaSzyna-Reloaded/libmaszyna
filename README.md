# libmaszyna

MaSzyna's vehicles, sceneries and drawing for Rails Reloaded: a plugin of libreloaded, built into its
library. Licensed under the Mozilla Public License 2.0 (`LICENSE`), as MaSzyna is.

libreloaded is the `vendor/libreloaded` submodule (`make libreloaded-submodule`); to work against a
local checkout of it instead: `make libreloaded-local LIBRELOADED_LOCAL=<path>`. Then:

    make compile-debug
    make import
    make test TEST=<test script name>
    make style-check STYLE_FILE=<file>
