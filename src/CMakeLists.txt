idf_component_register(
    SRCS            "main.c"
)

# ── Enlazar los PEM como texto ─────────────────────────────
target_add_binary_data(${COMPONENT_LIB} "../tmp/AmazonRootCA1.pem" TEXT)
target_add_binary_data(${COMPONENT_LIB} "../tmp/humidity_sensor.cert.pem" TEXT)
target_add_binary_data(${COMPONENT_LIB} "../tmp/humidity_sensor.private.key" TEXT)
