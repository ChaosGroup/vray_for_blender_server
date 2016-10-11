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

		cmake.append('-DLIBS_ROOT=%s' % os.path.join(self.dir_build, 'blender-for-vray-libs'))

		cmake.append(os.path.join(self.dir_source, "vrayserverzmq"))

		sys.stdout.write('cmake args:\n%s\n' % '\n\t'.join(cmake))
		sys.stdout.flush()

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
		releasePath = os.path.join(self.dir_install, 'V-Ray', 'VRayZmqServer', 'VRayZmqServer')

		sys.stdout.write("##teamcity[setParameter name='env.ENV_ARTEFACT_FILES' value='%s']" % releasePath)
		sys.stdout.flush()
