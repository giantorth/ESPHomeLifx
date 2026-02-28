import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.components import time as time_
from esphome.const import CONF_ID
from esphome.core import CORE

CODEOWNERS = ["@giantorth"]

lifx_emulation_ns = cg.esphome_ns.namespace("lifx_emulation")
LifxEmulation = lifx_emulation_ns.class_("LifxEmulation", cg.Component)

CONF_COLOR_LED = "color_led"
CONF_WHITE_LED = "white_led"
CONF_RGBWW_LED = "rgbww_led"
CONF_BULB_LABEL = "bulb_label"
CONF_BULB_LOCATION = "bulb_location"
CONF_BULB_LOCATION_GUID = "bulb_location_guid"
CONF_BULB_LOCATION_TIME = "bulb_location_time"
CONF_BULB_GROUP = "bulb_group"
CONF_BULB_GROUP_GUID = "bulb_group_guid"
CONF_BULB_GROUP_TIME = "bulb_group_time"
CONF_TIME_ID = "time_id"
CONF_DEBUG = "debug"


def _validate_light_config(config):
    has_dual = CONF_COLOR_LED in config and CONF_WHITE_LED in config
    has_rgbww = CONF_RGBWW_LED in config

    if has_dual and has_rgbww:
        raise cv.Invalid(
            "Cannot specify both 'rgbww_led' and 'color_led'/'white_led'. "
            "Use either a combined RGBWW light OR separate color + white lights."
        )
    if not has_dual and not has_rgbww:
        raise cv.Invalid(
            "Must specify either 'rgbww_led' for a combined RGBWW light, "
            "or both 'color_led' and 'white_led' for separate lights."
        )
    if (CONF_COLOR_LED in config) != (CONF_WHITE_LED in config):
        raise cv.Invalid(
            "Both 'color_led' and 'white_led' must be specified together."
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LifxEmulation),
            cv.Optional(CONF_COLOR_LED): cv.use_id(light.LightState),
            cv.Optional(CONF_WHITE_LED): cv.use_id(light.LightState),
            cv.Optional(CONF_RGBWW_LED): cv.use_id(light.LightState),
            cv.Required(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
            cv.Optional(CONF_BULB_LABEL, default=""): cv.string,
            cv.Optional(CONF_BULB_LOCATION, default="ESPHome"): cv.string,
            cv.Optional(
                CONF_BULB_LOCATION_GUID,
                default="b49bed4d-77b0-05a3-9ec3-be93d9582f1f",
            ): cv.string,
            cv.Optional(CONF_BULB_LOCATION_TIME, default=1553350342028441856): cv.positive_int,
            cv.Optional(CONF_BULB_GROUP, default="ESPHome"): cv.string,
            cv.Optional(
                CONF_BULB_GROUP_GUID,
                default="bd93e53d-2014-496f-8cfd-b8886f766d7a",
            ): cv.string,
            cv.Optional(CONF_BULB_GROUP_TIME, default=1600213602318000000): cv.positive_int,
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_light_config,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_RGBWW_LED in config:
        rgbww_led = await cg.get_variable(config[CONF_RGBWW_LED])
        cg.add(var.set_rgbww_led(rgbww_led))
    else:
        color_led = await cg.get_variable(config[CONF_COLOR_LED])
        cg.add(var.set_color_led(color_led))

        white_led = await cg.get_variable(config[CONF_WHITE_LED])
        cg.add(var.set_white_led(white_led))

    ha_time = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(ha_time))

    # Default label to the ESPHome device name if not explicitly set
    label = config[CONF_BULB_LABEL] or CORE.name
    cg.add(var.set_bulb_label(label))

    cg.add(var.set_bulb_location(config[CONF_BULB_LOCATION]))
    cg.add(var.set_bulb_location_guid(config[CONF_BULB_LOCATION_GUID]))
    cg.add(var.set_bulb_location_time(config[CONF_BULB_LOCATION_TIME]))
    cg.add(var.set_bulb_group(config[CONF_BULB_GROUP]))
    cg.add(var.set_bulb_group_guid(config[CONF_BULB_GROUP_GUID]))
    cg.add(var.set_bulb_group_time(config[CONF_BULB_GROUP_TIME]))

    cg.add(var.set_debug(config[CONF_DEBUG]))

    cg.add_library("ESPAsyncUDP", None)
