# A cab screen, the same on Python 2 and 3, that ends the interpreter's process the way a broken
# runtime does (Py_FatalError() ends with abort()) - PythonScreenServer must survive it.
import os


class fixture_abort_screen(object):
	def __init__(self, lookup_path):
		pass

	def manul_set_format(self, format_str):
		pass

	def render(self, state):
		os.abort()
