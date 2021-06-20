set(component_srcs "src/main.c" "src/app.c")

idf_component_register(SRCS "${component_srcs}"
                    INCLUDE_DIRS "inc")

#idf_component_register(SRCS "main.c"
#                    INCLUDE_DIRS "inc")