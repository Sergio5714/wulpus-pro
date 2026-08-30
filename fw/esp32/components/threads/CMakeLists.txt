idf_component_register(
    SRCS "threads.c" "acquisition_thread.c" "packet_tx_thread.c"
         "protocol_thread.c" "usb_thread.c" "tcp_thread.c"
         "provisioning_thread.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES board frames control protocol links provisioner mdns_manager esp_system)
