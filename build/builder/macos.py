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
import subprocess

from .builder import utils
from .builder import Builder


class MacBuilder(Builder):
	def config(self):
		# Not used on OS X anymore
		pass


	def compile(self):
		cmake_build_dir = os.path.join(self.dir_source, "blender-cmake-build")
		if os.path.exists(cmake_build_dir):
			if self.build_clean:
				utils.remove_directory(cmake_build_dir)
		else:
			os.makedirs(cmake_build_dir)
		os.chdir(cmake_build_dir)

		distr_info = utils.get_linux_distribution()

		cmake = ['cmake']

		cmake.append("-G")
		cmake.append("Ninja")

		if self.gcc:
			cmake.append("-DCMAKE_C_COMPILER=%s" % self.gcc)
		if self.gxx:
			cmake.append("-DCMAKE_CXX_COMPILER=%s" % self.gxx)

		cmake.append('-DCMAKE_BUILD_TYPE=%s' % os.environ['CGR_BUILD_TYPE'])
		cmake.append('-DCMAKE_INSTALL_PREFIX=%s' % self.dir_install)

		cmake.append('-DAPPSDK_PATH=%s' % os.environ['CGR_APPSDK_PATH'])
		cmake.append('-DAPPSDK_VERSION=%s' % os.environ['CGR_APPSDK_VERSION'])

		if distr_info['short_name'] == 'centos' and distr_info['version'] == '6.7':
			cmake.append('-DWITH_STATIC_LIBC=ON')

		cmake.append('-DLIBS_ROOT=%s' % utils.path_join(self.dir_build, '..', 'blender-for-vray-libs'))

		cmake.append("../vrayserverzmq")

		res = subprocess.call(cmake)
		if not res == 0:
			sys.stderr.write("There was an error during configuration!\n")
			sys.exit(1)

		make = ['ninja']
		make.append('-j%s' % self.build_jobs)
		make.append('install')

		res = subprocess.call(make)
		if not res == 0:
			sys.stderr.write("There was an error during the compilation!\n")
			sys.exit(1)

	def package(self):
		subdir = "macos" + "/" + self.build_arch

		release_path = utils.path_join(self.dir_release, subdir)

		if not self.mode_test:
			utils.path_create(release_path)

		# Example: vrayblender-2.60-42181-macos-10.6-x86_64.tar.bz2
		archive_name = utils.GetPackageName(self)
		archive_path = utils.path_join(release_path, archive_name)

		sys.stdout.write("Generating archive: %s\n" % (archive_name))
		sys.stdout.write("  in: %s\n" % (release_path))

		cmd = "tar jcf %s %s" % (archive_path, self.dir_install_name)

		sys.stdout.write("Calling: %s\n" % (cmd))
		sys.stdout.write("  in: %s\n" % (self.dir_install))

		if not self.mode_test:
			os.chdir(self.dir_install)
			os.system(cmd)

		return subdir, archive_path
