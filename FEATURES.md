# smartmet-plugin-cross_section — Feature List

A structured inventory of capabilities provided by the SmartMet
cross-section plugin. Use as a checklist when drafting release notes.
When new functionality is added, append the new entry under the
matching section (and bump the *Last updated* line at the bottom).

`smartmet-plugin-cross_section` (output: `cross_section.so`) generates
**vertical atmospheric cross-section** products from gridded weather
data. Given a geographic path and a set of parameters, it produces
isoband and isoline contours along the vertical slice between the
endpoints. Output is JSON containing SVG path strings, suitable for
direct client-side rendering. The plugin registers at `/csection`.

---

## 1. Endpoint

- **`/csection`** — single HTTP endpoint, configurable via `url` in
  the plugin config.
- **Request method** — HTTP GET.
- **Customer scoping** — each request targets one customer's product
  catalogue via the `customer=` parameter.

## 2. Product model

- **JSON product definition** — each product is a JSON file under
  `{root}/customers/{customer}/products/{name}.json`.
- **Layer composition** — a product owns a `Layers` collection that
  is rendered in order.
- **Reusable layer fragments** — common layer definitions can live
  under `{root}/customers/{customer}/layers/`.
- **JSON includes** — `"json:path/to/file.json"` substitutes the
  contents of another JSON file (`JSON::expand`).
- **JSON internal references** — `"path:name1.name2"` dereferences
  another node inside the same product (`JSON::dereference`).

## 3. Layer types

Built by `LayerFactory` from the JSON `layer_type` field; the abstract
`Layer` base is concrete-extended by:

- **`isoband`** — filled bands between value ranges (`IsobandLayer`,
  uses `Isoband.cpp`).
- **`isoline`** — line contours at value levels (`IsolineLayer`, uses
  `Isoline.cpp`).

Both layer types support the same configurable attributes:

- **Parameter selection** — which meteorological parameter to
  contour.
- **Value ranges** (`AttributeSelection`) — band limits or line
  levels.
- **Custom SVG attributes** — strokes, fills, classes, etc., applied
  to each contour element.

## 4. Two data sources

A single layer can be rendered from either backend:

- **`querydata` engine** (`generate_qEngine`) — FMI's native gridded
  format. Default.
- **`grid` engine** (`generate_gridEngine`) — GRIB / NetCDF data via
  the grid stack.

Selected by the `source=` request parameter or by the layer's own
JSON config.

## 5. Path specification

Cross-section paths are defined by the request parameters parsed into
`Query`:

- **Endpoint locations** — resolved via the geonames engine, so place
  names, lat/lon, and geo IDs all work.
- **Steps** — `steps=N` controls how many sample points are taken
  along the path.
- **Timezone** — `timezone=...` (defaults to the plugin's `timezone`
  config, typically `UTC`).
- **Producer** — `producer=...` picks the data producer; `zproducer=`
  may select a different producer for the vertical (Z) axis (e.g.
  geopotential heights from a separate model).

## 6. Vertical axis

- **Pressure levels** — pressure-coordinate cross-sections.
- **Height levels** — geometric or geopotential height
  cross-sections.
- **Z-producer** — `zproducer=...` allows pulling vertical-level
  metadata from a different producer than the data parameter.

## 7. Time selection

- **Per-request iteration** — `Product::generate` iterates available
  time steps and renders each as its own panel.
- **Time-zone aware rendering** — labels and axis ticks use the
  requested timezone.
- **Reproducible timestamps** — the timezone is applied uniformly
  across all layers in one product.

## 8. Contouring

- **Engine-backed** — uses the `contour` engine (marching-squares
  via `trax`) to compute isobands and isolines.
- **Cached snapshot** — `State` holds the producer data handle (`Q`)
  for the duration of the request, so every layer sees the same
  data snapshot.
- **Per-engine path** — `IsobandLayer` and `IsolineLayer` each have a
  `generate_qEngine` and a `generate_gridEngine` implementation that
  fetch data from the selected source and hand it to the contour
  engine.

## 9. Output format

- **Primary template**: **`svgjson`** — JSON containing SVG path
  strings for each contour. Suitable for client-side rendering by
  injecting the paths into an `<svg>` element.
- **CTPP2 templates** — `tmpl/svgjson.tmpl` is compiled to bytecode
  (`tmpl/svgjson.c2t`) at build time.
- **Template selection** — `template=...` (default: `svgjson`).
- **Per-thread template cache** — `TemplateFactory` uses a
  `boost::thread_specific_ptr` so concurrent requests don't share
  CTPP2 VMs.
- **Result content type** — JSON (`application/json`).

## 10. Caching

- **`FileCache`** — caches resolved JSON product / layer files so
  repeated requests don't re-read or re-expand them.
- **Per-request state cache** — `State` caches the producer handle
  to avoid redundant engine calls inside a single product.
- **Template bytecode cache** — `tmpl/*.c2t` is compiled once at
  build time and loaded into the per-thread `TemplateFactory`.

## 11. Engine integration

The plugin links to **four engines**, all resolved at runtime via
`dlopen`:

- **querydata** — primary data source.
- **grid** — secondary data source (GRIB / NetCDF).
- **contour** — isoband / isoline computation.
- **geonames** — place name resolution and timezone metadata.

## 12. Configuration

Plugin config (libconfig with the SmartMet `@include` / `@ifdef`
extensions):

- **`root`** (required) — base directory for customer product JSON
  files.
- **`url`** — HTTP endpoint path (default `/csection`).
- **`template`** — default output template name (default `svgjson`).
- **`templatedir`** — directory for compiled CTPP2 templates.
- **`customer`** — default customer name (default `fmi`).
- **`timezone`** — default timezone (default `UTC`).
- **Standard SmartMet config extensions** — `@include`, `@ifdef`,
  `$(VAR)`, `%(DIR)`.

## 13. Request parameters

Recognised HTTP query parameters (parsed by `Plugin::query` /
`Query`):

- **`producer`** — data producer name.
- **`zproducer`** — producer for vertical-level metadata.
- **`product`** — product name (looked up under the customer
  directory).
- **`customer`** — customer namespace (defaults to the config's
  `customer`).
- **`source`** — `querydata` or `grid`.
- **`steps`** — number of sample points along the path.
- **`timezone`** — timezone for time labels.
- **`format`** — output format (template name).
- **`json`** — inline JSON product (alternative to a product file).
- **`hash`** — content hash for client-side cache validation.
- **`debug`** — debug output mode.
- **`timer`** — request timing instrumentation.

## 14. Testing

- **Integration tests** under `test/`, driven by
  `smartmet-plugin-test`.
- **Inputs** — HTTP request files in `test/input/*.get`.
- **Expected outputs** — `test/output/*.get`.
- **WGS84 mode** — tests substitute `.wgs84` variants when the
  system newbase is built with WGS84 support; outputs are staged in
  `test/tmp/`.
- **Reactor config** — `test/cnf/reactor.conf` boots a minimal
  SmartMet Server with the plugin and the four engines loaded.

## 15. Build & integration

- **Output**: `cross_section.so`.
- **Loaded at**: `$(prefix)/share/smartmet/plugins/cross_section.so`.
- **Build**: `make`.
- **Format**: `make format` runs clang-format.
- **Install**: `make install`.
- **RPM**: `make rpm`.
- **Templates**: `tmpl/*.tmpl` are compiled to CTPP2 bytecode
  (`*.c2t`) as part of the build.
- **Linked SmartMet libraries**: spine, macgyver, newbase, gis,
  grid-files, grid-content.
- **External libraries**: GDAL, libconfig (configpp), CTPP2,
  jsoncpp, Boost.
- **Engine references** are deliberately unresolved at link time and
  resolved by the server via `dlopen` at runtime.

---

*Last updated: 2026-06-01.*
