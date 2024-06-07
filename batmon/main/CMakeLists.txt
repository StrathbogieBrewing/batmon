idf_component_register(SRCS 
                        "batmon_main.c" 
                        "tinbus.c" 
                        "mb_crc.c" 
                        "hardware.c" 
                        "batmon_wifi.c"
                        "rest_server.c"
                        "sntp_client.c"
                        "nvm_esp.c"
                        "storage.c"
                        INCLUDE_DIRS ".")

