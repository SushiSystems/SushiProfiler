sushiprofiler_add_module(
    NAME periods TIER capture
    SOURCES
        period_timeline.cpp
    DEPENDS events
    PRIVATE_DEPENDS rings collector)
