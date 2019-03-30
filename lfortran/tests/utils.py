import sys
import pytest

linux_only = pytest.mark.skipif(sys.platform != "linux",
                                reason="Runs on Linux only")
