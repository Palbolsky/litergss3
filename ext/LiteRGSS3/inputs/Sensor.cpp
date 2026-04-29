// Sensor input module — `LiteRGSS::Inputs::Sensor` + `Sf::Sensor` alias.
//
// Limitation: cgss::backend::ActiveBackend::Ops exposes no sensor_* — only
// the cgss::SensorType enum and a sensor event payload exist. Routes
// through direct backend calls behind CGSS_BACKEND_* guards. raylib has no
// device-sensor API in 5.x, so reads return [0,0,0] and is_available?
// returns false there. TODO: collapse onto Ops::sensor_* once LiteCGSS2
// exposes them.

#include "Sensor.h"
#include "Inputs.h"
#include "LiteRGSS.h"
#include <LiteCGSS/Events/SensorType.h>

#if defined(CGSS_BACKEND_SFML)
    #include <SFML/Window/Sensor.hpp>
#endif

VALUE rb_mSensor = Qnil;

VALUE rb_Sensor_isAvailable(VALUE self, VALUE val)
{
    (void)self;
    long t = NUM2LONG(val);
#if defined(CGSS_BACKEND_SFML)
    if (t < 0 || t >= sf::Sensor::Type::Count) return Qfalse;
    return sf::Sensor::isAvailable(static_cast<sf::Sensor::Type>(t)) ? Qtrue : Qfalse;
#else
    (void)t;
    return Qfalse;
#endif
}

VALUE rb_Sensor_setEnabled(VALUE self, VALUE sType, VALUE enabled)
{
    long t = NUM2LONG(sType);
#if defined(CGSS_BACKEND_SFML)
    if (t < 0 || t >= sf::Sensor::Type::Count) return Qfalse;
    sf::Sensor::setEnabled(static_cast<sf::Sensor::Type>(t), RTEST(enabled));
#else
    (void)t; (void)enabled;
#endif
    return self;
}

VALUE rb_Sensor_getValue(VALUE self, VALUE sType)
{
    (void)self;
    long t = NUM2LONG(sType);
    VALUE ary = rb_ary_new_capa(3);
#if defined(CGSS_BACKEND_SFML)
    if (t < 0 || t >= sf::Sensor::Type::Count) {
        rb_ary_push(ary, DBL2NUM(0.0));
        rb_ary_push(ary, DBL2NUM(0.0));
        rb_ary_push(ary, DBL2NUM(0.0));
        return ary;
    }
    auto v = sf::Sensor::getValue(static_cast<sf::Sensor::Type>(t));
    rb_ary_push(ary, DBL2NUM(static_cast<double>(v.x)));
    rb_ary_push(ary, DBL2NUM(static_cast<double>(v.y)));
    rb_ary_push(ary, DBL2NUM(static_cast<double>(v.z)));
#else
    (void)t;
    rb_ary_push(ary, DBL2NUM(0.0));
    rb_ary_push(ary, DBL2NUM(0.0));
    rb_ary_push(ary, DBL2NUM(0.0));
#endif
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
