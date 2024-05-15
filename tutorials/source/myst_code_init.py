import warnings

from IPython import get_ipython

ip = get_ipython()
if ip is not None:
    try:
        ip.run_line_magic("load_ext", "pymor.discretizers.builtin.gui.jupyter")
    except ImportError:
        pass
    ip.run_line_magic("matplotlib", "notebook")

warnings.filterwarnings("ignore", category=UserWarning, module="torch")
warnings.filterwarnings("ignore", category=UserWarning, module="numpy")
