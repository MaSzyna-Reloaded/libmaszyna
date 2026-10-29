# A Python 2 cab screen that counts the renders of its own instance and sends the count back as a
# command - two screens of one script tell by it whether they share an instance.
class fixture_counter_screen(object):
	def __init__(self, lookup_path):
		self.renders = 0

	def manul_set_format(self, format_str):
		pass

	def get_width(self):
		return 1

	def get_height(self):
		return 1

	def render(self, state):
		self.renders += 1
		return chr(0) * 4

	def getCommands(self):
		return [str(self.renders)]
