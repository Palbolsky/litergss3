// Sensor input module — `LiteRGSS::Inputs::Sensor` + `Sf::Sensor` alias.
//
// Backends without sensor support (raylib 5.x) report every sensor as
// unavailable and read [0, 0, 0].

#include "Sensor.h"
#include "Inputs.h"
#include "LiteRGSS.h"
#include <LiteCGSS/Events/SensorType.h>
#include <LiteCGSS/Inputs/Sensor.h>

VALUE rb_mSensor = Qnil;

// Ruby integer -> cgss::SensorType, false when out of the enum's range.
static bool to_sensor_type(VALUE val, cgss::SensorType& out)
{
    const long t = NUM2LONG(val);
    if (t < static_cast<long>(cgss::SensorType::Accelerometer) ||
        t > static_cast<long>(cgss::SensorType::Orientation)) return false;
    out = static_cast<cgss::SensorType>(t);
    return true;
}

VALUE rb_Sensor_isAvailable(VALUE self, VALUE val)
{
    (void)self;
    cgss::SensorType type;
    if (!to_sensor_type(val, type)) return Qfalse;
    return cgss::sensor::isAvailable(type) ? Qtrue : Qfalse;
}

VALUE rb_Sensor_setEnabled(VALUE self, VALUE sType, VALUE enabled)
{
    cgss::SensorType type;
    if (!to_sensor_type(sType, type)) return Qfalse;
    cgss::sensor::setEnabled(type, RTEST(enabled));
    return self;
}

VALUE rb_Sensor_getValue(VALUE self, VALUE sType)
{
    (void)self;
    cgss::SensorType type;
    const cgss::Vector3 v = to_sensor_type(sType, type) ? cgss::sensor::value(type) : cgss::Vector3{};
    VALUE ary = rb_ary_new_capa(3);
    rb_ary_push(ary, DBL2NUM(static_cast<double>(v.x)));
    rb_ary_push(ary, DBL2NUM(static_cast<double>(v.y)));
    rb_ary_push(ary, DBL2NUM(static_cast<double>(v.z)));
    return ary;
}

void Init_Sensor()
{
    rb_mSensor = rb_define_module_under(rb_mInputs, "Sensor");

    rb_define_const(rb_mSensor, "ACCELEROMETER", LONG2NUM(static_cast<long>(cgss::SensorType::Accelerometer)));
    rb_define_const(rb_mSensor, "GYROSCOPE",     LONG2NUM(static_cast<long>(cgss::SensorType::Gyroscope)));
    rb_define_const(rb_mSensor, "MAGNETOMETER",  LONG2NUM(static_cast<long>(cgss::SensorType::Magnetometer)));
    rb_define_const(rb_mSensor, "GRAVITY",       LONG2NUM(static_cast<long>(cgss::SensorType::Gravity)));
    rb_define_const(rb_mSensor, "USER_ACCELERATION", LONG2NUM(static_cast<long>(cgss::SensorType::UserAcceleration)));
    rb_define_const(rb_mSensor, "ORIENTATION",   LONG2NUM(static_cast<long>(cgss::SensorType::Orientation)));

    rb_define_module_function(rb_mSensor, "available?", _rbf rb_Sensor_isAvailable, 1);
    rb_define_module_function(rb_mSensor, "set_enabled", _rbf rb_Sensor_setEnabled, 2);
    rb_define_module_function(rb_mSensor, "value", _rbf rb_Sensor_getValue, 1);
}
