@AGENTS.md

## Backlog and design

1. Session start: `sp backlog next`. Take what it gives.
2. A defect or limitation turns up: `sp backlog add issue ...` or `sp backlog add work ...`
   at once.
3. Before changing a module: `sp design owner <path>`.
4. Finishing: `sp backlog close <ID>` in the same commit, and the changelog line it prints.
   The commit message ends with `Backlog: <ID>`, or `Backlog: none (<reason>)`.
5. Once per clone: `python tools/hooks/install_hooks.py`.

State is read with these commands. It is never read from a markdown file or the git log.
