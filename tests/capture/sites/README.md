# tests/capture/sites

- `test_site.cpp`: a site is a constant aggregate and a static handle starts unregistered.
- `test_site_registry.cpp`: one id per handle however often it is asked; two sites with equal
  text are two sites; the filter's level and groups, ungrouped sites passing a group filter, a
  filter applied before a site registers; a site past the capacity gets 0 and is counted once;
  the registry keeps copies of the site's strings; eight threads registering one handle at once
  get one id.
- `test_name_table.cpp`: one id per text, 0 for empty text, empty text for an unknown id.
- `test_dynamic_site_table.cpp`: one stable handle per name; the first unit and operation win;
  the name is copied; the 4097th name is refused and counted; eight threads get one handle per
  name.
