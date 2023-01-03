import warnings

from IPython import get_ipython

ip = get_ipython()
if ip is not None:
    # ip.run_line_magic('load_ext', 'pymor.discretizers.builtin.gui.jupyter')
    ip.run_line_magic("matplotlib", "inline")

warnings.filterwarnings("ignore", category=UserWarning, module="torch")
