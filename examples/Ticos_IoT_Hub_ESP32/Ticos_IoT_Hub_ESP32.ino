#include "ti_iot_api.h"
#include "user_app.h"

void setup()
{
  user_init();
  ti_iot_cloud_start();
}

void loop()
{
  key_scan();
}
