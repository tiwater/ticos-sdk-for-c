#
# Copyright (c) Ticos, Inc.
# See License.txt for details
#

"""
Shim around tcs_build_id to keep the original fw_build_id.py file (this file) working as before.
See tcs-build-id/src/tcs_build_id/__init__.py for actual source code.
"""

import os
import sys

scripts_dir = os.path.dirname(os.path.realpath(__file__))
bundled_tcs_build_id_src_dir = os.path.join(scripts_dir, "tcs-build-id", "src")

if os.path.exists(bundled_tcs_build_id_src_dir):
    # Released SDK:
    sys.path.insert(0, bundled_tcs_build_id_src_dir)

from tcs_build_id import *  # noqa

if __name__ == "__main__":
    main()  # noqa
