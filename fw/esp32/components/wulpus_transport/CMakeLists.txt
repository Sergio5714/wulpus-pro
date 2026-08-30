idf_component_register(
    SRCS "wulpus_transport.c" "wulpus_tcp_transport.c" "wulpus_usb_transport.c"
    INCLUDE_DIRS "."
    REQUIRES sock esp_driver_usb_serial_jtag
)
