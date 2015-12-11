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

class LinuxBuilder(Builder):
	def post_init(self):
		pass

	def compile(self):
		cmake_build_dir = self.dir_build
		if os.path.exists(cmake_build_dir):
			if self.build_clean:
				utils.remove_directory(cmake_build_dir)
		else:
			os.makedirs(cmake_build_dir)
		os.chdir(cmake_build_dir)
		PYTHON_VERSION = "3.4"

		distr_info = utils.get_linux_distribution()

		cmake = ['cmake']

		cmake.append("-G")
		cmake.append("Ninja")

		if self.gcc:
			cmake.append("-DCMAKE_C_COMPILER=%s" % self.gcc)
		if self.gxx:
			cmake.append("-DCMAKE_CXX_COMPILER=%s" % self.gxx)

		cmake.append('-DCMAKE_BUILD_TYPE=%s' % os.environ['CGR_BUILD_TYPE'])
		cmake.append('-DCMAKE_INSTALL_PREFIX=%s' % self.dir_install_path)

		cmake.append('-DAPPSDK_PATH=%s' % os.environ['CGR_APPSDK_PATH'])
		cmake.append('-DAPPSDK_VERSION=%s' % os.environ['CGR_APPSDK_VERSION'])

		cmake.append('-DZMQ_ROOT=%s' % os.environ['CGR_ZMQ_ROOT'])
		cmake.append('-DBoost_DIR=%s' % os.environ['CGR_BOOST_ROOT'])
		cmake.append('-DBoost_INCLUDE_DIR=%s' % os.path.join(os.environ['CGR_BOOST_ROOT'], 'include'))
		cmake.append('-DBoost_LIBRARY_DIRS=%s' % os.path.join(os.environ['CGR_BOOST_ROOT'], 'lib'))

		cmake.append("../vrayserverzmq")

		if self.mode_test:
			print(" ".join(cmake))

		else:
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
		pass
		# subdir = "linux" + "/" + self.build_arch

		# release_path = os.path.join(self.dir_release, subdir)

		# if not self.mode_test:
		# 	utils.path_create(release_path)

		# archive_name = utils.GetPackageName(self)
		# archive_path = utils.path_join(release_path, archive_name)

		# sys.stdout.write("Generating archive: %s\n" % (archive_name))
		# sys.stdout.write("  in: %s\n" % (release_path))

		# cmd = "tar jcf %s %s" % (archive_path, self.dir_install_name)

		# sys.stdout.write("Calling: %s\n" % (cmd))
		# sys.stdout.write("  in: %s\n" % (self.dir_install))

		# if not self.mode_test:
		# 	os.chdir(self.dir_install)
		# 	os.system(cmd)

		# return subdir, archive_path