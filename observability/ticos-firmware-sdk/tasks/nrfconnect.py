#
# Copyright (c) Ticos, Inc.
# See License.txt for details
#

import os

from invoke import task

SDK_FW_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NRFCONNECT_ROOT = os.path.join(SDK_FW_ROOT, "examples", "nrf-connect-sdk", "nrf9160")
NRFCONNECT_DEMO_APP_ROOT = os.path.join(NRFCONNECT_ROOT, "ticos_demo_app")

CI_TICOS_SDK_REPO_ROOT = os.path.join(SDK_FW_ROOT, "build", "release-repo")

# Manifest override for CI as suggested at:
#  https://docs.zephyrproject.org/latest/guides/west/manifest.html#example-2-3-continuous-integration-overrides
#
# This allows us to compile the nRF Connect against the latest version of the ticos-firmware-sdk in CI
# prior to it being released
CI_MANIFEST_OVERRIDE_PATH = os.path.join(NRFCONNECT_DEMO_APP_ROOT, "submanifests", "00-ci.yml")

CI_MANIFEST_OVERRIDE_TXT = f"""
manifest:
  projects:
    - name: ticos-firmware-sdk
      url: {CI_TICOS_SDK_REPO_ROOT}
      path: modules/ticos-firmware-sdk
      revision: master
"""


@task()
def nrfconnect_project_ci_setup(ctx):
    """Spin up a Demo App For Testing the nRF Connect SDK Integration

    We'll override the manifest and compile things against a CI build of the
    ticos-firmware-sdk rather than the github repo itself
    """
    with open(CI_MANIFEST_OVERRIDE_PATH, "w") as f:
        f.write(CI_MANIFEST_OVERRIDE_TXT)

    with ctx.cd(CI_TICOS_SDK_REPO_ROOT):
        ctx.run("git init")
        ctx.run('git config user.email "hello@ticos.com"')
        ctx.run('git config user.name "Ticos Inc."')

        ctx.run("git add .")
        ctx.run('git commit -m "CI Test Changes"')

    with ctx.cd(NRFCONNECT_ROOT):
        ctx.run(f"west init -l {NRFCONNECT_DEMO_APP_ROOT}")
        ctx.run("west update")
