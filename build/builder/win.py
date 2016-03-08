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
import subprocess

from .builder import utils
from .builder import Builder


class WindowsBuilder(Builder):
	def compile(self):
		cmake_build_dir = self.dir_build
		if os.path.exists(cmake_build_dir):
			if self.build_clean:
				utils.remove_directory(cmake_build_dir)
		else:
			os.makedirs(cmake_build_dir)
		os.chdir(cmake_build_dir)

		if self.mode_test:
			return

		cmake = ['cmake']

		cmake.append("-G")
		cmake.append("Ninja")

		cmake.append('-DCMAKE_BUILD_TYPE=%s' % os.environ['CGR_BUILD_TYPE'])
		cmake.append('-DCMAKE_INSTALL_PREFIX=%s' % self.dir_install_path)

		cmake.append('-DAPPSDK_PATH=%s' % os.environ['CGR_APPSDK_PATH'])
		cmake.append('-DAPPSDK_VERSION=%s' % os.environ['CGR_APPSDK_VERSION'])

		cmake.append('-DLIBS_ROOT=%s' % utils.path_join(self.dir_build, '..', 'blender-for-vray-libs'))

		cmake.append("../vrayserverzmq")

		res = subprocess.call(cmake)
		if not res == 0:
			sys.stderr.write("There was an error during configuration!\n")
			sys.exit(1)

		ninja = utils.path_join(self.dir_source, "build", "tools", "ninja.exe")


		make = [ninja]
		make.append('-j%s' % self.build_jobs)
		make.append('install')

		res = subprocess.call(make)
		if not res == 0:
			sys.stderr.write("There was an error during the compilation!\n")
			sys.exit(1)


	def config(self):
		# Not used on Windows anymore
		pass


	def installer_cgr(self, installer_path):
		pass
		# utils.GenCGRInstaller(self, installer_path, InstallerDir=self.dir_cgr_installer)


	def package(self):
		pass

	def package_archive(self):
		pass
