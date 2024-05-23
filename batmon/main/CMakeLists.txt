idf_component_register(SRCS 
                        "batmon_main.c" 
                        "tinbus.c" 
                        "mb_crc.c" 
                        "hardware.c" 
 #                       "batmon_littlefs.c" 
                        "batmon_wifi.c"
                        "rest_server.c"
                        "sntp_client.c"
                        INCLUDE_DIRS ".")

