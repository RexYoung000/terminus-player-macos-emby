include(FetchDependencies)

option(WEB_CLIENT_DISABLE_DESKTOP "" OFF)

# This is the line to edit when you bump the web-client.
set(WEB_CLIENT_BUILD_ID jwc-10.8.9)

message(STATUS ${BUILDIDS})

# TODO make this actually work
