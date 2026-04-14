# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

SmartMet cross-section plugin (`smartmet-plugin-cross_section`). It generates vertical atmospheric cross-section images as JSON (containing SVG path data) from gridded weather data. Given two geographic locations and a set of meteorological parameters, it produces isoband/isoline contour data along the vertical slice between those points.

The plugin is loaded as a shared library (`cross_section.so`) by SmartMet Server at runtime. It registers the `/csection` HTTP endpoint.

## Build commands

```bash
make                # Build the plugin (produces cross_section.so)
make format         # Run clang-format on all source files
make clean          # Clean build artifacts
make rpm            # Build RPM package
cd test && make test  # Run integration tests (requires smartmet-plugin-test and engine data)
```

Templates (`tmpl/*.tmpl`) are compiled to CTPP2 bytecode (`*.c2t`) as part of the build.

## Engine dependencies

The plugin requires four engines to be loaded by the server (resolved at runtime via dlopen, not at link time):

- **querydata** — provides access to QueryData (FMI's native gridded data format)
- **grid** — provides access to GRIB/NetCDF grid data (alternative data source)
- **contour** — performs the actual isoband/isoline contouring (marching squares via `trax`)
- **geonames** — resolves place names to coordinates and provides timezone info

## Architecture

**Request flow:** HTTP request → `Plugin::requestHandler` → `Plugin::query` → build `Query` + `State` → load `Product` JSON → `Product::generate` (iterates time steps, calls `Layer::generate` for each layer) → CTPP2 template renders output JSON.

**Key classes:**

- `Plugin` — entry point; implements `SmartMetPlugin` interface (`create`/`destroy` C exports)
- `Query` — holds parsed HTTP request parameters (producer, locations, steps, timezone)
- `State` — per-request mutable state passed to all layers; caches the producer data handle (`Q`) so all layers in one product see the same data snapshot
- `Product` — initialized from a customer's product JSON; contains a `Layers` collection
- `Layer` (abstract) → `IsobandLayer`, `IsolineLayer` — each layer type contours one parameter. Both support `querydata` and `grid` backends (separate `generate_qEngine`/`generate_gridEngine` methods)
- `LayerFactory` — creates `Layer` subclasses based on `layer_type` in JSON ("isoband" or "isoline")

**Product configuration** is JSON-based, organized under a `root` directory as:
```
{root}/customers/{customer}/products/{name}.json   — product definitions
{root}/customers/{customer}/layers/                 — reusable layer fragments
```

JSON supports `"json:path/to/file.json"` includes (expanded by `JSON::expand`) and `"path:name1.name2"` internal references (expanded by `JSON::dereference`).

**Template system:** CTPP2 templates in `tmpl/` produce the output format. The default template `svgjson` emits JSON with SVG path strings for each contour. Templates are compiled to bytecode (`.c2t`) and cached per-thread via `TemplateFactory` (using `boost::thread_specific_ptr`).

## Testing

Tests are integration tests using `smartmet-plugin-test`, not Boost.Test unit tests. Test cases are HTTP request files in `test/input/` (e.g., `temperaturesea.get`) compared against expected output in `test/output/`. The test config in `test/cnf/reactor.conf` boots a minimal server with the plugin and required engines.

WGS84 mode: tests copy expected outputs to `test/tmp/` and swap in `.wgs84` variants when the system newbase is built with WGS84 support.

## Configuration

The plugin config (`libconfig` format) has these settings:
- `root` (required) — base directory for customer product JSON files
- `url` — HTTP endpoint path (default: `/csection`)
- `template` — default output template name (default: `svgjson`)
- `templatedir` — directory for compiled CTPP2 templates
- `customer` — default customer name (default: `fmi`)
- `timezone` — default timezone (default: `UTC`)
