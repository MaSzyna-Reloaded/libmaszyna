# A cab screen derived from abstractscreenrenderer, the base class PythonScreenServer brings for the
# runtime the screen runs on; it sends back the major version of that Python and whether the base
# class is its own (openimage() is the base's).
import sys


class fixture_base_screen(abstractscreenrenderer):
	def __init__(self, lookup_path):
		abstractscreenrenderer.__init__(self, lookup_path)

	def get_width(self):
		return 1

	def get_height(self):
		return 1

	def render(self, state):
		return bytes(bytearray(4))

	def getCommands(self):
		return [str(sys.version_info[0]), str(hasattr(self, 'openimage'))]
