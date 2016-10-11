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
	def setup_msvc_2013(self, cgrepo):
		env = {
			'INCLUDE' : [
				"{CGR_SDK}/msvs2013/PlatformSDK/Include/shared",
				"{CGR_SDK}/msvs2013/PlatformSDK/Include/um",
				"{CGR_SDK}/msvs2013/PlatformSDK/Include/winrt",
				"{CGR_SDK}/msvs2013/PlatformSDK/Include/ucrt",
				"{CGR_SDK}/msvs2013/include",
				"{CGR_SDK}/msvs2013/atlmfc/include",
			],

			'LIB' : [
				"{CGR_SDK}/msvs2013/PlatformSDK/Lib/winv6.3/um/x64",
				"{CGR_SDK}/msvs2013/PlatformSDK/Lib/ucrt/x64",
				"{CGR_SDK}/msvs2013/atlmfc/lib/amd64",
				"{CGR_SDK}/msvs2013/lib/amd64",
			],

			'PATH' : [
					"{CGR_SDK}/msvs2013/bin/amd64",
					"{CGR_SDK}/msvs2013/bin",
					"{CGR_SDK}/msvs2013/PlatformSDK/bin/x64",
				] + os.environ['PATH'].split(';')
			,
		}
		os.environ['__MS_VC_INSTALL_PATH'] = "{CGR_SDK}/msvs2013"
		for var in env:
			os.environ[var] = ";".join(env[var]).format(CGR_SDK=cgrepo)

	def compile(self):
		if self.mode_test:
			return

		cmake = [cmake_path]

		cmake.append("-G")
		cmake.append("Ninja")

		old_path = ''
		if 'JENKINS_WIN_SDK_PATH' in os.environ:
			for path in os.environ['PATH'].split(';'):
				if path.endswith('cmake.exe'):
					cmake[0] = path
					break

			old_path = os.environ['PATH']
			os.environ['PATH'] = ''
			self.setup_msvc_2013(os.environ['JENKINS_WIN_SDK_PATH'])
			cmake.append('-DQT_ROOT=%s' % utils.path_join(os.environ['JENKINS_WIN_SDK_PATH'], 'qt', '4.8.4'))

		cmake.append('-DCMAKE_BUILD_TYPE=%s' % os.environ['CGR_BUILD_TYPE'])
		cmake.append('-DCMAKE_INSTALL_PREFIX=%s' % self.dir_install)

		cmake.append('-DAPPSDK_PATH=%s' % os.environ['CGR_APPSDK_PATH'])
		cmake.append('-DAPPSDK_VERSION=%s' % os.environ['CGR_APPSDK_VERSION'])

		cmake.append('-DLIBS_ROOT=%s' % utils.path_join(self.dir_source, 'blender-for-vray-libs'))

		cmake.append(utils.path_join(self.dir_source, "vrayserverzmq"))

		sys.stdout.write('PATH:\n\t%s\n' % '\n\t'.join(os.environ['PATH'].split(';')))
		sys.stdout.write('cmake args:\n%s\n' % '\n\t'.join(cmake))
		sys.stdout.flush()

		res = subprocess.call(cmake)
		if not res == 0:
			sys.stderr.write("There was an error during configuration!\n")
			sys.exit(1)

		ninja = utils.path_join(self.dir_source, "vrayserverzmq", "build", "tools", "ninja.exe")

		make = [ninja]
		make.append('-j%s' % self.build_jobs)
		make.append('install')

		res = subprocess.call(make)
		if not res == 0:
			sys.stderr.write("There was an error during the compilation!\n")
			sys.exit(1)

		if 'JENKINS_WIN_SDK_PATH' in os.environ:
			os.environ['PATH'] = old_path


	def config(self):
		# Not used on Windows anymore
		pass


	def installer_cgr(self, installer_path):
		pass
		# utils.GenCGRInstaller(self, installer_path, InstallerDir=self.dir_cgr_installer)


	def package(self):
		releasePath = os.path.join(self.dir_install, 'V-Ray', 'VRayZmqServer', 'VRayZmqServer.exe')

		sys.stdout.write("##teamcity[setParameter name='env.ENV_ARTEFACT_FILES' value='%s']" % os.path.normpath(releasePath))
		sys.stdout.flush()

	def package_archive(self):
		pass
