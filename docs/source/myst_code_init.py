import warnings

from IPython import get_ipython
from IPython.core.error import UsageError

ip = get_ipython()
if ip is not None:
    try:
        ip.run_line_magic("load_ext", "pymor.discretizers.builtin.gui.jupyter")
    except (ImportError, UsageError):
        # ImportError: the extension module (or one of its own imports) is missing.
        # UsageError: %load_ext raises this itself for a magic/extension IPython cannot resolve.
        pass
    ip.run_line_magic("matplotlib", "notebook")

warnings.filterwarnings("ignore", category=UserWarning, module="torch")
warnings.filterwarnings("ignore", category=UserWarning, module="numpy")
