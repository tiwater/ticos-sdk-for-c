#!/usr/bin/env zx

// Copyright (c) Tiwater Technologies. All rights reserved.
// SPDX-License-Identifier: MIT

if (!argv['sdk-version']) {
  console.log(chalk.red('--sdk-version not provided'))
  await $`exit -1`
}

if (!argv['lib-version']) {
  console.log(chalk.red('--lib-version not provided'))
  await $`exit -1`
}

// param(
// 	$SdkVersion = $(throw "SdkVersion not provided"),
// 	$NewLibraryVersion = $(throw "NewLibraryVersion not provided")
// )

const { 'sdk-version': sdkVersion, 'lib-version': libVersion } = argv;

const srcFolder = './src'
const libConfigFile = './library.properties'

console.log(chalk.green('Cloning ticos-sdk-for-c repository.'))
await $`git clone -b ${sdkVersion} git@github.com:tiwater/ticos-sdk-for-c.git sdkrepo`

console.log(chalk.green('Flattening the ticos-sdk-for-c file structure and updating src/.'))
// Filtering out files not needed/supported on Arduino.
const files = await glob(['./sdkrepo/sdk/**/*.{c,h}', '!**/{tests,samples}/**', '!**/*{curl,win32,ti_posix}*'])

await $`rm -f ${srcFolder}/*`

await Promise.all(files.map(async (f) => {
  await $`cp -vvv ${f} ${srcFolder}`
}))

// Fixing headers to work as a flat structure.

await $`sed -E -i '' -e 's/<ticos\\/(iot\\/internal|core\\/internal|iot|core|storage)\\//</g' src/*`

console.log(chalk.green('Removing clone of ticos-sdk-for-c.'))

await $`rm -rf sdkrepo`

console.log(chalk.green('Updating versions.'))

// Update Arduino library version with SDK version.
await $`yq -p=props -o=props -i '.version="${libVersion}" | .url = "https://github.com/tiwater/ticos-sdk-for-c/tree/${sdkVersion}"' library.properties`

console.log(chalk.yellow('You must manually update the library.properties with any new includes such as ti_core.h, ti_iot.h'))
