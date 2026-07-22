idf_component_register(
    SRCS
        "main.c"
        "ks203.c"
        "wifi_softap.c"
        "http_viz.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "index.html"
)
