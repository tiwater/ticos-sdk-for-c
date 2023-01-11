# Ticos Library for use with Particle Device OS

Ship Firmware with Confidence.

More details about the Ticos platform itself, how it works, and step-by-step
integration guides
[can be found here](https://ticos.io/particle-getting-started).

The library is compatible with all versions of
[Device OS](https://github.com/particle-iot/device-os) greater than or equal to
Device OS 3.0

:exclamation: Note: Use the
[particle-firmware-library](https://github.com/ticos/particle-firmware-library)
repository to add ticos support to an application. This repository is updated
as part of the release process for the
[ticos-firmware-sdk](https://github.com/ticos/ticos-firmware-sdk).

## Welcome to your library!

To get started, add the library to your particle application:

```bash
$ git submodule add https://github.com/ticos/particle-firmware-library lib/ticos
```

## Integration Steps

1. Add the following to your application

   ```c
   #include "ticos.h"
   Ticos ticos;

   void loop() {
     // ...
     ticos.process();
     // ...
   }
   ```

2. Create a Ticos Project Key at
   https://goto.ticos.com/create-key/particle and copy it to your clipboard.

3. Navigate to the Integrations tab in the Particle Cloud UI and create a new
   "Custom Template" webhook. Be sure to replace `TICOS_PROJECT_KEY` below
   with the one copied in step 2.

   ```json
   {
     "event": "ticos-chunks",
     "responseTopic": "",
     "url": "https://chunks.ticos.com/api/v0/chunks/{{PARTICLE_DEVICE_ID}}",
     "requestType": "POST",
     "noDefaults": false,
     "rejectUnauthorized": true,
     "headers": {
       "Ticos-Project-Key": "TICOS_PROJECT_KEY",
       "Content-Type": "application/octet-stream",
       "Content-Encoding": "base64"
     },
     "body": "{{{PARTICLE_EVENT_VALUE}}}"
   }
   ```

## Example Usage

See the [examples/ticos_test/](examples/ticos_test) folder for a complete
application demonstrating how to use Ticos.

## Questions

Don't hesitate to contact us for help! You can reach us through
[support@ticos.com](mailto:support@ticos.com).
