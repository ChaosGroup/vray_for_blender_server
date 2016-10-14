#
# V-Ray For Blender TeamCity Build Wrapper
#
# http://chaosgroup.com
#
# Author: Andrei Izrantcev
# E-Mail: andrei.izrantcev@chaosgroup.com
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


def main(args):
    import os
    import sys
    import subprocess
    from builder import utils

    sys.stdout.write('temacity args:\n%s\n' % str(args))
    sys.stdout.flush()

    python_exe = sys.executable

    if sys.platform == 'win32' and not args.jenkins:
        # Setup Visual Studio 2013 variables for usage from command line
        # Assumes default installation path
        #
        # PATH
        #
        PATH = os.environ['PATH'].split(';')
        PATH.extend([
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow',
            r'C:\Program Files (x86)\MSBuild\12.0\bin\amd64',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\BIN\amd64',
            r'C:\Windows\Microsoft.NET\Framework64\v4.0.30319',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\VCPackages',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools',
            r'C:\Program Files (x86)\HTML Help Workshop',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\Team Tools\Performance Tools\x64',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\Team Tools\Performance Tools',
            r'C:\Program Files (x86)\Windows Kits\8.1\bin\x64',
            r'C:\Program Files (x86)\Windows Kits\8.1\bin\x86',
            r'C:\Program Files (x86)\Microsoft SDKs\Windows\v8.1A\bin\NETFX 4.5.1 Tools\x64',
            r'C:\ProgramData\Oracle\Java\javapath',
            r'C:\Windows\system32',
            r'C:\Windows',
            r'C:\Windows\System32\Wbem',
            r'C:\Windows\System32\WindowsPowerShell\v1.0',
            r'C:\Program Files (x86)\Windows Kits\8.1\Windows Performance Toolkit',
            r'C:\Program Files\Microsoft SQL Server\110\Tools\Binn',
        ])

        INCLUDE = (
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\INCLUDE',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\ATLMFC\INCLUDE',
            r'C:\Program Files (x86)\Windows Kits\8.1\include\shared',
            r'C:\Program Files (x86)\Windows Kits\8.1\include\um',
            r'C:\Program Files (x86)\Windows Kits\8.1\include\winrt',
        )

        LIB = (
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\LIB\amd64',
            r'C:\ProgramFiles (x86)\Microsoft Visual Studio 12.0\VC\ATLMFC\LIB\amd64',
            r'C:\Program Files (x86)\Windows Kits\8.1\lib\winv6.3\um\x64',
        )

        LIBPATH = (
            r'C:\Windows\Microsoft.NET\Framework64\v4.0.30319',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\LIB\amd64',
            r'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\ATLMFC\LIB\amd64',
            r'C:\Program Files (x86)\Windows Kits\8.1\References\CommonConfiguration\Neutral',
            r'C:\Program Files (x86)\Microsoft SDKs\Windows\v8.1\ExtensionSDKs\Microsoft.VCLibs\12.0\References\CommonConfiguration\neutral',
        )

        os.environ['PATH']    = ";".join(PATH)
        os.environ['INCLUDE'] = ";".join(INCLUDE)
        os.environ['LIB']     = ";".join(LIB)
        os.environ['LIBPATH'] = ";".join(LIBPATH)

    source_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', '..')
    source_path = os.path.realpath(source_path)

    cmd = [python_exe]
    cmd.append(os.path.join(source_path, "vrayserverzmq", "build", "build.py"))
    cmd.append("--teamcity")
    cmd.append("--teamcity_branch_hash=%s" % args.teamcity_branch_hash)
    cmd.append('--dir_source=%s' % os.path.join(source_path))

    if args.teamcity_build_path != '':
        cmd.append('--dir_build=%s' % args.teamcity_build_path)
    else:
        cmd.append('--dir_build=%s' % os.path.join(source_path, "vrayserverzmq-cmake-build"))


    # install path defaults
    if args.teamcity_install_path != '':
        cmd.append('--dir_install=%s' % args.teamcity_install_path)
    else:
        if sys.platform == 'win32':
            cmd.append('--dir_install=H:/install/vrayserverzmq')
        else:
            cmd.append('--dir_install=%s' % os.path.expanduser("~/install/vrayserverzmq"))

    #release path defaults
    if args.teamcity_release_path != '':
        cmd.append('--dir_release=%s' % args.teamcity_release_path)
    else:
        if sys.platform == 'win32':
            cmd.append('--dir_release=H:/release/vrayserverzmq')
        else:
            cmd.append('--dir_release=%s' % os.path.expanduser("~/release/vrayserverzmq"))

    if sys.platform == 'win32':
        pass
    else:
        # No Cycles on TeamCity, failed to build some dependencies
        # cmd.append('--with_cycles')
        if sys.platform != 'darwin':
            if args.jenkins:
                cmd.append('--gcc=gcc482')
                cmd.append('--gxx=g++482')
            else:
                cmd.append('--gcc=gcc-4.9.3')
                cmd.append('--gxx=g++-4.9.3')
        cmd.append('--dev_static_libs')

    sys.stdout.write('Calling builder:\n%s\n' % '\n\t'.join(cmd))
    sys.stdout.flush()

    return subprocess.call(cmd, cwd=os.getcwd())


if __name__ == '__main__':
    import argparse
    import sys

    parser = argparse.ArgumentParser(usage="python3 build.py [options]")

    parser.add_argument('--teamcity_branch_hash',
        default = ""
    )

    parser.add_argument('--teamcity_install_path',
        default = "",
    )

    parser.add_argument('--teamcity_release_path',
        default = "",
    )

    parser.add_argument('--teamcity_build_path',
        default = "",
    )

    parser.add_argument('--jenkins',
        action = 'store_true',
    )

    args = parser.parse_args()
    sys.exit(main(args))
