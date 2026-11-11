# A cab screen that compiles and loads on Python 3 but fails there while it draws: a rectangle with
# its corners swapped, which the Pillow of 2.7 draws and Pillow 10 or later refuses (as
# dynamic/pkp/en57al_v1/screen_en57al.py does); it sends back the major version of the Python it
# runs on.
import sys
from PIL import Image, ImageDraw


class fixture_python2_draw_screen(object):
	def __init__(self, lookup_path):
		pass

	def manul_set_format(self, format_str):
		pass

	def get_width(self):
		return 2

	def get_height(self):
		return 1

	def render(self, state):
		image = Image.new("RGBA", (2, 1))
		ImageDraw.Draw(image).rectangle((1, 0, 0, 0), fill=(255, 255, 255, 255))
		return image.tobytes()

	def getCommands(self):
		return [str(sys.version_info[0])]
