# A cab screen that compiles on Python 3 but does not load there (sys.maxint is Python 2 only), so it
# runs on 2.7; it sends back the major version of the Python it runs on.
import sys
from sys import maxint


class fixture_python2_load_screen(object):
	def __init__(self, lookup_path):
		pass

	def manul_set_format(self, format_str):
		pass

	def get_width(self):
		return 1

	def get_height(self):
		return 1

	def render(self, state):
		return bytes(bytearray(4))

	def getCommands(self):
		return [str(sys.version_info[0])]
