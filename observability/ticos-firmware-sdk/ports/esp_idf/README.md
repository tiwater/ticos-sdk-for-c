# ESP32 Specific Port

## Overview

This directory contains an implementation of the dependency functions needed to
integrate the Ticos SDK into a ESP-IDF based project.

The port has been tested on major versions 3, 4, 5 of ESP-IDF.

## Directories

The subdirectories within the folder are titled to align with the ESP-IDF
"component" structure.

```plaintext
├── README.md
├── ticos.cmake # cmake helper to include to add the ticos "component"
└── ticos
    ├── CMakeLists.txt
    ├── common # sourced common across all ESP-IDF versions
    ├── include
    ├── v3.x # ESP-IDF sources specific to major version 3
    ├── v4.x # ESP-IDF sources specific to major version 4
    └── v5.x # ESP-IDF sources specific to major version 5
```

## Integrating the SDK

A step-by-step integration guide can be found at https://ticos.io/esp-tutorial
