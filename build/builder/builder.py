#
# V-Ray/Blender Build System
#
# http://vray.cgdo.ru
#
# Author: Andrey M. Izrantsev (aka bdancer)
# E-Mail: izrantsev@cgdo.ru
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# All Rights Reserved. V-Ray(R) is a registered trademark of Chaos Software.
#


import os
import sys
import shutil
import tempfile
import subprocess
import datetime

from . import utils

class Builder:
	"""
	  A generic build class.
	"""

	def __init__(self, params):
		# Store options as attrs
		for p in params:
			setattr(self, p, params[p])

		# Always building vb30
		self.project        = "vrayserverzmq"

		self.version        = utils.VERSION
		self.revision       = utils.REVISION
		self.brev           = ""
		self.commits        = '0'

		self.teamcity_branch_hash = self.teamcity_branch_hash[:7]
		self.dir_install_path = os.path.join(self.dir_install, self.teamcity_branch_hash)

		# Host info
		self.host_os        = utils.get_host_os()
		self.host_arch      = utils.get_host_architecture()
		self.host_name      = utils.get_hostname()
		self.host_username  = utils.get_username()
		self.host_linux     = utils.get_linux_distribution()

		# Build architecture
		self.build_arch     = self.host_arch


	def info(self):
		sys.stdout.write("\n")
		sys.stdout.write("Build information:\n")

		sys.stdout.write("OS: %s\n" % (self.host_os.title()))

		if self.host_os == utils.LNX:
			sys.stdout.write("Distribution: %s %s\n" % (self.host_linux["long_name"], self.host_linux["version"]))

		sys.stdout.write("Architecture: %s\n" % (self.host_arch))
		sys.stdout.write("Build architecture: %s\n" % (self.build_arch))
		sys.stdout.write("Target: %s %s (%s)\n" % (self.project, self.version, self.revision))
		sys.stdout.write("Source directory:  %s\n" % (self.dir_source))
		sys.stdout.write("Build directory:   %s\n" % (self.dir_build))
		sys.stdout.write("Install directory: %s\n" % (self.dir_install_path))
		sys.stdout.write("Release directory: %s\n" % (self.dir_release))
		sys.stdout.write("\n")


	def post_init(self):
		"""
		  Override this method in subclass.
		"""
		pass


	def init_paths(self):
		if self.package:
			if not self.mode_test:
				utils.path_create(self.dir_release)

		self.dir_build        = utils.path_slashify(self.dir_build)
		self.dir_source       = utils.path_slashify(self.dir_source)
		self.dir_install_path = utils.path_slashify(self.dir_install_path)

		if self.build_clean:
			if os.path.exists(self.dir_build):
				shutil.rmtree(self.dir_build)


	def compile(self):
		"""
		  Override this method in subclass.
		"""
		sys.stderr.write("Base class method called: package() This souldn't happen.\n")


	def compile_post(self):
		pass
	# 	if self.host_os == utils.WIN:
	# 		runtimeDir = utils.path_join(self.dir_source, "build", "non-gpl", self.build_arch)
	# 		files = []
	# 		if self.vc2013:
	# 			files.extend([
	# 				"msvcp120.dll",
	# 				"msvcr120.dll",
	# 				"vcomp120.dll",
	# 			])
	# 		else:
	# 			files.append("vcomp90.dll")
	# 		for f in files:
	# 			shutil.copy(utils.path_join(runtimeDir, f), self.dir_install_path)


	def package(self):
		"""
		  Override this method in subclass.
		"""
		sys.stderr.write("Base class method called: package() This souldn't happen.\n")


	def build(self):
		self.init_paths()
		self.post_init()

		self.info()

		if not self.export_only:
			self.compile()
			self.compile_post()
