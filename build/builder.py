import argparse
import sys
import getpass
import os
import platform
import re
import socket
import sys
import subprocess
import shutil


def exit_error(err):
    sys.stderr.write(err)
    sys.stderr.write('\n')
    sys.stderr.flush()
    sys.exit(-1)


def log_info(info):
    sys.stdout.write(info)
    sys.stdout.write('\n')
    sys.stdout.flush()


def is_win():
    return sys.platform == "win32"


def is_mac():
    return sys.platform == "darwin"


def is_lnx():
    return sys.platform.find("linux") != -1


def get_ninja_path(src_dir):
    if is_win():
        return os.path.join(src_dir, "build", "tools", "ninja.exe")
    return 'ninja'


def get_qt_path(sdk_path):
    if is_win():
        return os.path.join(args.kdrive2_path, 'qt', '5.6')
    if is_lnx():
        return os.path.join(args.kdrive2_path, 'qt', 'maya2016')
    if is_mac():
        return os.path.join(args.kdrive2_path, 'qt', 'maya2017')


def setup_msvc_2015(sdkPath):
    env = {
        'INCLUDE' : [
            "{KDRIVE}/msvs2015/PlatformSDK/Include/shared",
            "{KDRIVE}/msvs2015/PlatformSDK/Include/um",
            "{KDRIVE}/msvs2015/PlatformSDK/Include/winrt",
            "{KDRIVE}/msvs2015/PlatformSDK/Include/ucrt",
            "{KDRIVE}/msvs2015/include",
            "{KDRIVE}/msvs2015/atlmfc/include",
        ],

        'LIB' : [
            "{KDRIVE}/msvs2015/PlatformSDK/Lib/winv6.3/um/x64",
            "{KDRIVE}/msvs2015/PlatformSDK/Lib/ucrt/x64",
            "{KDRIVE}/msvs2015/atlmfc/lib/amd64",
            "{KDRIVE}/msvs2015/lib/amd64",
        ],

        'PATH' : [
            "{KDRIVE}/msvs2015/bin/amd64",
            "{KDRIVE}/msvs2015/bin",
            "{KDRIVE}/msvs2015/PlatformSDK/bin/x64",
        ] + os.environ['PATH'].split(os.pathsep),

        '__MS_VC_INSTALL_PATH' : [
            "{KDRIVE}/msvs2015"
        ],
    }

    for var in env:
        os.environ[var] = os.pathsep.join(env[var]).format(KDRIVE=sdkPath)



if __name__ == '__main__':
    parser = argparse.ArgumentParser(usage="python3 builder.py [options]")
    parser.add_argument('--install_path', default = "./")
    parser.add_argument('--source_path', default = "")
    parser.add_argument('--libs_path', default = "")
    parser.add_argument('--kdrive2_path', default = "")
    args = parser.parse_args()

    install_path = os.path.normpath(args.install_path)
    if not os.path.exists(install_path):
        os.makedirs(install_path)

    if not os.path.exists(args.source_path):
        exit_error("Invaliud source_path: %s" % source_path)

    if not os.path.exists(args.libs_path):
        exit_error("Invaliud libs_path: %s" % libs_path)

    if not os.path.exists(args.kdrive2_path):
        exit_error("Invaliud kdrive2_path: %s" % kdrive2_path)

    if is_win():
        setup_msvc_2015(args.kdrive2_path)

    cmake = ['cmake']

    cmake.append("-G")
    cmake.append("Ninja")

    if is_lnx():
        cmake.append("-DCMAKE_C_COMPILER=gcc")
        cmake.append("-DCMAKE_CXX_COMPILER=g++")
        cmake.append("-DWITH_STATIC_LIBC=ON")

    cmake.append('-DQT_ROOT=%s' % get_qt_path(args.kdrive2_path))
    cmake.append('-DCMAKE_BUILD_TYPE=Release')
    cmake.append('-DCMAKE_INSTALL_PREFIX=%s' % install_path)
    cmake.append('-DLIBS_ROOT=%s' % args.libs_path)
    cmake.append(args.source_path)

    log_info('cmake args:\n%s\n' % '\n\t'.join(cmake))

    if not subprocess.call(cmake) == 0:
        exit_error("There was an error during configuration!")

    ninja = [get_ninja_path(args.source_path), 'install']
    log_info('ninja args:\n%s\n' % '\n\t'.join(ninja))
    if not subprocess.call(ninja) == 0:
        exit_error("There was an error during configuration!")