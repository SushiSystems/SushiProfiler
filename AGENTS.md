# Agent instructions

This repository holds SushiProfiler, the profiler of the Sushi libraries. It follows the Sushi
Systems skills: load `sushiskills` before working here; it names the skill each task needs.

Build and test through `sp` only (`sp build`, `sp test`, `sp check`); never call CMake, Ninja or
CTest directly. In PowerShell `sp` is a built-in alias; type `sushiprofiler`.

Check before reporting work as finished:

```
python tools/check_all.py .
```
