
# Telemetry Monitor Service

SRCS-$(CONFIG_SERVICE_TELEMETRY) += \
	services/telemetry/telemetry_service.c  \

SRCS-$(CONFIG_SERVICE_TELEMETRY_PUBLISH) += \
	services/telemetry/telemetry_publish.c  \
	services/telemetry/packet/CcsdsUtils.c  \
	services/telemetry/packet/Endian.c  \
	services/telemetry/packet/PusUtils.c  \
	services/telemetry/packet/Crc.c  \
	services/telemetry/packet/Serializers.c  \
	services/telemetry/packet/SerializersUser.c  \
	services/telemetry/packet/Fillers.c  \
	services/telemetry/packet/FillersUser.c  \

INCLUDES +=\
	-I./services/telemetry \
