# sites

Where a record comes from, and which records are kept, in the namespace `SushiProfiler`. It
depends on `events`.

- `SushiProfiler/sites/site.hpp`: `Site`, the static description of one place in the source
  that records: name, group, file, line, level, `SiteRole` and, for a counter, its `CounterOp`
  and unit. The recording macros build each as a `static constexpr` object. `SiteHandle` pairs
  a site with the id the registry gives it; a static handle is constant-initialised, so the hot
  path never runs a static guard for it.
- `SushiProfiler/sites/site_registry.hpp`: `SiteRegistry` gives each handle one id, from 1, on
  first use, under a lock, and copies the site's strings, so an id stays valid after the image
  that declared the site unloads and can be written to a file. After that, `id_of` is one
  acquire load. `enabled(id)` is one relaxed byte load from an array allocated once; id 0 reads
  false. `apply(SiteFilter)` rewrites every byte, and later registrations follow the same filter.
  A site past the capacity (65,536 by default) is refused once and counted by `refused()`; its
  handle is marked so it never takes the lock again.
- `SushiProfiler/sites/name_table.hpp`: `NameTable` interns run-time strings into `NameID`s,
  from 1, never freed; empty text is 0.

- `SushiProfiler/sites/dynamic_site_table.hpp`: `DynamicSiteTable` gives each counter or value
  name built at run time one site, so the record format needs no name field on a sample.
  `handle(name, unit, op, level)` returns one stable `SiteHandle*` per name, made on first use
  under a unique lock and found afterwards under a shared lock that allocates nothing. The first
  unit, operation and level of a name win. A table holds `K_MAX_NAMES`, 4096 names; a later name
  returns null and `refused()` counts it. `dynamic_counter_sites()` and `dynamic_value_sites()`
  are the process's two tables, one per role.

The filter keeps a site when its level is at most the filter's and either the filter names no
group, the site has no group, or its group is named. An ungrouped site's group is inherited from
the zone around it, which only the fold knows, so the group filter does not remove it.

The tests are in `tests/capture/sites/`.
